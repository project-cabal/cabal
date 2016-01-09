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

#include "audio/mixer.h"
#include "audio/softsynth/pcspk.h"
#include "audio/null.h"

namespace Audio {

PCSpeaker::PCSpeaker() {
	_playForever = false;
	_oscLength = 0;
	_oscSamples = 0;
	_remainingSamples = 0;
	_mixedSamples = 0;
	_volume = 255;
}

PCSpeaker::~PCSpeaker() {
}

void PCSpeaker::play(int freq, int32 length) {
	Common::StackLock lock(_mutex);

	_oscLength = getRate() / freq;
	_oscSamples = 0;
	if (length == -1) {
		_remainingSamples = 1;
		_playForever = true;
	} else {
		_remainingSamples = (getRate() * length) / 1000;
		_playForever = false;
	}
	_mixedSamples = 0;
}

void PCSpeaker::stop(int32 delay) {
	Common::StackLock lock(_mutex);

	_remainingSamples = (getRate() * delay) / 1000;
	_playForever = false;
}

void PCSpeaker::setVolume(byte volume) {
	_volume = volume;
}

bool PCSpeaker::isPlaying() const {
	return _remainingSamples != 0;
}

int PCSpeaker::readBuffer(int16 *buffer, const int numSamples) {
	Common::StackLock lock(_mutex);

	int i;

	for (i = 0; _remainingSamples && (i < numSamples); i++) {
		buffer[i] = generateWave(_oscSamples, _oscLength) * _volume;
		if (_oscSamples++ >= _oscLength)
			_oscSamples = 0;
		if (!_playForever)
			_remainingSamples--;
		_mixedSamples++;
	}

	// Clear the rest of the buffer
	if (i < numSamples)
		memset(buffer + i, 0, (numSamples - i) * sizeof(int16));

	return numSamples;
}

int PCSpeaker::getRate() const {
	return g_system->getMixer()->getOutputRate();
}

int8 PCSpeaker::generateWave(uint32 x, uint32 oscLength) {
	return (x < (oscLength / 2)) ? 127 : -128;
}

} // End of namespace Audio


//	Plugin interface
//	(This can only create a null driver since pc speaker support is not part of the
//	midi driver architecture. But we need the plugin for the options menu in the launcher
//	and for MidiDriver::detectDevice() which is more or less used by all engines.)

class PCSpeakerMusicPlugin : public NullMusicPlugin {
public:
	const char *getName() const {
		return _s("PC Speaker Emulator");
	}

	const char *getId() const {
		return "pcspk";
	}

	MusicDevices getDevices() const;
};

MusicDevices PCSpeakerMusicPlugin::getDevices() const {
	MusicDevices devices;
	devices.push_back(MusicDevice(this, "", MT_PCSPK));
	return devices;
}

class PCjrMusicPlugin : public NullMusicPlugin {
public:
	const char *getName() const {
		return _s("IBM PCjr Emulator");
	}

	const char *getId() const {
		return "pcjr";
	}

	MusicDevices getDevices() const;
};

MusicDevices PCjrMusicPlugin::getDevices() const {
	MusicDevices devices;
	devices.push_back(MusicDevice(this, "", MT_PCJR));
	return devices;
}

//#if PLUGIN_ENABLED_DYNAMIC(PCSPK)
	//REGISTER_PLUGIN_DYNAMIC(PCSPK, PLUGIN_TYPE_MUSIC, PCSpeakerMusicPlugin);
//#else
	REGISTER_PLUGIN_STATIC(PCSPK, PLUGIN_TYPE_MUSIC, PCSpeakerMusicPlugin);
//#endif

//#if PLUGIN_ENABLED_DYNAMIC(PCJR)
	//REGISTER_PLUGIN_DYNAMIC(PCJR, PLUGIN_TYPE_MUSIC, PCjrMusicPlugin);
//#else
	REGISTER_PLUGIN_STATIC(PCJR, PLUGIN_TYPE_MUSIC, PCjrMusicPlugin);
//#endif
