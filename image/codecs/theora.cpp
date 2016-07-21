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

#include <theora/theoradec.h>

#include "common/ptr.h"
#include "common/rect.h"
#include "common/stream.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "common/util.h"
#include "graphics/surface.h"
#include "graphics/yuv_to_rgb.h"
#include "image/codecs/codec.h"
#include "image/codecs/theora.h"

namespace Image {

class TheoraDecoder : public Codec {
public:
	TheoraDecoder();
	~TheoraDecoder();

	const Graphics::Surface *decodeFrame(Common::SeekableReadStream &stream);
	Graphics::PixelFormat getPixelFormat() const { return _pixelFormat; }

	bool parseExtraData(Common::SeekableReadStream &stream);
	bool parseExtraData(Common::SeekableReadStream &packet1, Common::SeekableReadStream &packet2, Common::SeekableReadStream &packet3);

private:
	Graphics::PixelFormat _pixelFormat;	
	Graphics::Surface _surface;
	Graphics::Surface _displaySurface;

	th_info _theoraInfo;
	th_comment _theoraComment;
	th_dec_ctx *_theoraDecode;
	th_setup_info *_theoraSetup;
	ogg_packet _packet;

	void initCommon();
};

TheoraDecoder::TheoraDecoder() {
	th_info_init(&_theoraInfo);
	th_comment_init(&_theoraComment);
	_theoraDecode = 0;
	_theoraSetup = 0;

	if (g_system->getScreenFormat().bytesPerPixel == 1)
		_pixelFormat = Graphics::PixelFormat(4, 8, 8, 8, 8, 8, 16, 24, 0);
	else
		_pixelFormat = g_system->getScreenFormat();
}

TheoraDecoder::~TheoraDecoder() {
	if (_theoraDecode)
		th_decode_free(_theoraDecode);

	th_setup_free(_theoraSetup);
	th_comment_clear(&_theoraComment);
	th_info_clear(&_theoraInfo);
}

enum TheoraYUVBuffers {
	kBufferY = 0,
	kBufferU = 1,
	kBufferV = 2
};

const Graphics::Surface *TheoraDecoder::decodeFrame(Common::SeekableReadStream &stream) {
	// Prepare a packet from the stream
	Common::ScopedArray<byte> data(new byte[stream.size()]);
	stream.read(data.get(), stream.size());

	ogg_packet packet;
	packet.packet = data.get();
	packet.bytes = stream.size();

	// Attempt to decode it
	int decodeResult = th_decode_packetin(_theoraDecode, &packet, 0);
	if (decodeResult != 0) {
		warning("Failed to decode theora frame: %d", decodeResult);
		return 0;
	}

	// Success! Convert to RGB
	th_ycbcr_buffer yuvBuffer;
	th_decode_ycbcr_out(_theoraDecode, yuvBuffer);

	switch (_theoraInfo.pixel_fmt) {
	case TH_PF_420:
		// Width and height of all buffers have to be divisible by 2.
		assert((yuvBuffer[kBufferY].width & 1) == 0);
		assert((yuvBuffer[kBufferY].height & 1) == 0);
		assert((yuvBuffer[kBufferU].width & 1) == 0);
		assert((yuvBuffer[kBufferV].width & 1) == 0);

		// UV images have to have a quarter of the Y image resolution
		assert(yuvBuffer[kBufferU].width == yuvBuffer[kBufferY].width >> 1);
		assert(yuvBuffer[kBufferV].width == yuvBuffer[kBufferY].width >> 1);
		assert(yuvBuffer[kBufferU].height == yuvBuffer[kBufferY].height >> 1);
		assert(yuvBuffer[kBufferV].height == yuvBuffer[kBufferY].height >> 1);

		YUVToRGBMan.convert420(&_surface, Graphics::YUVToRGBManager::kScaleITU, yuvBuffer[kBufferY].data, yuvBuffer[kBufferU].data, yuvBuffer[kBufferV].data, yuvBuffer[kBufferY].width, yuvBuffer[kBufferY].height, yuvBuffer[kBufferY].stride, yuvBuffer[kBufferU].stride);
		break;
	case TH_PF_422:
		// TODO
		break;
	case TH_PF_444:
		YUVToRGBMan.convert444(&_surface, Graphics::YUVToRGBManager::kScaleITU, yuvBuffer[kBufferY].data, yuvBuffer[kBufferU].data, yuvBuffer[kBufferV].data, yuvBuffer[kBufferY].width, yuvBuffer[kBufferY].height, yuvBuffer[kBufferY].stride, yuvBuffer[kBufferU].stride);
		break;
	default:
		break;
	}

	return &_displaySurface;
}

bool TheoraDecoder::parseExtraData(Common::SeekableReadStream &stream) {
	// TODO: This is similar enough with the vorbis one; should be merged
	// somehow.

	if (stream.size() < 3)
		return false;

	byte initialBytes[3];
	stream.read(initialBytes, sizeof(initialBytes));

	int headerSizes[3];
	Common::ScopedArray<byte> headers[3];

	if (stream.size() >= 6 && READ_BE_UINT16(initialBytes) == 42) {
		stream.seek(0);

		for (int i = 0; i < 3; i++) {
			headerSizes[i] = stream.readUint16BE();

			if (headerSizes[i] + stream.pos() > stream.size()) {
				warning("Theora header size invalid");
				return false;
			}

			headers[i].reset(new byte[headerSizes[i]]);
			stream.read(headers[i].get(), headerSizes[i]);
		}
	} else if (initialBytes[0] == 2 && stream.size() < 0x7FFFFE00) {
		stream.seek(1);
		uint32 offset = 1;

		for (int i = 0; i < 2; i++) {
			headerSizes[i] = 0;

			while (stream.pos() < stream.size()) {
				byte length = stream.readByte();
				headerSizes[i] += length;
				offset++;

				if (length != 0xFF)
					break;
			}

			if (offset >= (uint32)stream.size()) {
				warning("Theora header sizes damaged");
				return false;
			}
		}

		headerSizes[2] = stream.size() - (headerSizes[0] + headerSizes[1] + offset);
		stream.seek(offset);

		for (int i = 0; i < 3; i++) {
			headers[i].reset(new byte[headerSizes[i]]);
			stream.read(headers[i].get(), headerSizes[i]);
		}
	} else {
		warning("Invalid theora initial header length: %d", initialBytes[0]);
		return false;
	}

	for (int i = 0; i < 3; i++) {
		_packet.b_o_s = (i == 0);
		_packet.bytes = headerSizes[i];
		_packet.packet = headers[i].get();

		if (th_decode_headerin(&_theoraInfo, &_theoraComment, &_theoraSetup, &_packet) < 0) {
			warning("Theora header %d is damaged", i);
			return false;
		}
	}

	initCommon();

	return true;
}

bool TheoraDecoder::parseExtraData(Common::SeekableReadStream &packet1, Common::SeekableReadStream &packet2, Common::SeekableReadStream &packet3) {
	// TODO: This is similar enough with the vorbis one; should be merged
	// somehow.

	int headerSizes[3];
	Common::ScopedArray<byte> headers[3];

#define READ_WHOLE_STREAM(x) \
	do { \
		Common::SeekableReadStream &packet = packet##x; \
		headerSizes[x - 1] = packet.size(); \
		headers[x - 1].reset(new byte[headerSizes[x - 1]]); \
		packet.read(headers[x - 1].get(), headerSizes[x - 1]); \
	} while (0)

	READ_WHOLE_STREAM(1);
	READ_WHOLE_STREAM(2);
	READ_WHOLE_STREAM(3);

#undef READ_WHOLE_STREAM

	for (int i = 0; i < 3; i++) {
		_packet.b_o_s = (i == 0);
		_packet.bytes = headerSizes[i];
		_packet.packet = headers[i].get();

		if (th_decode_headerin(&_theoraInfo, &_theoraComment, &_theoraSetup, &_packet) < 0) {
			warning("Theora header %d is damaged", i);
			return false;
		}
	}

	initCommon();

	return true;
}

void TheoraDecoder::initCommon() {
	// Begin decode
	_theoraDecode = th_decode_alloc(&_theoraInfo, _theoraSetup);

	if (_theoraInfo.pixel_fmt != TH_PF_420 && _theoraInfo.pixel_fmt != TH_PF_444)
		warning("Unhandled theora pixel format: %d", _theoraInfo.pixel_fmt);

	// Set post-processing to the highest it can.
	// TODO: Make this configurable somehow.
	int postProcessingMax;
	th_decode_ctl(_theoraDecode, TH_DECCTL_GET_PPLEVEL_MAX, &postProcessingMax, sizeof(postProcessingMax));
	th_decode_ctl(_theoraDecode, TH_DECCTL_SET_PPLEVEL, &postProcessingMax, sizeof(postProcessingMax));

	// Create the main surface
	_surface.create(_theoraInfo.frame_width, _theoraInfo.frame_height, _pixelFormat);

	// Set up a display surface to take into account the x/y offset
	_displaySurface = _surface.getSubArea(Common::Rect(_theoraInfo.pic_x, _theoraInfo.pic_y, _theoraInfo.pic_x + _theoraInfo.pic_width, _theoraInfo.pic_y + _theoraInfo.pic_height));
}

Codec *makeTheoraDecoder(Common::SeekableReadStream &extraData) {
	Common::ScopedPtr<TheoraDecoder> decoder(new TheoraDecoder());
	if (!decoder->parseExtraData(extraData))
		return 0;

	return decoder.release();
}

Codec *makeTheoraDecoder(Common::SeekableReadStream &packet1, Common::SeekableReadStream &packet2, Common::SeekableReadStream &packet3) {
	Common::ScopedPtr<TheoraDecoder> decoder(new TheoraDecoder());
	if (!decoder->parseExtraData(packet1, packet2, packet3))
		return 0;

	return decoder.release();
}

} // End of namespace Image

