/* Cabal - Legacy Game Implementations
 *
 * Cabal is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

// Based on the ScummVM (GPLv2+) file of the same name

#ifndef BACKENDS_AUDIOCD_SDL_H
#define BACKENDS_AUDIOCD_SDL_H

#include "backends/audiocd/default/default-audiocd.h"

#include "backends/platform/sdl/sdl-sys.h"

#if !SDL_VERSION_ATLEAST(2, 0, 0)

/**
 * The SDL audio cd manager. Implements real audio cd playback.
 */
class SdlAudioCDManager : public DefaultAudioCDManager {
public:
	SdlAudioCDManager();
	virtual ~SdlAudioCDManager();

	virtual bool open();
	virtual void close();
	virtual bool play(int track, int numLoops, int startFrame, int duration, bool onlyEmulate = false);
	virtual void stop();
	virtual bool isPlaying() const;
	virtual void update();

protected:
	virtual bool openCD(int drive);

	SDL_CD *_cdrom;
	int _cdTrack, _cdNumLoops, _cdStartFrame, _cdDuration;
	uint32 _cdEndTime, _cdStopTime;
};

#endif // !SDL_VERSION_ATLEAST(2, 0, 0)

#endif
