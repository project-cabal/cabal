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

#include "graphics/thumbnail.h"
#include "graphics/scaler.h"
#include "graphics/colormasks.h"
#include "common/endian.h"
#include "common/algorithm.h"
#include "common/system.h"
#include "common/stream.h"
#include "common/textconsole.h"

namespace Graphics {

namespace {
#define THMB_VERSION 2

struct ThumbnailHeader {
	uint32 type;
	uint32 size;
	byte version;
	uint16 width, height;
	PixelFormat format;
};

#define ThumbnailHeaderSize (4+4+1+2+2+(1+4+4))

enum HeaderState {
	/// There is no header present
	kHeaderNone,
	/// The header present only has reliable values for version and size
	kHeaderUnsupported,
	/// The header is present and the version is supported
	kHeaderPresent
};

HeaderState loadHeader(Common::SeekableReadStream &in, ThumbnailHeader &header, bool outputWarnings) {
	header.type = in.readUint32BE();
	// We also accept the bad 'BMHT' header here, for the sake of compatibility
	// with some older savegames which were written incorrectly due to a bug in
	// ScummVM which wrote the thumb header type incorrectly on LE systems.
	if (header.type != MKTAG('T','H','M','B') && header.type != MKTAG('B','M','H','T')) {
		if (outputWarnings)
			warning("couldn't find thumbnail header type");
		return kHeaderNone;
	}

	header.size = in.readUint32BE();
	header.version = in.readByte();

	// Do a check whether any read errors had occured. If so we cannot use the
	// values obtained for size and version because they might be bad.
	if (in.err() || in.eos()) {
		// TODO: We fake that there is no header. This is actually not quite
		// correct since we found the start of the header and then things
		// started to break. Right no we leave detection of this to the client.
		// Since this case is caused by broken files, the client code should
		// catch it anyway... If there is a nicer solution here, we should
		// implement it.
		return kHeaderNone;
	}

	if (header.version > THMB_VERSION) {
		if (outputWarnings)
			warning("trying to load a newer thumbnail version: %d instead of %d", header.version, THMB_VERSION);
		return kHeaderUnsupported;
	}

	header.width = in.readUint16BE();
	header.height = in.readUint16BE();
	header.format.bytesPerPixel = in.readByte();
	// Starting from version 2 on we serialize the whole PixelFormat.
	if (header.version >= 2) {
		header.format.rLoss = in.readByte();
		header.format.gLoss = in.readByte();
		header.format.bLoss = in.readByte();
		header.format.aLoss = in.readByte();

		header.format.rShift = in.readByte();
		header.format.gShift = in.readByte();
		header.format.bShift = in.readByte();
		header.format.aShift = in.readByte();
	} else {
		// Version 1 used a hardcoded RGB565.
		header.format = createPixelFormat<565>();
	}

	if (in.err() || in.eos()) {
		// When we reached this point we know that at least the size and
		// version field was loaded successfully, thus we tell this header
		// is not supported and silently hope that the client code is
		// prepared to handle read errors.
		return kHeaderUnsupported;
	} else {
		return kHeaderPresent;
	}
}
} // end of anonymous namespace

bool checkThumbnailHeader(Common::SeekableReadStream &in) {
	uint32 position = in.pos();
	ThumbnailHeader header;

	// TODO: It is not clear whether this is the best semantics. Now
	// checkThumbnailHeader will return true even when the thumbnail header
	// found is actually not usable. However, most engines seem to use this
	// to detect the presence of any header and if there is none it wont even
	// try to skip it. Thus, this looks like the best solution for now...
	bool hasHeader = (loadHeader(in, header, false) != kHeaderNone);

	in.seek(position, SEEK_SET);

	return hasHeader;
}

bool skipThumbnail(Common::SeekableReadStream &in) {
	uint32 position = in.pos();
	ThumbnailHeader header;

	// We can skip unsupported and supported headers. So we only seek back
	// to the old position in case there is no header at all.
	if (loadHeader(in, header, false) == kHeaderNone) {
		in.seek(position, SEEK_SET);
		return false;
	}

	in.seek(header.size - (in.pos() - position), SEEK_CUR);
	return true;
}

Graphics::Surface *loadThumbnail(Common::SeekableReadStream &in) {
	const uint32 position = in.pos();
	ThumbnailHeader header;
	HeaderState headerState = loadHeader(in, header, true);

	// Try to handle unsupported/broken headers gracefully. If there is no
	// header at all, we seek back and return at this point. If there is an
	// unsupported/broken header, we skip the actual data and return. The
	// downside is that we might reset the end of stream flag with this and
	// the client code would not be able to notice a read past the end of the
	// stream at this point then.
	if (headerState == kHeaderNone) {
		in.seek(position, SEEK_SET);
		return 0;
	} else if (headerState == kHeaderUnsupported) {
		in.seek(header.size - (in.pos() - position), SEEK_CUR);
		return 0;
	}

	if (header.format.bytesPerPixel != 2 && header.format.bytesPerPixel != 4) {
		warning("trying to load thumbnail with unsupported bit depth %d", header.format.bytesPerPixel);
		return 0;
	}

	Graphics::Surface *const to = new Graphics::Surface(header.width, header.height, header.format);

	for (int y = 0; y < to->getHeight(); ++y) {
		if (header.format.bytesPerPixel == 2) {
			uint16 *pixels = (uint16 *)to->getBasePtr(0, y);
			for (uint x = 0; x < to->getWidth(); ++x) {
				*pixels++ = in.readUint16BE();
			}
		} else {
			uint32 *pixels = (uint32 *)to->getBasePtr(0, y);
			for (uint x = 0; x < to->getWidth(); ++x) {
				*pixels++ = in.readUint32BE();
			}
		}
	}
	return to;
}

bool saveThumbnail(Common::WriteStream &out) {
	Graphics::Surface thumb;

	if (!createThumbnailFromScreen(&thumb)) {
		warning("Couldn't create thumbnail from screen, aborting thumbnail save");
		return false;
	}

	return saveThumbnail(out, thumb);
}

bool saveThumbnail(Common::WriteStream &out, const Graphics::Surface &thumb) {
	if (thumb.getFormat().bytesPerPixel != 2 && thumb.getFormat().bytesPerPixel != 4) {
		warning("trying to save thumbnail with bpp %u", thumb.getFormat().bytesPerPixel);
		return false;
	}

	ThumbnailHeader header;
	header.type = MKTAG('T','H','M','B');
	header.size = ThumbnailHeaderSize + thumb.getWidth() * thumb.getHeight() * thumb.getFormat().bytesPerPixel;
	header.version = THMB_VERSION;
	header.width = thumb.getWidth();
	header.height = thumb.getHeight();

	out.writeUint32BE(header.type);
	out.writeUint32BE(header.size);
	out.writeByte(header.version);
	out.writeUint16BE(header.width);
	out.writeUint16BE(header.height);

	// Serialize the PixelFormat
	out.writeByte(thumb.getFormat().bytesPerPixel);
	out.writeByte(thumb.getFormat().rLoss);
	out.writeByte(thumb.getFormat().gLoss);
	out.writeByte(thumb.getFormat().bLoss);
	out.writeByte(thumb.getFormat().aLoss);
	out.writeByte(thumb.getFormat().rShift);
	out.writeByte(thumb.getFormat().gShift);
	out.writeByte(thumb.getFormat().bShift);
	out.writeByte(thumb.getFormat().aShift);

	// Serialize the pixel data
	for (uint y = 0; y < thumb.getHeight(); ++y) {
		if (thumb.getFormat().bytesPerPixel == 2) {
			const uint16 *pixels = (const uint16 *)thumb.getBasePtr(0, y);
			for (uint x = 0; x < thumb.getWidth(); ++x) {
				out.writeUint16BE(*pixels++);
			}
		} else {
			const uint32 *pixels = (const uint32 *)thumb.getBasePtr(0, y);
			for (uint x = 0; x < thumb.getWidth(); ++x) {
				out.writeUint32BE(*pixels++);
			}
		}
	}

	return true;
}


/**
 * Returns an array indicating which pixels of a source image horizontally or vertically get
 * included in a scaled image
 */
int *scaleLine(int size, int srcSize) {
	int scale = 100 * size / srcSize;
	assert(scale > 0);
	int *v = new int[size];
	Common::fill(v, &v[size], 0);

	int distCtr = 0;
	int *destP = v;
	for (int distIndex = 0; distIndex < srcSize; ++distIndex) {
		distCtr += scale;
		while (distCtr >= 100) {
			assert(destP < &v[size]);
			*destP++ = distIndex;
			distCtr -= 100;
		}
	}

	return v;
}

Graphics::Surface *scale(const Graphics::Surface &srcImage, int xSize, int ySize) {
	Graphics::Surface *s = new Graphics::Surface(xSize, ySize, srcImage.getFormat());

	int *horizUsage = scaleLine(xSize, srcImage.getWidth());
	int *vertUsage = scaleLine(ySize, srcImage.getHeight());

	// Loop to create scaled version
	for (int yp = 0; yp < ySize; ++yp) {
		const byte *srcP = (const byte *)srcImage.getBasePtr(0, vertUsage[yp]);
		byte *destP = (byte *)s->getBasePtr(0, yp);

		for (int xp = 0; xp < xSize; ++xp) {
			const byte *tempSrcP = srcP + (horizUsage[xp] * srcImage.getFormat().bytesPerPixel);
			for (int byteCtr = 0; byteCtr < srcImage.getFormat().bytesPerPixel; ++byteCtr) {
				*destP++ = *tempSrcP++;
			}
		}
	}

	// Delete arrays and return surface
	delete[] horizUsage;
	delete[] vertUsage;
	return s;
}

} // End of namespace Graphics
