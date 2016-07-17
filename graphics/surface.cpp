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

#include "common/algorithm.h"
#include "common/endian.h"
#include "common/util.h"
#include "common/rect.h"
#include "common/textconsole.h"
#include "graphics/primitives.h"
#include "graphics/surface.h"
#include "graphics/conversion.h"

namespace Graphics {

template<typename T>
static void plotPoint(int x, int y, int color, void *data) {
	Surface *s = (Surface *)data;
	if (x >= 0 && x < s->getWidth() && y >= 0 && y < s->getHeight()) {
		T *ptr = (T *)s->getBasePtr(x, y);
		*ptr = (T)color;
	}
}

void Surface::drawLine(int x0, int y0, int x1, int y1, uint32 color) {
	if (getFormat().bytesPerPixel == 1)
		Graphics::drawLine(x0, y0, x1, y1, color, plotPoint<byte>, this);
	else if (getFormat().bytesPerPixel == 2)
		Graphics::drawLine(x0, y0, x1, y1, color, plotPoint<uint16>, this);
	else if (getFormat().bytesPerPixel == 4)
		Graphics::drawLine(x0, y0, x1, y1, color, plotPoint<uint32>, this);
	else
		error("Surface::drawLine: bytesPerPixel must be 1, 2, or 4");
}

void Surface::drawThickLine(int x0, int y0, int x1, int y1, int penX, int penY, uint32 color) {
	if (getFormat().bytesPerPixel == 1)
		Graphics::drawThickLine(x0, y0, x1, y1, penX, penY, color, plotPoint<byte>, this);
	else if (getFormat().bytesPerPixel == 2)
		Graphics::drawThickLine(x0, y0, x1, y1, penX, penY, color, plotPoint<uint16>, this);
	else if (getFormat().bytesPerPixel == 4)
		Graphics::drawThickLine(x0, y0, x1, y1, penX, penY, color, plotPoint<uint32>, this);
	else
		error("Surface::drawThickLine: bytesPerPixel must be 1, 2, or 4");
}

void Surface::create(uint16 width, uint16 height, const PixelFormat &format) {
	uint16 pitch = width * format.bytesPerPixel;
	uint32 size = pitch * height;
	byte *pixels = new byte[size];
	memset(pixels, 0, size);
	reset(width, height, pitch, pixels, format);
}

void Surface::create(uint16 width, uint16 height, uint16 allocWidth, uint16 allocHeight, const PixelFormat &format) {
	assert(width <= allocWidth);
	assert(height <= allocHeight);
	create(allocWidth, allocHeight, format);
	_width = width;
	_height = height;
}

void Surface::free() {
	_width = 0;
	_height = 0;
	_pitch = 0;
	_format = PixelFormat();
	_pixels = 0;
	_ref.reset();
}

void Surface::copyFrom(const Surface &surf) {
	create(surf.getWidth(), surf.getHeight(), surf.getFormat());
	if (surf.getPitch() == getPitch()) {
		memcpy(getPixels(), surf.getPixels(), getHeight() * getPitch());
	} else {
		const byte *src = surf._pixels;
		byte *dst = _pixels;
		for (int y = getHeight(); y > 0; --y) {
			memcpy(dst, src, _width * getFormat().bytesPerPixel);
			src += surf.getPitch();
			dst += getPitch();
		}
	}
}

Surface Surface::getSubArea(const Common::Rect &area) {
	Common::Rect effectiveArea(area);
	effectiveArea.clip(getWidth(), getHeight());

	Surface subSurface;
	subSurface._width = effectiveArea.width();
	subSurface._height = effectiveArea.height();
	subSurface._pitch = getPitch();
	subSurface._pixels = static_cast<byte *>(getBasePtr(area.left, area.top));
	subSurface._format = getFormat();
	subSurface._ref = _ref;
	return subSurface;
}

const Surface Surface::getSubArea(const Common::Rect &area) const {
	Common::Rect effectiveArea(area);
	effectiveArea.clip(getWidth(), getHeight());

	Surface subSurface;
	subSurface._width = effectiveArea.width();
	subSurface._height = effectiveArea.height();
	subSurface._pitch = getPitch();
	// We need to cast the const away here because a Surface always has a
	// pointer to modifiable pixel data.
	subSurface._pixels = static_cast<byte *>(const_cast<void *>(getBasePtr(area.left, area.top)));
	subSurface._format = getFormat();
	subSurface._ref = _ref;
	return subSurface;
}

void Surface::copyRectToSurface(const void *buffer, int srcPitch, int destX, int destY, int width, int height) {
	assert(buffer);

	assert(destX >= 0 && destX < _width);
	assert(destY >= 0 && destY < _height);
	assert(height > 0 && destY + height <= _height);
	assert(width > 0 && destX + width <= _width);

	// Copy buffer data to internal buffer
	const byte *src = (const byte *)buffer;
	byte *dst = (byte *)getBasePtr(destX, destY);
	for (int i = 0; i < height; i++) {
		memcpy(dst, src, width * getFormat().bytesPerPixel);
		src += srcPitch;
		dst += getPitch();
	}
}

void Surface::copyRectToSurface(const Surface &srcSurface, int destX, int destY, const Common::Rect subRect) {
	assert(srcSurface.getFormat() == getFormat());

	copyRectToSurface(srcSurface.getBasePtr(subRect.left, subRect.top), srcSurface.getPitch(), destX, destY, subRect.width(), subRect.height());
}

void Surface::hLine(int x, int y, int x2, uint32 color) {
	// Clipping
	if (y < 0 || y >= getHeight())
		return;

	if (x2 < x)
		SWAP(x2, x);

	if (x < 0)
		x = 0;
	if (x2 >= getWidth())
		x2 = getWidth() - 1;

	if (x2 < x)
		return;

	if (getFormat().bytesPerPixel == 1) {
		byte *ptr = (byte *)getBasePtr(x, y);
		memset(ptr, (byte)color, x2 - x + 1);
	} else if (getFormat().bytesPerPixel == 2) {
		uint16 *ptr = (uint16 *)getBasePtr(x, y);
		Common::fill(ptr, ptr + (x2 - x + 1), (uint16)color);
	} else if (getFormat().bytesPerPixel == 4) {
		uint32 *ptr = (uint32 *)getBasePtr(x, y);
		Common::fill(ptr, ptr + (x2 - x + 1), color);
	} else {
		error("Surface::hLine: bytesPerPixel must be 1, 2, or 4");
	}
}

void Surface::vLine(int x, int y, int y2, uint32 color) {
	// Clipping
	if (x < 0 || x >= getWidth())
		return;

	if (y2 < y)
		SWAP(y2, y);

	if (y < 0)
		y = 0;
	if (y2 >= getHeight())
		y2 = getHeight() - 1;

	if (getFormat().bytesPerPixel == 1) {
		byte *ptr = (byte *)getBasePtr(x, y);
		while (y++ <= y2) {
			*ptr = (byte)color;
			ptr += getPitch();
		}
	} else if (getFormat().bytesPerPixel == 2) {
		uint16 *ptr = (uint16 *)getBasePtr(x, y);
		while (y++ <= y2) {
			*ptr = (uint16)color;
			ptr += getPitch() / 2;
		}

	} else if (getFormat().bytesPerPixel == 4) {
		uint32 *ptr = (uint32 *)getBasePtr(x, y);
		while (y++ <= y2) {
			*ptr = color;
			ptr += getPitch() / 4;
		}
	} else {
		error("Surface::vLine: bytesPerPixel must be 1, 2, or 4");
	}
}

void Surface::fillRect(Common::Rect r, uint32 color) {
	r.clip(getWidth(), getHeight());

	if (!r.isValidRect())
		return;

	int width = r.width();
	int lineLen = width;
	int height = r.height();
	bool useMemset = true;

	if (getFormat().bytesPerPixel == 2) {
		lineLen *= 2;
		if ((uint16)color != ((color & 0xff) | (color & 0xff) << 8))
			useMemset = false;
	} else if (getFormat().bytesPerPixel == 4) {
		useMemset = false;
	} else if (getFormat().bytesPerPixel != 1) {
		error("Surface::fillRect: bytesPerPixel must be 1, 2, or 4");
	}

	if (useMemset) {
		byte *ptr = (byte *)getBasePtr(r.left, r.top);
		while (height--) {
			memset(ptr, (byte)color, lineLen);
			ptr += getPitch();
		}
	} else {
		if (getFormat().bytesPerPixel == 2) {
			uint16 *ptr = (uint16 *)getBasePtr(r.left, r.top);
			while (height--) {
				Common::fill(ptr, ptr + width, (uint16)color);
				ptr += getPitch() / 2;
			}
		} else {
			uint32 *ptr = (uint32 *)getBasePtr(r.left, r.top);
			while (height--) {
				Common::fill(ptr, ptr + width, color);
				ptr += getPitch() / 4;
			}
		}
	}
}

void Surface::frameRect(const Common::Rect &r, uint32 color) {
	hLine(r.left, r.top, r.right - 1, color);
	hLine(r.left, r.bottom - 1, r.right - 1, color);
	vLine(r.left, r.top, r.bottom - 1, color);
	vLine(r.right - 1, r.top, r.bottom - 1, color);
}

void Surface::move(int dx, int dy, int height) {
	// Short circuit check - do we have to do anything anyway?
	if ((dx == 0 && dy == 0) || height <= 0)
		return;

	if (getFormat().bytesPerPixel != 1 && getFormat().bytesPerPixel != 2 && getFormat().bytesPerPixel != 4)
		error("Surface::move: bytesPerPixel must be 1, 2, or 4");

	byte *src, *dst;
	int x, y;

	// vertical movement
	if (dy > 0) {
		// move down - copy from bottom to top
		dst = (byte *)getPixels() + (height - 1) * getPitch();
		src = dst - dy * getPitch();
		for (y = dy; y < height; y++) {
			memcpy(dst, src, getPitch());
			src -= getPitch();
			dst -= getPitch();
		}
	} else if (dy < 0) {
		// move up - copy from top to bottom
		dst = (byte *)getPixels();
		src = dst - dy * getPitch();
		for (y = -dy; y < height; y++) {
			memcpy(dst, src, getPitch());
			src += getPitch();
			dst += getPitch();
		}
	}

	// horizontal movement
	if (dx > 0) {
		// move right - copy from right to left
		dst = (byte *)getPixels() + (getPitch() - getFormat().bytesPerPixel);
		src = dst - (dx * getFormat().bytesPerPixel);
		for (y = 0; y < height; y++) {
			for (x = dx; x < getWidth(); x++) {
				if (getFormat().bytesPerPixel == 1) {
					*dst-- = *src--;
				} else if (getFormat().bytesPerPixel == 2) {
					*(uint16 *)dst = *(const uint16 *)src;
					src -= 2;
					dst -= 2;
				} else if (getFormat().bytesPerPixel == 4) {
					*(uint32 *)dst = *(const uint32 *)src;
					src -= 4;
					dst -= 4;
				}
			}
			src += getPitch() + (getPitch() - dx * getFormat().bytesPerPixel);
			dst += getPitch() + (getPitch() - dx * getFormat().bytesPerPixel);
		}
	} else if (dx < 0)  {
		// move left - copy from left to right
		dst = (byte *)getPixels();
		src = dst - (dx * getFormat().bytesPerPixel);
		for (y = 0; y < height; y++) {
			for (x = -dx; x < getWidth(); x++) {
				if (getFormat().bytesPerPixel == 1) {
					*dst++ = *src++;
				} else if (getFormat().bytesPerPixel == 2) {
					*(uint16 *)dst = *(const uint16 *)src;
					src += 2;
					dst += 2;
				} else if (getFormat().bytesPerPixel == 4) {
					*(uint32 *)dst = *(const uint32 *)src;
					src += 4;
					dst += 4;
				}
			}
			src += getPitch() - (getPitch() + dx * getFormat().bytesPerPixel);
			dst += getPitch() - (getPitch() + dx * getFormat().bytesPerPixel);
		}
	}
}

void Surface::convertToInPlace(const PixelFormat &dstFormat, const byte *palette) {
	Common::ScopedPtr<Surface> newSurface(convertTo(dstFormat, palette));
	*this = *newSurface;
}

Surface *Surface::convertTo(const PixelFormat &dstFormat, const byte *palette) const {
	assert(getPixels() != 0);

	// If the target format is the same, just copy
	if (getFormat() == dstFormat)
		return new Surface(*this);

	if (getFormat().bytesPerPixel < 1 || getFormat().bytesPerPixel > 4)
		error("Surface::convertTo(): Can only convert from 1Bpp, 2Bpp, 3Bpp, and 4Bpp");

	if (dstFormat.bytesPerPixel != 2 && dstFormat.bytesPerPixel != 4)
		error("Surface::convertTo(): Can only convert to 2Bpp and 4Bpp");

	Surface *surface = new Surface(getWidth(), getHeight(), dstFormat);

	if (getFormat().bytesPerPixel == 1) {
		// Converting from paletted to high color
		assert(palette);

		for (int y = 0; y < getHeight(); y++) {
			const byte *srcRow = (const byte *)getBasePtr(0, y);
			byte *dstRow = (byte *)surface->getBasePtr(0, y);

			for (int x = 0; x < getWidth(); x++) {
				byte index = *srcRow++;
				byte r = palette[index * 3];
				byte g = palette[index * 3 + 1];
				byte b = palette[index * 3 + 2];

				uint32 color = dstFormat.RGBToColor(r, g, b);

				if (dstFormat.bytesPerPixel == 2)
					*((uint16 *)dstRow) = color;
				else
					*((uint32 *)dstRow) = color;

				dstRow += dstFormat.bytesPerPixel;
			}
		}
	} else {
		// Converting from high color to high color
		for (int y = 0; y < getHeight(); y++) {
			const byte *srcRow = (const byte *)getBasePtr(0, y);
			byte *dstRow = (byte *)surface->getBasePtr(0, y);

			for (int x = 0; x < getWidth(); x++) {
				uint32 srcColor;
				if (getFormat().bytesPerPixel == 2)
					srcColor = READ_UINT16(srcRow);
				else if (getFormat().bytesPerPixel == 3)
					srcColor = READ_UINT24(srcRow);
				else
					srcColor = READ_UINT32(srcRow);

				srcRow += getFormat().bytesPerPixel;

				// Convert that color to the new format
				byte r, g, b, a;
				getFormat().colorToARGB(srcColor, a, r, g, b);
				uint32 color = dstFormat.ARGBToColor(a, r, g, b);

				if (dstFormat.bytesPerPixel == 2)
					*((uint16 *)dstRow) = color;
				else
					*((uint32 *)dstRow) = color;

				dstRow += dstFormat.bytesPerPixel;
			}
		}
	}

	return surface;
}

} // End of namespace Graphics
