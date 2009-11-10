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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/trunk/backends/platform/sdl/sdl.h $
 * $Id: sdl.h 44793 2009-10-08 19:41:38Z fingolfin $
 *
 */

#ifndef SDL_SAMSUNGTV_COMMON_H
#define SDL_SAMSUNGTV_COMMON_H

#include <SDL.h>

#include "backends/base-backend.h"
#include "backends/platform/sdl/sdl.h"
#include "graphics/scaler.h"

#if defined(SAMSUNGTV)

namespace Audio {
	class MixerImpl;
}

class OSystem_SDL_SamsungTV : public OSystem_SDL {
public:
	OSystem_SDL_SamsungTV();

	virtual void initBackend();

	// Highest supported
	virtual Common::List<Graphics::PixelFormat> getSupportedFormats();

	// Set the size and format of the video bitmap.
	// Typically, 320x200 CLUT8
	virtual void initSize(uint w, uint h, const Graphics::PixelFormat *format);

	// Warp the mouse cursor. Where set_mouse_pos() only informs the
	// backend of the mouse cursor's current position, this function
	// actually moves the cursor to the specified position.
	virtual void warpMouse(int x, int y);

	// Set the bitmap that's used when drawing the cursor.
	virtual void setMouseCursor(const byte *buf, uint w, uint h, int hotspot_x, int hotspot_y, uint32 keycolor, int cursorTargetScale, const Graphics::PixelFormat *format); // overloaded by CE backend (FIXME)

	// Get the next event.
	// Returns true if an event was retrieved.
	virtual bool pollEvent(Common::Event &event);

	// Define all hardware keys for keymapper
	virtual Common::HardwareKeySet *getHardwareKeySet();

	// Quit
	virtual void quit(); // overloaded by CE backend

	virtual int getDefaultGraphicsMode() const;

	virtual bool hasFeature(Feature f);
	virtual void setFeatureState(Feature f, bool enable);
	virtual bool getFeatureState(Feature f);

	virtual void addSysArchivesToSearchSet(Common::SearchSet &s, int priority = 0);

	virtual Common::SeekableReadStream *createConfigReadStream();
	virtual Common::WriteStream *createConfigWriteStream();

protected:

	SDL_Surface *_prehwscreen;

	virtual void drawMouse(); // overloaded by CE backend
	virtual void blitCursor(); // overloaded by CE backend (FIXME)

	virtual void internUpdateScreen(); // overloaded by CE backend

	virtual bool loadGFXMode(); // overloaded by CE backend
	virtual void unloadGFXMode(); // overloaded by CE backend
	virtual bool hotswapGFXMode(); // overloaded by CE backend

	void setFullscreenMode(bool enable);

	void handleKbdMouse();

	virtual bool remapKey(SDL_Event &ev, Common::Event &event);
};

#endif

#endif
