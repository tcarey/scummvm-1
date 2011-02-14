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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL: https://scummvm-misc.svn.sourceforge.net/svnroot/scummvm-misc/trunk/engines/tsage/events.cpp $
 * $Id: events.cpp 224 2011-02-10 10:58:52Z dreammaster $
 *
 */

#include "common/events.h"
#include "common/singleton.h"
#include "graphics/cursorman.h"
#include "common/system.h"

#include "tsage/events.h"
#include "tsage/core.h"
#include "tsage/staticres.h"
#include "tsage/tsage.h"
#include "tsage/globals.h"

namespace tSage {

EventsClass::EventsClass() { 
	_frameNumber = 0;
	_priorFrameTime = 0;
	_prevDelayFrame = 0;
	_saver->addListener(this);
}

bool EventsClass::pollEvent() {
	uint32 milli = g_system->getMillis();
	if ((milli - _priorFrameTime) >= GAME_FRAME_TIME) {
		_priorFrameTime = milli;
		++_frameNumber;

		g_system->updateScreen();
	}

	if (!g_system->getEventManager()->pollEvent(_event)) return false;

	// Handle keypress
	switch (_event.type) {
	case Common::EVENT_QUIT:
	case Common::EVENT_RTL:
		break;

	case Common::EVENT_MOUSEMOVE:
	case Common::EVENT_LBUTTONDOWN:
	case Common::EVENT_LBUTTONUP:
	case Common::EVENT_RBUTTONDOWN:
	case Common::EVENT_RBUTTONUP:
		// Keep a copy of the current mouse position
		_mousePos = _event.mouse;
		break;

	default:
 		break;
	}

	return true;
}

void EventsClass::waitForPress(int eventMask) {
	Event evt;
	while (!_vm->getEventManager()->shouldQuit() && !getEvent(evt, eventMask))
		g_system->delayMillis(10);
}

/**
 * Standard event retrieval, which only returns keyboard and mouse clicks
 */
bool EventsClass::getEvent(Event &evt, int eventMask) {
	while (pollEvent() && !_vm->getEventManager()->shouldQuit()) {
		evt.handled = false;
		evt.eventType = EVENT_NONE;
		evt.mousePos = _event.mouse;
		evt.kbd = _event.kbd;

		switch (_event.type) {
		case Common::EVENT_MOUSEMOVE:
			evt.eventType = EVENT_MOUSE_MOVE;
			break;
		case Common::EVENT_LBUTTONDOWN:
			evt.eventType = EVENT_BUTTON_DOWN;
			evt.btnState = BTNSHIFT_LEFT;
			break;
		case Common::EVENT_RBUTTONDOWN:
			evt.eventType = EVENT_BUTTON_DOWN;
			evt.btnState = BTNSHIFT_RIGHT;
			break;
		case Common::EVENT_MBUTTONDOWN:
			evt.eventType = EVENT_BUTTON_DOWN;
			evt.btnState = BTNSHIFT_MIDDLE;
			break;
		case Common::EVENT_LBUTTONUP:
		case Common::EVENT_RBUTTONUP:
		case Common::EVENT_MBUTTONUP:
			evt.eventType = EVENT_BUTTON_UP;
			evt.btnState = 0;
			break;
		case Common::EVENT_KEYDOWN:
			evt.eventType = EVENT_KEYPRESS;
			evt.kbd = _event.kbd;
			break;
		default:
			break;
		}

		if (evt.eventType & eventMask)
			return true;
	}

	return false;
}

/**
 * Sets the specified cursor
 *
 * @cursorType Specified cursor number
 */
void EventsClass::setCursor(CursorType cursorType) {
	_globals->clearFlag(122);
	
	if (cursorType != CURSOR_ARROW)
		_currentCursor = cursorType;
	if (!CursorMan.isVisible())
		showCursor();

	const byte *cursor;
	bool delFlag = true;
	uint size;

	switch (cursorType) {
	case CURSOR_CROSSHAIRS:
		// Crosshairs cursor
		cursor = _vm->_dataManager->getSubResource(4, 1, 6, &size);
		_globals->setFlag(122);
		break;
	
	case CURSOR_LOOK:
		// Look cursor
		cursor = _vm->_dataManager->getSubResource(4, 1, 5, &size);
		break;

	case CURSOR_USE:
		// Use cursor
		cursor = _vm->_dataManager->getSubResource(4, 1, 4, &size);
		break;

	case CURSOR_TALK:
		// Talk cursor
		cursor = _vm->_dataManager->getSubResource(4, 1, 3, &size);
		break;

	case CURSOR_ARROW:
		// Arrow cursor
		cursor = CURSOR_ARROW_DATA;
		delFlag = false;
		break;

	default:
		// Walk cursor
		cursor = CURSOR_WALK_DATA;
		delFlag = false;
		break;
	}

	// Decode the cursor
	GfxSurface s = surfaceFromRes(cursor);

	Graphics::Surface surface = s.lockSurface();
	const byte *cursorData = (const byte *)surface.getBasePtr(0, 0);
	CursorMan.replaceCursor(cursorData, surface.w, surface.h, s._centroid.x, s._centroid.y, s._transColour);
	s.unlockSurface();

	if (delFlag)
		DEALLOCATE(cursor);
}

void EventsClass::setCursor(Graphics::Surface &cursor, int transColour, const Common::Point &hotspot, CursorType cursorId) {
	const byte *cursorData = (const byte *)cursor.getBasePtr(0, 0);
	CursorMan.replaceCursor(cursorData, cursor.w, cursor.h, hotspot.x, hotspot.y, transColour);

	_currentCursor = cursorId;
}

void EventsClass::setCursorFromFlag() {
	setCursor(_globals->getFlag(122) ? CURSOR_CROSSHAIRS : _currentCursor);
}

void EventsClass::showCursor() {
	CursorMan.showMouse(true);
}

void EventsClass::hideCursor() {
	CursorMan.showMouse(false);
}

/**
 * Delays the game for the specified number of frames, if necessary, from the
 * previous time the delay method was called
 */
void EventsClass::delay(int numFrames) {
	while (_frameNumber < (_prevDelayFrame + numFrames)) {
		uint32 delayAmount = CLIP(_priorFrameTime + GAME_FRAME_TIME - g_system->getMillis(), 
			(uint32)0, (uint32)GAME_FRAME_TIME);
		if (delayAmount > 0)
			g_system->delayMillis(delayAmount);

		++_frameNumber;
		_priorFrameTime = g_system->getMillis();
	}

	g_system->updateScreen();
	_prevDelayFrame = _frameNumber;
	_priorFrameTime = g_system->getMillis();
}

} // end of namespace tSage
