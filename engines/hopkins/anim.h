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

#ifndef HOPKINS_ANIM_H
#define HOPKINS_ANIM_H

#include "common/scummsys.h"
#include "common/endian.h"
#include "common/str.h"
#include "graphics/surface.h"

namespace Hopkins {

class HopkinsEngine;

class AnimationManager {
private:
	HopkinsEngine *_vm;

	int loadSpriteBank(int idx, const Common::String &filename);
	void searchAnim(const byte *data, int animIndex, int count);

public:
	bool _clearAnimationFl;

	AnimationManager(HopkinsEngine *vm);

	void loadAnim(const Common::String &animName);
	void clearAnim();
	void playAnim(const Common::String &filename, uint32 rate1, uint32 rate2, uint32 rate3, bool skipSeqFl = false);
	void playAnim2(const Common::String &filename, uint32 rate1, uint32 rate2, uint32 rate3);
	void playSequence(const Common::String &file, uint32 rate1, uint32 rate2, uint32 rate3, bool skipEscFl, bool skipSeqFl, bool noColFl = false);
	void playSequence2(const Common::String &file, uint32 rate1, uint32 rate2, uint32 rate3, bool skipSeqFl = false);
};

} // End of namespace Hopkins

#endif /* HOPKINS_ANIM_H */
