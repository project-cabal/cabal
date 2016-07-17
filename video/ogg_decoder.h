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

#include "common/scummsys.h"	// for USE_THEORADEC

#ifdef USE_THEORADEC

#ifndef VIDEO_OGG_DECODER_H
#define VIDEO_OGG_DECODER_H

#include "common/rational.h"
#include "video/video_decoder.h"
#include "audio/mixer.h"
#include "graphics/surface.h"

#include <theora/theoradec.h>

#ifdef USE_VORBIS
#ifdef USE_TREMOR
#include <tremor/ivorbiscodec.h>
#else
#include <vorbis/codec.h>
#endif
#endif

namespace Common {
class SeekableReadStream;
}

namespace Audio {
class AudioStream;
class QueuingAudioStream;
}

namespace Video {

/**
 *
 * Decoder for ogg videos.
 * Video decoder used in engines:
 *  - pegasus
 *  - sword25
 *  - wintermute
 */
class OggDecoder : public VideoDecoder {
public:
	OggDecoder(Audio::Mixer::SoundType soundType = Audio::Mixer::kMusicSoundType);
	virtual ~OggDecoder();

	/**
	 * Load a video file
	 * @param stream  the stream to load
	 */
	bool loadStream(Common::SeekableReadStream *stream);
	void close();

protected:
	void readNextPacket();

private:
	class TheoraVideoTrack : public VideoTrack {
	public:
		TheoraVideoTrack(const Graphics::PixelFormat &format, th_info &theoraInfo, th_setup_info *theoraSetup);
		~TheoraVideoTrack();

		bool endOfTrack() const { return _endOfVideo; }
		uint16 getWidth() const { return _displaySurface.getWidth(); }
		uint16 getHeight() const { return _displaySurface.getHeight(); }
		Graphics::PixelFormat getPixelFormat() const { return _displaySurface.getFormat(); }
		int getCurFrame() const { return _curFrame; }
		Common::Timestamp getNextFrameStartTime() const { return (uint32)(_nextFrameStartTime * 1000); }
		const Graphics::Surface *decodeNextFrame() { return &_displaySurface; }

		bool decodePacket(ogg_packet &oggPacket);
		void setEndOfVideo() { _endOfVideo = true; }

	private:
		int _curFrame;
		bool _endOfVideo;
		Common::Rational _frameRate;
		double _nextFrameStartTime;

		Graphics::Surface _surface;
		Graphics::Surface _displaySurface;

		th_dec_ctx *_theoraDecode;

		void translateYUVtoRGBA(th_ycbcr_buffer &YUVBuffer);
	};

#ifdef USE_VORBIS
	class VorbisAudioTrack : public AudioTrack {
	public:
		VorbisAudioTrack(Audio::Mixer::SoundType soundType, vorbis_info &vorbisInfo);
		~VorbisAudioTrack();

		Audio::Mixer::SoundType getSoundType() const { return _soundType; }

		bool decodeSamples();
		bool hasAudio() const;
		bool needsAudio() const;
		void synthesizePacket(ogg_packet &oggPacket);
		void setEndOfAudio() { _endOfAudio = true; }

	protected:
		Audio::AudioStream *getAudioStream() const;

	private:
		// single audio fragment audio buffering
		int _audioBufferFill;
		ogg_int16_t *_audioBuffer;

		Audio::Mixer::SoundType _soundType;
		Audio::QueuingAudioStream *_audStream;

		vorbis_block _vorbisBlock;
		vorbis_dsp_state _vorbisDSP;

		bool _endOfAudio;
	};
#endif

	void queuePage(ogg_page *page);
	int bufferData();
	bool queueAudio();
	void ensureAudioBufferSize();

	Common::SeekableReadStream *_fileStream;

	Audio::Mixer::SoundType _soundType;

	ogg_sync_state _oggSync;
	ogg_page _oggPage;
	ogg_packet _oggPacket;

	ogg_stream_state _theoraOut, _vorbisOut;
	bool _hasVideo, _hasAudio;

	TheoraVideoTrack *_videoTrack;

#ifdef USE_VORBIS
	vorbis_info _vorbisInfo;
	VorbisAudioTrack *_audioTrack;
#endif
};

} // End of namespace Video

#endif

#endif
