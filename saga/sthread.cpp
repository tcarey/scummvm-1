/* ScummVM - Scumm Interpreter
 * Copyright (C) 2004-2005 The ScummVM project
 *
 * The ReInherit Engine is (C)2000-2003 by Daniel Balsom.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 *
 */

// Scripting module thread management component
#include "saga/saga.h"

#include "saga/gfx.h"
#include "saga/actor.h"
#include "saga/console.h"
#include "saga/interface.h"

#include "saga/script.h"

#include "saga/stream.h"
#include "saga/scene.h"
#include "saga/resnames.h"

namespace Saga {

ScriptThread *Script::createThread(uint16 scriptModuleNumber, uint16 scriptEntryPointNumber) {
	ScriptThread *newThread;

	loadModule(scriptModuleNumber);
	if (_modules[scriptModuleNumber].entryPointsCount <= scriptEntryPointNumber) {
		error("Script::createThread wrong scriptEntryPointNumber");
	}
		
	newThread = _threadList.pushFront().operator->();
	newThread->_flags = kTFlagNone;
	newThread->_stackSize = DEFAULT_THREAD_STACK_SIZE;
	newThread->_stackBuf = (uint16 *)malloc(newThread->_stackSize * sizeof(*newThread->_stackBuf));
	newThread->_stackTopIndex = newThread->_stackSize - 1; // or 2 - as in original
	newThread->_instructionOffset = _modules[scriptModuleNumber].entryPoints[scriptEntryPointNumber].offset;
	newThread->_commonBase = _commonBuffer;
	newThread->_staticBase = _commonBuffer + _modules[scriptModuleNumber].staticOffset;
	newThread->_moduleBase = _modules[scriptModuleNumber].moduleBase;
	newThread->_moduleBaseSize = _modules[scriptModuleNumber].moduleBaseSize;
	
	newThread->_strings = &_modules[scriptModuleNumber].strings;
	newThread->_voiceLUT = &_modules[scriptModuleNumber].voiceLUT;

	return newThread;
}

void Script::wakeUpActorThread(int waitType, void *threadObj) {
	ScriptThread *thread;
	ScriptThreadList::iterator threadIterator;

	for (threadIterator = _threadList.begin(); threadIterator != _threadList.end(); ++threadIterator) {
		thread = threadIterator.operator->();
		if ((thread->_flags & kTFlagWaiting) && (thread->_waitType == waitType) && (thread->_threadObj == threadObj)) {
			thread->_flags &= ~kTFlagWaiting;
		}
	}
}

void Script::wakeUpThreads(int waitType) {
	ScriptThread *thread;
	ScriptThreadList::iterator threadIterator;
	
	for (threadIterator = _threadList.begin(); threadIterator != _threadList.end(); ++threadIterator) {
		thread = threadIterator.operator->();
		if ((thread->_flags & kTFlagWaiting) && (thread->_waitType == waitType)) {
			thread->_flags &= ~kTFlagWaiting;
		}
	}
}

void Script::wakeUpThreadsDelayed(int waitType, int sleepTime) {
	ScriptThread *thread;
	ScriptThreadList::iterator threadIterator;

	for (threadIterator = _threadList.begin(); threadIterator != _threadList.end(); ++threadIterator) {
		thread = threadIterator.operator->();
		if ((thread->_flags & kTFlagWaiting) && (thread->_waitType == waitType)) {
			thread->_waitType = kWaitTypeDelay;
			thread->_sleepTime = sleepTime;
		}
	}
}

int Script::executeThreads(uint msec) {
	ScriptThread *thread;
	ScriptThreadList::iterator threadIterator;

	if (!isInitialized()) {
		return FAILURE;
	}

	threadIterator = _threadList.begin();

	while (threadIterator != _threadList.end()) {
		thread = threadIterator.operator->();

		if (thread->_flags & (kTFlagFinished | kTFlagAborted)) {
			if (thread->_flags & kTFlagFinished)
				setPointerVerb();
			
			threadIterator = _threadList.erase(threadIterator);
			continue;
		}

		if (thread->_flags & kTFlagWaiting) {
			
			if (thread->_waitType == kWaitTypeDelay) {
				if (thread->_sleepTime < msec) {
					thread->_sleepTime = 0;
				} else {
					thread->_sleepTime -= msec;
				}

				if (thread->_sleepTime == 0)
					thread->_flags &= ~kTFlagWaiting;			
			} else {
				if (thread->_waitType == kWaitTypeWalk) {
					ActorData *actor;
					actor = (ActorData *)thread->_threadObj;
					if (actor->currentAction == kActionWait) {
						thread->_flags &= ~kTFlagWaiting;			
					}
				}
			}
		}

		if (!(thread->_flags & kTFlagWaiting))
			runThread(thread, STHREAD_TIMESLICE);

		++threadIterator;
	}

	return SUCCESS;
}

void Script::completeThread(void) {
	for (int i = 0; i < 40 &&  !_threadList.isEmpty() ; i++)
		executeThreads(0);
}

int Script::SThreadDebugStep() {
	if (_dbg_singlestep) {
		_dbg_dostep = 1;
	}

	return SUCCESS;
}

void Script::runThread(ScriptThread *thread, uint instructionLimit) {
	const char*operandName;
	uint instructionCount;
	uint16 savedInstructionOffset;

	byte *addr;
	uint16 param1;
	uint16 param2;
	int16 iparam1;
	int16 iparam2;

	byte argumentsCount;
	uint16 functionNumber;
	int scriptFunctionReturnValue;
	ScriptFunctionType scriptFunction;

	int debug_print = 0;
	int operandChar;
	int i;

	// Handle debug single-stepping
	if ((thread == _dbg_thread) && _dbg_singlestep) {
		if (_dbg_dostep) {
			debug_print = 1;
			thread->_sleepTime = 0;
			instructionLimit = 1;
			_dbg_dostep = 0;
		} else {
			return;
		}
	}

	MemoryReadStream scriptS(thread->_moduleBase, thread->_moduleBaseSize);

	scriptS.seek(thread->_instructionOffset);

	for (instructionCount = 0; instructionCount < instructionLimit; instructionCount++) {
		if (thread->_flags & (kTFlagAsleep))
			break;

		savedInstructionOffset = thread->_instructionOffset;
		operandChar = scriptS.readByte();

#define CASEOP(opName)	case opName:									\
							if (operandChar == opName) {				\
								operandName = #opName;					\
							}
						
		debug(8, "Executing thread offset: %lu (%x) stack: %d", thread->_instructionOffset, operandChar, thread->pushedSize());
		operandName="";
		switch (operandChar) {
		CASEOP(opNextBlock)
			// Some sort of "jump to the start of the next memory
			// page" instruction, I think.
			thread->_instructionOffset = 1024 * ((thread->_instructionOffset / 1024) + 1);
			break;

// STACK INSTRUCTIONS
		CASEOP(opDup)
			thread->push(thread->stackTop());
			break;
		CASEOP(opDrop)
			thread->pop();
			break;
		CASEOP(opZero)
			thread->push(0);
			break;
		CASEOP(opOne)
			thread->push(1);
			break;
		CASEOP(opConstint)
		CASEOP(opStrlit)
			iparam1 = scriptS.readSint16LE();
			thread->push(iparam1);
			break;

// DATA INSTRUCTIONS  
		CASEOP(opGetFlag)
			addr = thread->baseAddress(scriptS.readByte());
			param1 = scriptS.readUint16LE();
			addr += (param1 >> 3);
			param1 = (1 << (param1 & 7));
			thread->push((*addr) & param1 ? 1 : 0);
			break;
		CASEOP(opGetInt)
			addr = thread->baseAddress(scriptS.readByte());
			iparam1 = scriptS.readSint16LE();
			addr += iparam1;
			thread->push(*((uint16*)addr));
			break;
		CASEOP(opPutFlag)
			addr = thread->baseAddress(scriptS.readByte());
			param1 = scriptS.readUint16LE();
			addr += (param1 >> 3);
			param1 = (1 << (param1 & 7));
			if (thread->stackTop()) {
				*addr |= param1;
			} else {
				*addr &= ~param1;
			}
			break;
		CASEOP(opPutInt)
			addr = thread->baseAddress(scriptS.readByte());
			iparam1 = scriptS.readSint16LE();
			addr += iparam1;
			*(uint16*)addr =  thread->stackTop();
			break;
		CASEOP(opPutFlagV)
			addr = thread->baseAddress(scriptS.readByte());
			param1 = scriptS.readUint16LE();
			addr += (param1 >> 3);
			param1 = (1 << (param1 & 7));
			if (thread->pop()) {
				*addr |= param1;
			} else {
				*addr &= ~param1;
			}
			break;
		CASEOP(opPutIntV)
			addr = thread->baseAddress(scriptS.readByte());
			iparam1 = scriptS.readSint16LE();
			addr += iparam1;
			*(uint16*)addr =  thread->pop();
			break;

// FUNCTION CALL INSTRUCTIONS    
		CASEOP(opCall)
			argumentsCount = scriptS.readByte();
			param1 = scriptS.readByte();
			if (param1 != kAddressModule) {
				error("Script::runThread param1 != kAddressModule");
			}
			addr = thread->baseAddress(param1);
			iparam1 = scriptS.readSint16LE();
			addr += iparam1;
			thread->push(argumentsCount);

			param2 = scriptS.pos();
			// NOTE: The original pushes the program
			// counter as a pointer here. But I don't think
			// we will have to do that.
			thread->push(param2);
			thread->_instructionOffset = iparam1;
			
			break;
		CASEOP(opCcall)
		CASEOP(opCcallV)
			argumentsCount = scriptS.readByte();
			functionNumber = scriptS.readUint16LE();
			if (functionNumber >= SCRIPT_FUNCTION_MAX) {
				error("Script::runThread() Invalid script function number");
			}

			debug(8, "Calling 0x%X %s", functionNumber, _scriptFunctionsList[functionNumber].scriptFunctionName);
			scriptFunction = _scriptFunctionsList[functionNumber].scriptFunction;
			scriptFunctionReturnValue = (this->*scriptFunction)(thread, argumentsCount);
			if (scriptFunctionReturnValue != SUCCESS) {		// TODO: scriptFunctionReturnValue should be ignored & removed
				_vm->_console->DebugPrintf(S_WARN_PREFIX "%X: Script function %d failed.\n", thread->_instructionOffset, scriptFunctionReturnValue);
			}

			if (functionNumber ==  16) { // SF_gotoScene
				instructionCount = instructionLimit; // break the loop
				break;
			}

			if (operandChar == opCcall) {// CALL function
				thread->push(thread->_returnValue);
			}

			if (thread->_flags & kTFlagAsleep)
				instructionCount = instructionLimit;	// break out of loop!
			break;
		CASEOP(opEnter)
			thread->push(thread->_frameIndex);
			thread->_frameIndex = thread->_stackTopIndex;
			thread->_stackTopIndex -= (scriptS.readSint16LE() / 2);
			break;
		CASEOP(opReturn)
			thread->_returnValue = thread->pop();
		CASEOP(opReturnV)
			thread->_stackTopIndex = thread->_frameIndex;
			thread->_frameIndex = thread->pop();
			if (thread->pushedSize() == 0) {
				thread->_flags |= kTFlagFinished;
				break;
			} else {
				thread->_instructionOffset = thread->pop();
				iparam1 = thread->pop();
				iparam1 += iparam1;
				while (iparam1--) {
					thread->pop();
				}

				if (operandChar == opReturn) {
					thread->push(thread->_returnValue);
				}
			}
			break;

// BRANCH INSTRUCTIONS    
		CASEOP(opJmp)
			param1 = scriptS.readUint16LE();
			thread->_instructionOffset = param1;
			break;
		CASEOP(opJmpTrueV)
			param1 = scriptS.readUint16LE();
			if (thread->pop()) {
				thread->_instructionOffset = param1;
			}
			break;
		CASEOP(opJmpFalseV)
			param1 = scriptS.readUint16LE();
			if (!thread->pop()) {
				thread->_instructionOffset = param1;
			}
			break;
		CASEOP(opJmpTrue)
			param1 = scriptS.readUint16LE();
			if (thread->stackTop()) {
				thread->_instructionOffset = param1;
			}
			break;
		CASEOP(opJmpFalse)
			param1 = scriptS.readUint16LE();
			if (!thread->stackTop()) {
				thread->_instructionOffset = param1;
			}
			break;
		CASEOP(opJmpSwitch)
			iparam1 = scriptS.readSint16LE();
			param1 = thread->pop();
			while (iparam1--) {
				param2 = scriptS.readUint16LE();
				thread->_instructionOffset = scriptS.readUint16LE();
				if (param2 == param1) {
					break;
				}
			}
			if (iparam1 < 0) {
				thread->_instructionOffset = scriptS.readUint16LE();
			}
			break;
		CASEOP(opJmpRandom)
			// Supposedly the number of possible branches.
			// The original interpreter ignores it.
			scriptS.readUint16LE();
			iparam1 = scriptS.readSint16LE();
			iparam1 = _vm->_rnd.getRandomNumber(iparam1 - 1);
			while (1) {
				iparam2 = scriptS.readSint16LE();
				thread->_instructionOffset = scriptS.readUint16LE();
				
				iparam1 -= iparam2;
				if (iparam1 < 0) {
					break;
				}
			}
			break;

// UNARY INSTRUCTIONS
		CASEOP(opNegate)
			thread->push(-thread->pop());
			break;
		CASEOP(opNot)
			thread->push(!thread->pop());
			break;
		CASEOP(opCompl)
			thread->push(~thread->pop());
			break;

		CASEOP(opIncV)
			addr = thread->baseAddress(scriptS.readByte());
			iparam1 = scriptS.readSint16LE();
			addr += iparam1;
			*(uint16*)addr += 1;
			break;
		CASEOP(opDecV)
			addr = thread->baseAddress(scriptS.readByte());
			iparam1 = scriptS.readSint16LE();
			addr += iparam1;
			*(uint16*)addr -= 1;
			break;
		CASEOP(opPostInc)
			addr = thread->baseAddress(scriptS.readByte());
			iparam1 = scriptS.readSint16LE();
			addr += iparam1;
			thread->push(*(int16*)addr);
			*(uint16*)addr += 1;
			break;
		CASEOP(opPostDec)
			addr = thread->baseAddress(scriptS.readByte());
			iparam1 = scriptS.readSint16LE();
			addr += iparam1;
			thread->push(*(int16*)addr);
			*(uint16*)addr -= 1;
			break;

// ARITHMETIC INSTRUCTIONS    
		CASEOP(opAdd)
			iparam2 = thread->pop();
			iparam1 = thread->pop();
			iparam1 += iparam2;
			thread->push(iparam1);
			break;
		CASEOP(opSub)
			iparam2 = thread->pop();
			iparam1 = thread->pop();
			iparam1 -= iparam2;
			thread->push(iparam1);
			break;
		CASEOP(opMul)
			iparam2 = thread->pop();
			iparam1 = thread->pop();
			iparam1 *= iparam2;
			thread->push(iparam1);
			break;
		CASEOP(opDiv)
			iparam2 = thread->pop();
			iparam1 = thread->pop();
			iparam1 /= iparam2;
			thread->push(iparam1);
			break;
		CASEOP(opMod)
			iparam2 = thread->pop();
			iparam1 = thread->pop();
			iparam1 %= iparam2;
			thread->push(iparam1);
			break;

// COMPARISION INSTRUCTIONS    
		CASEOP(opEq)
			iparam2 = thread->pop();
			iparam1 = thread->pop();
			thread->push((iparam1 == iparam2) ? 1 : 0);
			break;
		CASEOP(opNe)
			iparam2 = thread->pop();
			iparam1 = thread->pop();
			thread->push((iparam1 != iparam2) ? 1 : 0);
			break;
		CASEOP(opGt)
			iparam2 = thread->pop();
			iparam1 = thread->pop();
			thread->push((iparam1 > iparam2) ? 1 : 0);
			break;
		CASEOP(opLt)
			iparam2 = thread->pop();
			iparam1 = thread->pop();
			thread->push((iparam1 < iparam2) ? 1 : 0);
			break;
		CASEOP(opGe)
			iparam2 = thread->pop();
			iparam1 = thread->pop();
			thread->push((iparam1 >= iparam2) ? 1 : 0);
			break;
		CASEOP(opLe)
			iparam2 = thread->pop();
			iparam1 = thread->pop();
			thread->push((iparam1 <= iparam2) ? 1 : 0);
			break;

// SHIFT INSTRUCTIONS    
		CASEOP(opRsh)
			iparam2 = thread->pop();
			iparam1 = thread->pop();
			iparam1 >>= iparam2;
			thread->push(iparam1);
			break;
		CASEOP(opLsh)
			iparam2 = thread->pop();
			iparam1 = thread->pop();
			iparam1 <<= iparam2;
			thread->push(iparam1);
			break;

// BITWISE INSTRUCTIONS   
		CASEOP(opAnd)
			iparam2 = thread->pop();
			iparam1 = thread->pop();
			iparam1 &= iparam2;
			thread->push(iparam1);
			break;
		CASEOP(opOr)
			iparam2 = thread->pop();
			iparam1 = thread->pop();
			iparam1 |= iparam2;
			thread->push(iparam1);
			break;
		CASEOP(opXor)
			iparam2 = thread->pop();
			iparam1 = thread->pop();
			iparam1 ^= iparam2;
			thread->push(iparam1);
			break;

// LOGICAL INSTRUCTIONS     
		CASEOP(opLAnd)
			iparam2 = thread->pop();
			iparam1 = thread->pop();
			thread->push((iparam1 && iparam2) ? 1 : 0);
			break;
		CASEOP(opLOr)
			iparam2 = thread->pop();
			iparam1 = thread->pop();
			thread->push((iparam1 || iparam2) ? 1 : 0);
			break;
		CASEOP(opLXor)
			iparam2 = thread->pop();
			iparam1 = thread->pop();			
			thread->push(((iparam1 && !iparam2) || (!iparam1 && iparam2)) ? 1 : 0);
			break;

// GAME INSTRUCTIONS  
		CASEOP(opSpeak) {
				int stringsCount;
				uint16 actorId;
				uint16 speechFlags;
				int sampleResourceId = -1;
				int16 first;
				const char *strings[ACTOR_SPEECH_STRING_MAX];

				if (_vm->_actor->isSpeaking()) {
					thread->wait(kWaitTypeSpeech);
					return;
				}

				stringsCount = scriptS.readByte();
				actorId =  scriptS.readUint16LE();
				speechFlags = scriptS.readByte();
				scriptS.readUint16LE(); // x,y skip
				
				if (stringsCount == 0)
					error("opSpeak stringsCount == 0");

				if (stringsCount > ACTOR_SPEECH_STRING_MAX)
					error("opSpeak stringsCount=0x%X exceed ACTOR_SPEECH_STRING_MAX", stringsCount);
				
				iparam1 = first = thread->stackTop();
				for (i = 0; i < stringsCount; i++) {
					 iparam1 = thread->pop();
					 strings[i] = thread->_strings->getString(iparam1);
				}
				// now data contains last string index

				if (_vm->getGameId() == GID_ITE_DISK_G) { // special ITE dos
					if ((_vm->_scene->currentSceneNumber() == ITE_DEFAULT_SCENE) && 
						(iparam1 >= 288) && (iparam1 <= (RID_SCENE1_VOICE_138 - RID_SCENE1_VOICE_009 + 288))) {
						sampleResourceId = RID_SCENE1_VOICE_009 + iparam1 - 288;
					}
				} else {
					if (thread->_voiceLUT->voicesCount > first) {
						sampleResourceId = thread->_voiceLUT->voices[first];
					}
				}

				_vm->_actor->actorSpeech(actorId, strings, stringsCount, sampleResourceId, speechFlags);				

				if (!(speechFlags & kSpeakAsync)) {
					thread->wait(kWaitTypeSpeech);
				}
			}
			break;
		CASEOP(opDialogBegin)
			if (_conversingThread) {
				thread->wait(kWaitTypeDialogBegin);
				return;
			}
			_conversingThread = thread;
			_vm->_interface->converseClear();
			break;
		CASEOP(opDialogEnd)
			if (thread == _conversingThread) {
				_vm->_interface->activate();
				_vm->_interface->setMode(kPanelConverse);
				thread->wait(kWaitTypeDialogEnd);
			}
			break;
		CASEOP(opReply) {
				const char *str;
				byte replyNum;
				byte flags;
				replyNum = scriptS.readByte();
				flags = scriptS.readByte();
				param1 = 0;

				if (flags & kReplyOnce) {
					param1 = scriptS.readUint16LE();
					addr = thread->_staticBase + (param1 >> 3);
					if (*addr & (1 << (param1 & 7))) {
						break;
					}
				}

				str = thread->_strings->getString(thread->pop());
				if (_vm->_interface->converseAddText(str, replyNum, flags, param1))
					warning("Error adding ConverseText (%s, %d, %d, %d)", str, replyNum, flags, param1);
			}
			break;
		CASEOP(opAnimate)
			scriptS.readUint16LE();
			scriptS.readUint16LE();
			param1 = scriptS.readByte();
			thread->_instructionOffset += param1;
			break;

		default:
			error("Script::runThread() Invalid opcode encountered 0x%X", operandChar);
		}

		debug(8, operandName);
		_vm->_console->DebugPrintf("%s\n", operandName);

		
		if (thread->_flags & (kTFlagFinished | kTFlagAborted)) {
			_vm->_console->DebugPrintf("Script finished\n");			
			break;
		} else {

			// Set instruction offset only if a previous instruction didn't branch
			if (savedInstructionOffset == thread->_instructionOffset) {
				thread->_instructionOffset = scriptS.pos();
			} else {
				if (thread->_instructionOffset >= scriptS.size()) {
					error("Script::runThread() Out of range script execution");
				}

				scriptS.seek(thread->_instructionOffset);
			}
		}

	}
}

} // End of namespace Saga

