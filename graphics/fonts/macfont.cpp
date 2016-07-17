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

#include "common/stream.h"
#include "common/textconsole.h"
#include "graphics/surface.h"
#include "graphics/fonts/macfont.h"

namespace Graphics {

// Based entirely on https://developer.apple.com/legacy/library/documentation/mac/Text/Text-250.html

MacFont::MacFont() {
	_bitImage = 0;
	close();
}

MacFont::~MacFont() {
	close();
}

void MacFont::close() {
	_rectHeight = 0;
	_maxWidth = 0;
	_firstChar = 0;
	_lastChar = 0;
	_glyphs.clear();
	delete[] _bitImage;
	_bitImage = 0;
	_defaultChar.clear();
}

bool MacFont::loadFont(Common::SeekableReadStream &stream) {
	uint16 fontType = stream.readUint16BE();
	_firstChar = stream.readUint16BE();
	_lastChar = stream.readUint16BE();
	_maxWidth = stream.readUint16BE();
	int16 maxKerning = stream.readSint16BE();
	int16 negatedDescent = stream.readSint16BE();
	/* uint16 rectWidth = */ stream.readUint16BE();
	_rectHeight = stream.readUint16BE();
	uint32 widthOffsetOffset = stream.readUint16BE();
	/* uint16 maxAscent = */ stream.readUint16BE();
	/* uint16 maxDescent = */ stream.readUint16BE();
	/* uint16 leading = */ stream.readUint16BE();
	_bitImageRowWidth = stream.readUint16BE() * 2;

	// Sanity check on the bit depth
	if ((fontType & 6) != 0) {
		warning("Only 1bpp Mac fonts supported");
		return false;
	}

	// If positive, negatedDescent holds the high bits of the offset to the
	// width/offset table.
	// https://developer.apple.com/legacy/library/documentation/mac/Text/Text-252.html
	if (negatedDescent > 0)
		widthOffsetOffset |= negatedDescent << 16;

	// Adjust the offset (it's in terms of words and from its position in the header)
	widthOffsetOffset *= 2;
	widthOffsetOffset += 16;

	uint16 glyphCount = _lastChar - _firstChar + 1;
	_glyphs.resize(glyphCount);

	// Bit image table
	uint16 bitImageSize = _bitImageRowWidth * _rectHeight;
	_bitImage = new byte[bitImageSize];
	stream.read(_bitImage, bitImageSize);

	// Bitmap location table
	// There is one offset for each glyph, then one for the default character
	// and one beyond that to figure out the default character's bitmap width.
	Common::Array<uint16> bitmapOffsets;
	bitmapOffsets.resize(glyphCount + 2);

	for (uint16 i = 0; i < bitmapOffsets.size(); i++)
		bitmapOffsets[i] = stream.readUint16BE();

	for (uint16 i = 0; i < glyphCount + 1; i++) {
		Glyph *glyph = (i == glyphCount) ? &_defaultChar : &_glyphs[i];
		glyph->bitmapOffset = bitmapOffsets[i];
		glyph->bitmapWidth = bitmapOffsets[i + 1] - bitmapOffsets[i];
	}

	// Width/offset table
	stream.seek(widthOffsetOffset);
	for (uint16 i = 0; i < glyphCount; i++) {
		byte kerningOffset = stream.readByte();
		byte width = stream.readByte();

		// Skip missing glyphs. Apparently different from empty glyphs.
		if (kerningOffset == 0xFF && width == 0xFF)
			continue;

		_glyphs[i].kerningOffset = maxKerning + kerningOffset;
		_glyphs[i].width = width;
	}

	// Get the default char offset/width
	_defaultChar.kerningOffset = maxKerning + stream.readByte();
	_defaultChar.width = maxKerning + stream.readByte();

	// We don't need the glyph-width table.
	// (Bit 1 of fontType is its flag)

	// We don't need the image height table either, so we're done.
	// (Bit 0 of fontType is its flag)

	return true;
}

int MacFont::getCharWidth(uint32 chr) const {
	const Glyph *glyph = findGlyph(chr);
	if (!glyph)
		return _maxWidth;

	return glyph->width;
}

void MacFont::drawChar(Surface *dst, uint32 chr, int x, int y, uint32 color) const {
	assert(dst);
	assert(dst->getFormat().bytesPerPixel == 1 || dst->getFormat().bytesPerPixel == 2 || dst->getFormat().bytesPerPixel == 4);

	const Glyph *glyph = findGlyph(chr);
	if (!glyph || glyph->width == 0)
		return;

	for (uint16 i = 0; i < _rectHeight; i++) {
		byte *srcRow = _bitImage + i * _bitImageRowWidth;

		// Draw 'bitmapWidth' worth of pixels. This number may be smaller, in which case
		// it draws the space between characters.
		for (uint16 j = 0; j < glyph->bitmapWidth; j++) {
			uint16 bitmapOffset = glyph->bitmapOffset + j;

			if (srcRow[bitmapOffset / 8] & (1 << (7 - (bitmapOffset % 8)))) {
				if (dst->getFormat().bytesPerPixel == 1)
					*((byte *)dst->getBasePtr(x + j, y + i)) = color;
				else if (dst->getFormat().bytesPerPixel == 2)
					*((uint16 *)dst->getBasePtr(x + j, y + i)) = color;
				else if (dst->getFormat().bytesPerPixel == 4)
					*((uint32 *)dst->getBasePtr(x + j, y + i)) = color;
			}
		}
	}
}

int MacFont::getKerningOffset(uint32 left, uint32 right) const {
	// TODO
	return 0;
}

const MacFont::Glyph *MacFont::findGlyph(uint32 c) const {
	if (_glyphs.empty())
		return 0;

	if (c < _firstChar || c > _lastChar)
		return &_defaultChar;

	return &_glyphs[c - _firstChar];
}

} // End of namespace Graphics
