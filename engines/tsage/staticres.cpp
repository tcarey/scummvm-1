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
 * $URL: https://scummvm-misc.svn.sourceforge.net/svnroot/scummvm-misc/trunk/engines/tsage/staticres.cpp $
 * $Id: staticres.cpp 219 2011-02-08 12:05:46Z dreammaster $
 *
 */

#include "tsage/staticres.h"

namespace tSage {

const byte CURSOR_ARROW_DATA[] = {
	15, 0, 15, 0, 0, 0, 0, 0, 9, 0, 
	0x00, 0x00, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
	0x00, 0x00, 0x00, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 
	0x00, 0xFF, 0x00, 0x00, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 
	0x00, 0xFF,	0xFF, 0x00, 0x00, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 
	0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 
	0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
	0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
	0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	0xFF, 0x00, 0x00, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
	0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x09, 0x09, 0x09, 0x09, 0x09,
	0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x09, 0x09, 0x09, 0x09,
	0x00, 0xFF, 0xFF, 0x00, 0x00, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
	0x00, 0xFF, 0x00, 0x00, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,	0x09, 0x09, 0x09, 0x09, 0x09,
	0x00, 0x00, 0x00, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
	0x00, 0x00, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,	0x09, 0x09, 0x09,
	0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
	0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09
};

const byte CURSOR_WALK_DATA[] = {
	15, 0, 15, 0, 7, 0, 7, 0, 9, 0,
	0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x00, 0xFF, 0x00, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
	0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x00, 0xFF, 0x00, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
	0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x00, 0xFF, 0x00, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
	0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x00, 0xFF, 0x00, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
	0x09, 0x09, 0x09, 0x09,	0x09, 0x09, 0x00, 0xFF, 0x00, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
	0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x09,	0x09, 0x09, 0x09, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x09, 0x09, 0x09, 0x09, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x09, 0x09,	0x09, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
	0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x00, 0xFF, 0x00, 0x09,	0x09, 0x09, 0x09, 0x09, 0x09,
	0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x00, 0xFF, 0x00, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
	0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x00, 0xFF, 0x00, 0x09, 0x09, 0x09,	0x09, 0x09, 0x09,
	0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x00, 0xFF, 0x00, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
	0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x00, 0xFF, 0x00, 0x09, 0x09, 0x09, 0x09, 0x09,	0x09,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09
};

const char *LOOK_SCENE_HOTSPOT = "You see nothing special.";
const char *USE_SCENE_HOTSPOT = "That accomplishes nothing.";
const char *TALK_SCENE_HOTSPOT = "Yak, yak.";
const char *SPECIAL_SCENE_HOTSPOT = "That is a unique use for that.";
const char *DEFAULT_SCENE_HOTSPOT = "That accomplishes nothing.";
const char *SAVE_ERROR_MSG = "Error occurred saving game. Please do not try to restore this game!";
const char *SAVING_NOT_ALLOWED_MSG = "Saving is not allowed at this time.";
const char *RESTORING_NOT_ALLOWED_MSG = "Restoring is not allowed at this time.";
const char *RESTART_CONFIRM_MSG = "Do you want to restart your game?";
const char *WATCH_INTRO_MSG = "Do you wish to watch the introduction?";
const char *INV_EMPTY_MSG = "You have nothing in your possesion.";

const char *HELP_MSG = "Ringworld\rRevenge of the Patriarch\x14\rScummVM Version\r\r\
\x01 Keyboard shortcuts...\rF2 - Sound options\rF3 - Quit\r\
F4 - Restart\rF5 - Save game\rF7 - Restore Game\rF10 - Pause game";
const char *QUIT_CONFIRM_MSG = "Do you want to quit playing this game?";
const char *RESTART_MSG = "Do you want to restart this game?";
const char *GAME_PAUSED_MSG = "Game is paused.";
const char *OPTIONS_MSG = "\x01Options...";
const char *OK_BTN_STRING = " Ok ";
const char *CANCEL_BTN_STRING = "Cancel";
const char *QUIT_BTN_STRING = " Quit ";
const char *RESTART_BTN_STRING = "Restart";
const char *SAVE_BTN_STRING = "Save";
const char *RESTORE_BTN_STRING = "Restore";
const char *SOUND_BTN_STRING = "Sound";
const char *RESUME_BTN_STRING = " Resume \rplay";
const char *LOOK_BTN_STRING = "Look";
const char *PICK_BTN_STRING = "Pick";
const char *START_PLAY_BTN_STRING = " Start Play ";
const char *INTRODUCTION_BTN_STRING = "Introduction";


} // End of namespace tSage
