/* ScummVM - Scumm Interpreter
 * Copyright (C) 2004 The ScummVM project
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
#include "saga.h"
#include "reinherit.h"

#include "yslib.h"

/*
 * Uses the following modules:
\*--------------------------------------------------------------------------*/
#include "sound.h"
#include "game_mod.h"

namespace Saga {

/*
 * Begin module component
\*--------------------------------------------------------------------------*/

Sound::Sound(int enabled) {
	int result;

	/* Load sound module resource file contexts */
	result = GAME_GetFileContext(&_soundContext, R_GAME_SOUNDFILE, 0);
	if (result != R_SUCCESS) {
		return;
	}

	result = GAME_GetFileContext(&_voiceContext, R_GAME_VOICEFILE, 0);
	if (result != R_SUCCESS) {
		return;
	}

    /* Grab sound resource information for the current game */
    //GAME_GetSoundInfo(&SoundModule.snd_info);

	_soundInitialized = 1;
	return;
}

Sound::~Sound() {
	if (!_soundInitialized) {
		return;
	}

	_soundInitialized = 0;
}

int Sound::play(int sound_rn, int channel) {
	int resource_size;
	char *resource_data;

	if (!_soundInitialized) {
		return R_FAILURE;
	}

	if (channel > 3) {
		return R_FAILURE;
	}
	
	return R_SUCCESS;
}

int Sound::pause(int channel) {
	(void)channel;

	if (!_soundInitialized) {
		return R_FAILURE;
	}

	return R_SUCCESS;
}

int Sound::resume(int channel) {
	(void)channel;

	if (!_soundInitialized) {
		return R_FAILURE;
	}

	return R_SUCCESS;
}

int Sound::stop(int channel) {
	(void)channel;

	if (!_soundInitialized) {
		return R_FAILURE;
	}

	return R_SUCCESS;
}

int Sound::playVoice(R_SOUNDBUFFER *buf) {
	(void)buf;

	if (!_soundInitialized) {
		return R_FAILURE;
	}

	return R_SUCCESS;
}

int Sound::pauseVoice(void) {
	if (!_soundInitialized) {
		return R_FAILURE;
	}

	return R_SUCCESS;
}

int Sound::resumeVoice(void) {
	if (!_soundInitialized) {
		return R_FAILURE;
	}

	return R_SUCCESS;
}

int Sound::stopVoice(void) {
	if (!_soundInitialized) {
		return R_FAILURE;
	}

	return R_SUCCESS;
}

} // End of namespace Saga
