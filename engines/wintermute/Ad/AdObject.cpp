/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

/*
 * This file is based on WME Lite.
 * http://dead-code.org/redir.php?target=wmelite
 * Copyright (c) 2011 Jan Nedoma
 */

#include "engines/wintermute/dcgf.h"
#include "engines/wintermute/Ad/AdGame.h"
#include "engines/wintermute/Ad/AdItem.h"
#include "engines/wintermute/Ad/AdObject.h"
#include "engines/wintermute/Ad/AdInventory.h"
#include "engines/wintermute/Ad/AdLayer.h"
#include "engines/wintermute/Ad/AdScene.h"
#include "engines/wintermute/Ad/AdSceneNode.h"
#include "engines/wintermute/Ad/AdSentence.h"
#include "engines/wintermute/Ad/AdWaypointGroup.h"
#include "engines/wintermute/Base/BGame.h"
#include "engines/wintermute/Base/BFrame.h"
#include "engines/wintermute/Base/BSound.h"
#include "engines/wintermute/Base/BSurfaceStorage.h"
#include "engines/wintermute/Base/BSubFrame.h"
#include "engines/wintermute/Base/BFont.h"
#include "engines/wintermute/Base/BFontStorage.h"
#include "engines/wintermute/Base/BSprite.h"
#include "engines/wintermute/Base/BStringTable.h"
#include "engines/wintermute/Base/scriptables/ScEngine.h"
#include "engines/wintermute/Base/scriptables/ScScript.h"
#include "engines/wintermute/Base/scriptables/ScStack.h"
#include "engines/wintermute/Base/scriptables/ScValue.h"
#include "common/str.h"
#include "common/util.h"

namespace WinterMute {

IMPLEMENT_PERSISTENT(CAdObject, false)

//////////////////////////////////////////////////////////////////////////
CAdObject::CAdObject(CBGame *inGame): CBObject(inGame) {
	_type = OBJECT_NONE;
	_state = _nextState = STATE_NONE;

	_active = true;
	_drawn = false;

	_currentSprite = NULL;
	_animSprite = NULL;
	_tempSprite2 = NULL;

	_font = NULL;

	_sentence = NULL;

	_forcedTalkAnimName = NULL;
	_forcedTalkAnimUsed = false;

	_blockRegion = NULL;
	_wptGroup = NULL;

	_currentBlockRegion = NULL;
	_currentWptGroup = NULL;

	_ignoreItems = false;
	_sceneIndependent = false;

	_stickRegion = NULL;

	_subtitlesModRelative = true;
	_subtitlesModX = 0;
	_subtitlesModY = 0;
	_subtitlesWidth = 0;
	_subtitlesModXCenter = true;

	_inventory = NULL;

	for (int i = 0; i < MAX_NUM_REGIONS; i++) _currentRegions[i] = NULL;

	_partEmitter = NULL;
	_partFollowParent = false;
	_partOffsetX = _partOffsetY = 0;

	_registerAlias = this;
}


//////////////////////////////////////////////////////////////////////////
CAdObject::~CAdObject() {
	_currentSprite = NULL; // reference only, don't delete
	delete _animSprite;
	_animSprite = NULL;
	delete _sentence;
	_sentence = NULL;
	delete[] _forcedTalkAnimName;
	_forcedTalkAnimName = NULL;

	delete _blockRegion;
	_blockRegion = NULL;
	delete _wptGroup;
	_wptGroup = NULL;

	delete _currentBlockRegion;
	_currentBlockRegion = NULL;
	delete _currentWptGroup;
	_currentWptGroup = NULL;

	_tempSprite2 = NULL; // reference only
	_stickRegion = NULL;

	if (_font) Game->_fontStorage->removeFont(_font);

	if (_inventory) {
		((CAdGame *)Game)->unregisterInventory(_inventory);
		_inventory = NULL;
	}

	if (_partEmitter)
		Game->unregisterObject(_partEmitter);


	for (int i = 0; i < _attachmentsPre.GetSize(); i++) {
		Game->unregisterObject(_attachmentsPre[i]);
	}
	_attachmentsPre.RemoveAll();

	for (int i = 0; i < _attachmentsPost.GetSize(); i++) {
		Game->unregisterObject(_attachmentsPost[i]);
	}
	_attachmentsPost.RemoveAll();
}


//////////////////////////////////////////////////////////////////////////
HRESULT CAdObject::playAnim(const char *filename) {
	delete _animSprite;
	_animSprite = NULL;
	_animSprite = new CBSprite(Game, this);
	if (!_animSprite) {
		Game->LOG(0, "CAdObject::PlayAnim: error creating temp sprite (object:\"%s\" sprite:\"%s\")", _name, filename);
		return E_FAIL;
	}
	HRESULT res = _animSprite->loadFile(filename);
	if (FAILED(res)) {
		Game->LOG(res, "CAdObject::PlayAnim: error loading temp sprite (object:\"%s\" sprite:\"%s\")", _name, filename);
		delete _animSprite;
		_animSprite = NULL;
		return res;
	}
	_state = STATE_PLAYING_ANIM;

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CAdObject::display() {
	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CAdObject::update() {
	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
// high level scripting interface
//////////////////////////////////////////////////////////////////////////
HRESULT CAdObject::scCallMethod(CScScript *script, CScStack *stack, CScStack *thisStack, const char *name) {

	//////////////////////////////////////////////////////////////////////////
	// PlayAnim / PlayAnimAsync
	//////////////////////////////////////////////////////////////////////////
	if (strcmp(name, "PlayAnim") == 0 || strcmp(name, "PlayAnimAsync") == 0) {
		stack->correctParams(1);
		if (FAILED(playAnim(stack->pop()->getString()))) stack->pushBool(false);
		else {
			if (strcmp(name, "PlayAnimAsync") != 0) script->WaitFor(this);
			stack->pushBool(true);
		}
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// Reset
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Reset") == 0) {
		stack->correctParams(0);
		reset();
		stack->pushNULL();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// IsTalking
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "IsTalking") == 0) {
		stack->correctParams(0);
		stack->pushBool(_state == STATE_TALKING);
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// StopTalk / StopTalking
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "StopTalk") == 0 || strcmp(name, "StopTalking") == 0) {
		stack->correctParams(0);
		if (_sentence) _sentence->finish();
		if (_state == STATE_TALKING) {
			_state = _nextState;
			_nextState = STATE_READY;
			stack->pushBool(true);
		} else stack->pushBool(false);
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// ForceTalkAnim
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "ForceTalkAnim") == 0) {
		stack->correctParams(1);
		const char *animName = stack->pop()->getString();
		delete[] _forcedTalkAnimName;
		_forcedTalkAnimName = new char[strlen(animName) + 1];
		strcpy(_forcedTalkAnimName, animName);
		_forcedTalkAnimUsed = false;
		stack->pushBool(true);
		return S_OK;
	}


	//////////////////////////////////////////////////////////////////////////
	// Talk / TalkAsync
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Talk") == 0 || strcmp(name, "TalkAsync") == 0) {
		stack->correctParams(5);

		const char *text    = stack->pop()->getString();
		CScValue *soundVal = stack->pop();
		int duration  = stack->pop()->getInt();
		CScValue *valStances = stack->pop();

		const char *stances = valStances->isNULL() ? NULL : valStances->getString();

		int align = 0;
		CScValue *val = stack->pop();
		if (val->isNULL()) align = TAL_CENTER;
		else align = val->getInt();

		align = MIN(MAX(0, align), NUM_TEXT_ALIGN - 1);

		const char *sound = soundVal->isNULL() ? NULL : soundVal->getString();

		talk(text, sound, duration, stances, (TTextAlign)align);
		if (strcmp(name, "TalkAsync") != 0) script->WaitForExclusive(this);

		stack->pushNULL();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// StickToRegion
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "StickToRegion") == 0) {
		stack->correctParams(1);

		CAdLayer *main = ((CAdGame *)Game)->_scene->_mainLayer;
		bool regFound = false;

		int i;
		CScValue *val = stack->pop();
		if (val->isNULL() || !main) {
			_stickRegion = NULL;
			regFound = true;
		} else if (val->isString()) {
			const char *regionName = val->getString();
			for (i = 0; i < main->_nodes.GetSize(); i++) {
				if (main->_nodes[i]->_type == OBJECT_REGION && main->_nodes[i]->_region->_name && scumm_stricmp(main->_nodes[i]->_region->_name, regionName) == 0) {
					_stickRegion = main->_nodes[i]->_region;
					regFound = true;
					break;
				}
			}
		} else if (val->isNative()) {
			CBScriptable *obj = val->getNative();

			for (i = 0; i < main->_nodes.GetSize(); i++) {
				if (main->_nodes[i]->_type == OBJECT_REGION && main->_nodes[i]->_region == obj) {
					_stickRegion = main->_nodes[i]->_region;
					regFound = true;
					break;
				}
			}

		}

		if (!regFound) _stickRegion = NULL;
		stack->pushBool(regFound);
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SetFont
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SetFont") == 0) {
		stack->correctParams(1);
		CScValue *val = stack->pop();

		if (val->isNULL()) SetFont(NULL);
		else SetFont(val->getString());

		stack->pushNULL();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// GetFont
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "GetFont") == 0) {
		stack->correctParams(0);
		if (_font && _font->_filename) stack->pushString(_font->_filename);
		else stack->pushNULL();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// TakeItem
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "TakeItem") == 0) {
		stack->correctParams(2);

		if (!_inventory) {
			_inventory = new CAdInventory(Game);
			((CAdGame *)Game)->registerInventory(_inventory);
		}

		CScValue *val = stack->pop();
		if (!val->isNULL()) {
			const char *itemName = val->getString();
			val = stack->pop();
			const char *insertAfter = val->isNULL() ? NULL : val->getString();
			if (FAILED(_inventory->insertItem(itemName, insertAfter))) script->RuntimeError("Cannot add item '%s' to inventory", itemName);
			else {
				// hide associated entities
				((CAdGame *)Game)->_scene->handleItemAssociations(itemName, false);
			}

		} else script->RuntimeError("TakeItem: item name expected");

		stack->pushNULL();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// DropItem
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "DropItem") == 0) {
		stack->correctParams(1);

		if (!_inventory) {
			_inventory = new CAdInventory(Game);
			((CAdGame *)Game)->registerInventory(_inventory);
		}

		CScValue *val = stack->pop();
		if (!val->isNULL()) {
			if (FAILED(_inventory->removeItem(val->getString()))) script->RuntimeError("Cannot remove item '%s' from inventory", val->getString());
			else {
				// show associated entities
				((CAdGame *)Game)->_scene->handleItemAssociations(val->getString(), true);
			}
		} else script->RuntimeError("DropItem: item name expected");

		stack->pushNULL();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// GetItem
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "GetItem") == 0) {
		stack->correctParams(1);

		if (!_inventory) {
			_inventory = new CAdInventory(Game);
			((CAdGame *)Game)->registerInventory(_inventory);
		}

		CScValue *val = stack->pop();
		if (val->_type == VAL_STRING) {
			CAdItem *item = ((CAdGame *)Game)->getItemByName(val->getString());
			if (item) stack->pushNative(item, true);
			else stack->pushNULL();
		} else if (val->isNULL() || val->getInt() < 0 || val->getInt() >= _inventory->_takenItems.GetSize())
			stack->pushNULL();
		else
			stack->pushNative(_inventory->_takenItems[val->getInt()], true);

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// HasItem
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "HasItem") == 0) {
		stack->correctParams(1);

		if (!_inventory) {
			_inventory = new CAdInventory(Game);
			((CAdGame *)Game)->registerInventory(_inventory);
		}

		CScValue *val = stack->pop();
		if (!val->isNULL()) {
			for (int i = 0; i < _inventory->_takenItems.GetSize(); i++) {
				if (val->getNative() == _inventory->_takenItems[i]) {
					stack->pushBool(true);
					return S_OK;
				} else if (scumm_stricmp(val->getString(), _inventory->_takenItems[i]->_name) == 0) {
					stack->pushBool(true);
					return S_OK;
				}
			}
		} else script->RuntimeError("HasItem: item name expected");

		stack->pushBool(false);
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// CreateParticleEmitter
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "CreateParticleEmitter") == 0) {
		stack->correctParams(3);
		bool followParent = stack->pop()->getBool();
		int offsetX = stack->pop()->getInt();
		int offsetY = stack->pop()->getInt();

		CPartEmitter *emitter = createParticleEmitter(followParent, offsetX, offsetY);
		if (emitter) stack->pushNative(_partEmitter, true);
		else stack->pushNULL();

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// DeleteParticleEmitter
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "DeleteParticleEmitter") == 0) {
		stack->correctParams(0);
		if (_partEmitter) {
			Game->unregisterObject(_partEmitter);
			_partEmitter = NULL;
		}
		stack->pushNULL();

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// AddAttachment
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "AddAttachment") == 0) {
		stack->correctParams(4);
		const char *filename = stack->pop()->getString();
		bool preDisplay = stack->pop()->getBool(true);
		int offsetX = stack->pop()->getInt();
		int offsetY = stack->pop()->getInt();

		HRESULT res;
		CAdEntity *ent = new CAdEntity(Game);
		if (FAILED(res = ent->loadFile(filename))) {
			delete ent;
			ent = NULL;
			script->RuntimeError("AddAttachment() failed loading entity '%s'", filename);
			stack->pushBool(false);
		} else {
			Game->registerObject(ent);

			ent->_posX = offsetX;
			ent->_posY = offsetY;
			ent->_active = true;

			if (preDisplay) _attachmentsPre.Add(ent);
			else _attachmentsPost.Add(ent);

			stack->pushBool(true);
		}

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// RemoveAttachment
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "RemoveAttachment") == 0) {
		stack->correctParams(1);
		CScValue *val = stack->pop();
		bool found = false;
		if (val->isNative()) {
			CBScriptable *obj = val->getNative();
			for (int i = 0; i < _attachmentsPre.GetSize(); i++) {
				if (_attachmentsPre[i] == obj) {
					found = true;
					Game->unregisterObject(_attachmentsPre[i]);
					_attachmentsPre.RemoveAt(i);
					i--;
				}
			}
			for (int i = 0; i < _attachmentsPost.GetSize(); i++) {
				if (_attachmentsPost[i] == obj) {
					found = true;
					Game->unregisterObject(_attachmentsPost[i]);
					_attachmentsPost.RemoveAt(i);
					i--;
				}
			}
		} else {
			const char *attachmentName = val->getString();
			for (int i = 0; i < _attachmentsPre.GetSize(); i++) {
				if (_attachmentsPre[i]->_name && scumm_stricmp(_attachmentsPre[i]->_name, attachmentName) == 0) {
					found = true;
					Game->unregisterObject(_attachmentsPre[i]);
					_attachmentsPre.RemoveAt(i);
					i--;
				}
			}
			for (int i = 0; i < _attachmentsPost.GetSize(); i++) {
				if (_attachmentsPost[i]->_name && scumm_stricmp(_attachmentsPost[i]->_name, attachmentName) == 0) {
					found = true;
					Game->unregisterObject(_attachmentsPost[i]);
					_attachmentsPost.RemoveAt(i);
					i--;
				}
			}
		}
		stack->pushBool(found);

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// GetAttachment
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "GetAttachment") == 0) {
		stack->correctParams(1);
		CScValue *val = stack->pop();

		CAdObject *ret = NULL;
		if (val->isInt()) {
			int index = val->getInt();
			int currIndex = 0;
			for (int i = 0; i < _attachmentsPre.GetSize(); i++) {
				if (currIndex == index) ret = _attachmentsPre[i];
				currIndex++;
			}
			for (int i = 0; i < _attachmentsPost.GetSize(); i++) {
				if (currIndex == index) ret = _attachmentsPost[i];
				currIndex++;
			}
		} else {
			const char *attachmentName = val->getString();
			for (int i = 0; i < _attachmentsPre.GetSize(); i++) {
				if (_attachmentsPre[i]->_name && scumm_stricmp(_attachmentsPre[i]->_name, attachmentName) == 0) {
					ret = _attachmentsPre[i];
					break;
				}
			}
			if (!ret) {
				for (int i = 0; i < _attachmentsPost.GetSize(); i++) {
					if (_attachmentsPost[i]->_name && scumm_stricmp(_attachmentsPost[i]->_name, attachmentName) == 0) {
						ret = _attachmentsPre[i];
						break;
					}
				}
			}
		}

		if (ret != NULL) stack->pushNative(ret, true);
		else stack->pushNULL();

		return S_OK;
	}

	else return CBObject::scCallMethod(script, stack, thisStack, name);
}


//////////////////////////////////////////////////////////////////////////
CScValue *CAdObject::scGetProperty(const char *name) {
	_scValue->setNULL();

	//////////////////////////////////////////////////////////////////////////
	// Type
	//////////////////////////////////////////////////////////////////////////
	if (strcmp(name, "Type") == 0) {
		_scValue->setString("object");
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// Active
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Active") == 0) {
		_scValue->setBool(_active);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// IgnoreItems
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "IgnoreItems") == 0) {
		_scValue->setBool(_ignoreItems);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// SceneIndependent
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SceneIndependent") == 0) {
		_scValue->setBool(_sceneIndependent);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// SubtitlesWidth
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SubtitlesWidth") == 0) {
		_scValue->setInt(_subtitlesWidth);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// SubtitlesPosRelative
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SubtitlesPosRelative") == 0) {
		_scValue->setBool(_subtitlesModRelative);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// SubtitlesPosX
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SubtitlesPosX") == 0) {
		_scValue->setInt(_subtitlesModX);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// SubtitlesPosY
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SubtitlesPosY") == 0) {
		_scValue->setInt(_subtitlesModY);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// SubtitlesPosXCenter
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SubtitlesPosXCenter") == 0) {
		_scValue->setBool(_subtitlesModXCenter);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// NumItems (RO)
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "NumItems") == 0) {
		_scValue->setInt(getInventory()->_takenItems.GetSize());
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// ParticleEmitter (RO)
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "ParticleEmitter") == 0) {
		if (_partEmitter) _scValue->setNative(_partEmitter, true);
		else _scValue->setNULL();

		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// NumAttachments (RO)
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "NumAttachments") == 0) {
		_scValue->setInt(_attachmentsPre.GetSize() + _attachmentsPost.GetSize());
		return _scValue;
	}


	else return CBObject::scGetProperty(name);
}


//////////////////////////////////////////////////////////////////////////
HRESULT CAdObject::scSetProperty(const char *name, CScValue *value) {

	//////////////////////////////////////////////////////////////////////////
	// Active
	//////////////////////////////////////////////////////////////////////////
	if (strcmp(name, "Active") == 0) {
		_active = value->getBool();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// IgnoreItems
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "IgnoreItems") == 0) {
		_ignoreItems = value->getBool();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SceneIndependent
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SceneIndependent") == 0) {
		_sceneIndependent = value->getBool();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SubtitlesWidth
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SubtitlesWidth") == 0) {
		_subtitlesWidth = value->getInt();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SubtitlesPosRelative
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SubtitlesPosRelative") == 0) {
		_subtitlesModRelative = value->getBool();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SubtitlesPosX
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SubtitlesPosX") == 0) {
		_subtitlesModX = value->getInt();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SubtitlesPosY
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SubtitlesPosY") == 0) {
		_subtitlesModY = value->getInt();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// SubtitlesPosXCenter
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "SubtitlesPosXCenter") == 0) {
		_subtitlesModXCenter = value->getBool();
		return S_OK;
	}

	else return CBObject::scSetProperty(name, value);
}


//////////////////////////////////////////////////////////////////////////
const char *CAdObject::scToString() {
	return "[ad object]";
}


//////////////////////////////////////////////////////////////////////////
HRESULT CAdObject::SetFont(const char *filename) {
	if (_font) Game->_fontStorage->removeFont(_font);
	if (filename) {
		_font = Game->_fontStorage->addFont(filename);
		return _font == NULL ? E_FAIL : S_OK;
	} else {
		_font = NULL;
		return S_OK;
	}
}


//////////////////////////////////////////////////////////////////////////
int CAdObject::getHeight() {
	if (!_currentSprite) return 0;
	else {
		CBFrame *frame = _currentSprite->_frames[_currentSprite->_currentFrame];
		int ret = 0;
		for (int i = 0; i < frame->_subframes.GetSize(); i++) {
			ret = MAX(ret, frame->_subframes[i]->_hotspotY);
		}

		if (_zoomable) {
			float zoom = ((CAdGame *)Game)->_scene->getZoomAt(_posX, _posY);
			ret = (int)(ret * zoom / 100);
		}
		return ret;
	}
}


//////////////////////////////////////////////////////////////////////////
void CAdObject::talk(const char *text, const char *sound, uint32 duration, const char *stances, TTextAlign Align) {
	if (!_sentence) _sentence = new CAdSentence(Game);
	if (!_sentence) return;

	if (_forcedTalkAnimName && _forcedTalkAnimUsed) {
		delete[] _forcedTalkAnimName;
		_forcedTalkAnimName = NULL;
		_forcedTalkAnimUsed = false;
	}

	delete(_sentence->_sound);
	_sentence->_sound = NULL;

	_sentence->setText(text);
	Game->_stringTable->expand(&_sentence->_text);
	_sentence->setStances(stances);
	_sentence->_duration = duration;
	_sentence->_align = Align;
	_sentence->_startTime = Game->_timer;
	_sentence->_currentStance = -1;
	_sentence->_font = _font == NULL ? Game->_systemFont : _font;
	_sentence->_freezable = _freezable;

	// try to locate speech file automatically
	bool deleteSound = false;
	if (!sound) {
		char *key = Game->_stringTable->getKey(text);
		if (key) {
			sound = ((CAdGame *)Game)->findSpeechFile(key);
			delete [] key;

			if (sound) deleteSound = true;
		}
	}

	// load sound and set duration appropriately
	if (sound) {
		CBSound *snd = new CBSound(Game);
		if (snd && SUCCEEDED(snd->setSound(sound, SOUND_SPEECH, true))) {
			_sentence->setSound(snd);
			if (_sentence->_duration <= 0) {
				uint32 Length = snd->getLength();
				if (Length != 0) _sentence->_duration = Length;
			}
		} else delete snd;
	}

	// set duration by text length
	if (_sentence->_duration <= 0) {// TODO: Avoid longs.
		_sentence->_duration = MAX((size_t)1000, Game->_subtitlesSpeed * strlen(_sentence->_text));
	}


	int x, y, width, height;

	x = _posX;
	y = _posY;

	if (!_sceneIndependent && _subtitlesModRelative) {
		x -= ((CAdGame *)Game)->_scene->getOffsetLeft();
		y -= ((CAdGame *)Game)->_scene->getOffsetTop();
	}


	if (_subtitlesWidth > 0) width = _subtitlesWidth;
	else {
		if ((x < Game->_renderer->_width / 4 || x > Game->_renderer->_width * 0.75) && !Game->_touchInterface) {
			width = MAX(Game->_renderer->_width / 4, MIN(x * 2, (Game->_renderer->_width - x) * 2));
		} else width = Game->_renderer->_width / 2;
	}

	height = _sentence->_font->getTextHeight((byte *)_sentence->_text, width);

	y = y - height - getHeight() - 5;
	if (_subtitlesModRelative) {
		x += _subtitlesModX;
		y += _subtitlesModY;
	} else {
		x = _subtitlesModX;
		y = _subtitlesModY;
	}
	if (_subtitlesModXCenter)
		x = x - width / 2;


	x = MIN(MAX(0, x), Game->_renderer->_width - width);
	y = MIN(MAX(0, y), Game->_renderer->_height - height);

	_sentence->_width = width;


	_sentence->_pos.x = x;
	_sentence->_pos.y = y;


	if (_subtitlesModRelative) {
		_sentence->_pos.x += ((CAdGame *)Game)->_scene->getOffsetLeft();
		_sentence->_pos.y += ((CAdGame *)Game)->_scene->getOffsetTop();
	}

	_sentence->_fixedPos = !_subtitlesModRelative;


	_sentence->setupTalkFile(sound);

	_state = STATE_TALKING;

	if (deleteSound) delete [] sound;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CAdObject::reset() {
	if (_state == STATE_PLAYING_ANIM && _animSprite != NULL) {
		delete _animSprite;
		_animSprite = NULL;
	} else if (_state == STATE_TALKING && _sentence) {
		_sentence->finish();
	}

	_state = _nextState = STATE_READY;

	Game->_scEngine->ResetObject(this);

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CAdObject::persist(CBPersistMgr *persistMgr) {
	CBObject::persist(persistMgr);

	persistMgr->transfer(TMEMBER(_active));
	persistMgr->transfer(TMEMBER(_blockRegion));
	persistMgr->transfer(TMEMBER(_currentBlockRegion));
	persistMgr->transfer(TMEMBER(_currentWptGroup));
	persistMgr->transfer(TMEMBER(_currentSprite));
	persistMgr->transfer(TMEMBER(_drawn));
	persistMgr->transfer(TMEMBER(_font));
	persistMgr->transfer(TMEMBER(_ignoreItems));
	persistMgr->transfer(TMEMBER_INT(_nextState));
	persistMgr->transfer(TMEMBER(_sentence));
	persistMgr->transfer(TMEMBER_INT(_state));
	persistMgr->transfer(TMEMBER(_animSprite));
	persistMgr->transfer(TMEMBER(_sceneIndependent));
	persistMgr->transfer(TMEMBER(_forcedTalkAnimName));
	persistMgr->transfer(TMEMBER(_forcedTalkAnimUsed));
	persistMgr->transfer(TMEMBER(_tempSprite2));
	persistMgr->transfer(TMEMBER_INT(_type));
	persistMgr->transfer(TMEMBER(_wptGroup));
	persistMgr->transfer(TMEMBER(_stickRegion));
	persistMgr->transfer(TMEMBER(_subtitlesModRelative));
	persistMgr->transfer(TMEMBER(_subtitlesModX));
	persistMgr->transfer(TMEMBER(_subtitlesModY));
	persistMgr->transfer(TMEMBER(_subtitlesModXCenter));
	persistMgr->transfer(TMEMBER(_subtitlesWidth));
	persistMgr->transfer(TMEMBER(_inventory));
	persistMgr->transfer(TMEMBER(_partEmitter));

	for (int i = 0; i < MAX_NUM_REGIONS; i++) persistMgr->transfer(TMEMBER(_currentRegions[i]));

	_attachmentsPre.persist(persistMgr);
	_attachmentsPost.persist(persistMgr);
	persistMgr->transfer(TMEMBER(_registerAlias));

	persistMgr->transfer(TMEMBER(_partFollowParent));
	persistMgr->transfer(TMEMBER(_partOffsetX));
	persistMgr->transfer(TMEMBER(_partOffsetY));

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CAdObject::updateSounds() {
	if (_sentence && _sentence->_sound)
		updateOneSound(_sentence->_sound);

	return CBObject::updateSounds();
}


//////////////////////////////////////////////////////////////////////////
HRESULT CAdObject::resetSoundPan() {
	if (_sentence && _sentence->_sound) {
		_sentence->_sound->setPan(0.0f);
	}
	return CBObject::resetSoundPan();
}


//////////////////////////////////////////////////////////////////////////
bool CAdObject::getExtendedFlag(const char *flagName) {
	if (!flagName) return false;
	else if (strcmp(flagName, "usable") == 0) return true;

	else return CBObject::getExtendedFlag(flagName);
}


//////////////////////////////////////////////////////////////////////////
HRESULT CAdObject::saveAsText(CBDynBuffer *buffer, int indent) {
	if (_blockRegion) _blockRegion->saveAsText(buffer, indent + 2, "BLOCKED_REGION");
	if (_wptGroup) _wptGroup->saveAsText(buffer, indent + 2);

	CBBase::saveAsText(buffer, indent + 2);

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CAdObject::updateBlockRegion() {
	CAdGame *adGame = (CAdGame *)Game;
	if (adGame->_scene) {
		if (_blockRegion && _currentBlockRegion)
			_currentBlockRegion->mimic(_blockRegion, _zoomable ? adGame->_scene->getScaleAt(_posY) : 100.0f, _posX, _posY);

		if (_wptGroup && _currentWptGroup)
			_currentWptGroup->mimic(_wptGroup, _zoomable ? adGame->_scene->getScaleAt(_posY) : 100.0f, _posX, _posY);
	}
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
CAdInventory *CAdObject::getInventory() {
	if (!_inventory) {
		_inventory = new CAdInventory(Game);
		((CAdGame *)Game)->registerInventory(_inventory);
	}
	return _inventory;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CAdObject::afterMove() {
	CAdRegion *newRegions[MAX_NUM_REGIONS];

	((CAdGame *)Game)->_scene->getRegionsAt(_posX, _posY, newRegions, MAX_NUM_REGIONS);
	for (int i = 0; i < MAX_NUM_REGIONS; i++) {
		if (!newRegions[i]) break;
		bool regFound = false;
		for (int j = 0; j < MAX_NUM_REGIONS; j++) {
			if (_currentRegions[j] == newRegions[i]) {
				_currentRegions[j] = NULL;
				regFound = true;
				break;
			}
		}
		if (!regFound) newRegions[i]->applyEvent("ActorEntry");
	}

	for (int i = 0; i < MAX_NUM_REGIONS; i++) {
		if (_currentRegions[i] && Game->validObject(_currentRegions[i])) {
			_currentRegions[i]->applyEvent("ActorLeave");
		}
		_currentRegions[i] = newRegions[i];
	}

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
HRESULT CAdObject::invalidateCurrRegions() {
	for (int i = 0; i < MAX_NUM_REGIONS; i++) _currentRegions[i] = NULL;
	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CAdObject::getScale(float *scaleX, float *scaleY) {
	if (_zoomable) {
		if (_scaleX >= 0 || _scaleY >= 0) {
			*scaleX = _scaleX < 0 ? 100 : _scaleX;
			*scaleY = _scaleY < 0 ? 100 : _scaleY;
		} else if (_scale >= 0) *scaleX = *scaleY = _scale;
		else *scaleX = *scaleY = ((CAdGame *)Game)->_scene->getZoomAt(_posX, _posY) + _relativeScale;
	} else {
		*scaleX = *scaleY = 100;
	}
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
HRESULT CAdObject::updateSpriteAttachments() {
	for (int i = 0; i < _attachmentsPre.GetSize(); i++) {
		_attachmentsPre[i]->update();
	}
	for (int i = 0; i < _attachmentsPost.GetSize(); i++) {
		_attachmentsPost[i]->update();
	}
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
HRESULT CAdObject::displaySpriteAttachments(bool preDisplay) {
	if (preDisplay) {
		for (int i = 0; i < _attachmentsPre.GetSize(); i++) {
			displaySpriteAttachment(_attachmentsPre[i]);
		}
	} else {
		for (int i = 0; i < _attachmentsPost.GetSize(); i++) {
			displaySpriteAttachment(_attachmentsPost[i]);
		}
	}
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
HRESULT CAdObject::displaySpriteAttachment(CAdObject *attachment) {
	if (!attachment->_active) return S_OK;

	float scaleX, scaleY;
	getScale(&scaleX, &scaleY);

	int origX = attachment->_posX;
	int origY = attachment->_posY;

	// inherit position from owner
	attachment->_posX = (int)(this->_posX + attachment->_posX * scaleX / 100.0f);
	attachment->_posY = (int)(this->_posY + attachment->_posY * scaleY / 100.0f);

	// inherit other props
	attachment->_alphaColor = this->_alphaColor;
	attachment->_blendMode = this->_blendMode;

	attachment->_scale = this->_scale;
	attachment->_relativeScale = this->_relativeScale;
	attachment->_scaleX = this->_scaleX;
	attachment->_scaleY = this->_scaleY;

	attachment->_rotate = this->_rotate;
	attachment->_relativeRotate = this->_relativeRotate;
	attachment->_rotateValid = this->_rotateValid;

	attachment->_registerAlias = this;
	attachment->_registrable = this->_registrable;

	HRESULT ret = attachment->display();

	attachment->_posX = origX;
	attachment->_posY = origY;

	return ret;
}

//////////////////////////////////////////////////////////////////////////
CPartEmitter *CAdObject::createParticleEmitter(bool followParent, int offsetX, int offsetY) {
	_partFollowParent = followParent;
	_partOffsetX = offsetX;
	_partOffsetY = offsetY;

	if (!_partEmitter) {
		_partEmitter = new CPartEmitter(Game, this);
		if (_partEmitter) {
			Game->registerObject(_partEmitter);
		}
	}
	updatePartEmitter();
	return _partEmitter;
}

//////////////////////////////////////////////////////////////////////////
HRESULT CAdObject::updatePartEmitter() {
	if (!_partEmitter) return E_FAIL;

	if (_partFollowParent) {
		float scaleX, scaleY;
		getScale(&scaleX, &scaleY);

		_partEmitter->_posX = (int)(_posX + (scaleX / 100.0f) * _partOffsetX);
		_partEmitter->_posY = (int)(_posY + (scaleY / 100.0f) * _partOffsetY);
	}
	return _partEmitter->update();
}

} // end of namespace WinterMute
