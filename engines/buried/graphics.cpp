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

// Additional copyright for this file:
// Copyright (C) 1995 Presto Studios, Inc.

#include "common/archive.h"
#ifdef USE_ICONV
#include "common/iconv.h"
#endif
#include "common/stream.h"
#include "common/str-array.h"
#include "common/system.h"
#include "graphics/cursorman.h"
#include "graphics/font.h"
#include "graphics/palette.h"
#include "graphics/surface.h"
#include "graphics/wincursor.h"
#include "graphics/fonts/sysfont.h"
#include "image/bmp.h"

#include "buried/buried.h"
#include "buried/database.h"
#include "buried/graphics.h"
#include "buried/window.h"

namespace Buried {

GraphicsManager::GraphicsManager(BuriedEngine *vm) : _vm(vm) {
	_curCursor = kCursorNone;
	_mouseMoved = false;
	_needsErase = false;

	setCursor(kCursorArrow);
	CursorMan.showMouse(true);

	_screen = new Graphics::Surface();
	_screen->create(640, 480, g_system->getScreenFormat());

	if (_vm->isTrueColor()) {
		// No palette to deal with
		_palette = 0;
	} else {
		// Grab the palette from our EXE bitmap
		_palette = createDefaultPalette();

		// Then apply it. The only time we'll use this call even.
		g_system->getPaletteManager()->setPalette(_palette, 0, 256);
	}
}

GraphicsManager::~GraphicsManager() {
	delete _screen;

	delete[] _palette;
}

// If we don't have iconv available, have a CP-1252 codepage mapping.
#ifndef USE_ICONV

static const uint32 s_codePage1252[256] = {
	0x0000, 0x0001, 0x0002, 0x0003,
	0x0004, 0x0005, 0x0006, 0x0007,
	0x0008, 0x0009, 0x000A, 0x000B,
	0x000C, 0x000D, 0x000E, 0x000F,
	0x0010, 0x0011, 0x0012, 0x0013,
	0x0014, 0x0015, 0x0016, 0x0017,
	0x0018, 0x0019, 0x001A, 0x001B,
	0x001C, 0x001D, 0x001E, 0x001F,
	0x0020, 0x0021, 0x0022, 0x0023,
	0x0024, 0x0025, 0x0026, 0x0027,
	0x0028, 0x0029, 0x002A, 0x002B,
	0x002C, 0x002D, 0x002E, 0x002F,
	0x0030, 0x0031, 0x0032, 0x0033,
	0x0034, 0x0035, 0x0036, 0x0037,
	0x0038, 0x0039, 0x003A, 0x003B,
	0x003C, 0x003D, 0x003E, 0x003F,
	0x0040, 0x0041, 0x0042, 0x0043,
	0x0044, 0x0045, 0x0046, 0x0047,
	0x0048, 0x0049, 0x004A, 0x004B,
	0x004C, 0x004D, 0x004E, 0x004F,
	0x0050, 0x0051, 0x0052, 0x0053,
	0x0054, 0x0055, 0x0056, 0x0057,
	0x0058, 0x0059, 0x005A, 0x005B,
	0x005C, 0x005D, 0x005E, 0x005F,
	0x0060, 0x0061, 0x0062, 0x0063,
	0x0064, 0x0065, 0x0066, 0x0067,
	0x0068, 0x0069, 0x006A, 0x006B,
	0x006C, 0x006D, 0x006E, 0x006F,
	0x0070, 0x0071, 0x0072, 0x0073,
	0x0074, 0x0075, 0x0076, 0x0077,
	0x0078, 0x0079, 0x007A, 0x007B,
	0x007C, 0x007D, 0x007E, 0x007F,
	0xFFFE, 0xFFFE, 0x201A, 0x0192,
	0x201E, 0x2026, 0x2020, 0x2021,
	0x02C6, 0x2030, 0x0160, 0x2039,
	0x0152, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0x2018, 0x2019, 0x201C,
	0x201D, 0x2022, 0x2013, 0x2014,
	0x02DC, 0x2122, 0x0161, 0x203A,
	0x0153, 0xFFFE, 0xFFFE, 0x0178,
	0x00A0, 0x00A1, 0x00A2, 0x00A3,
	0x00A4, 0x00A5, 0x00A6, 0x00A7,
	0x00A8, 0x00A9, 0x00AA, 0x00AB,
	0x00AC, 0x00AD, 0x00AE, 0x00AF,
	0x00B0, 0x00B1, 0x00B2, 0x00B3,
	0x00B4, 0x00B5, 0x00B6, 0x00B7,
	0x00B8, 0x00B9, 0x00BA, 0x00BB,
	0x00BC, 0x00BD, 0x00BE, 0x00BF,
	0x00C0, 0x00C1, 0x00C2, 0x00C3,
	0x00C4, 0x00C5, 0x00C6, 0x00C7,
	0x00C8, 0x00C9, 0x00CA, 0x00CB,
	0x00CC, 0x00CD, 0x00CE, 0x00CF,
	0x00D0, 0x00D1, 0x00D2, 0x00D3,
	0x00D4, 0x00D5, 0x00D6, 0x00D7,
	0x00D8, 0x00D9, 0x00DA, 0x00DB,
	0x00DC, 0x00DD, 0x00DE, 0x00DF,
	0x00E0, 0x00E1, 0x00E2, 0x00E3,
	0x00E4, 0x00E5, 0x00E6, 0x00E7,
	0x00E8, 0x00E9, 0x00EA, 0x00EB,
	0x00EC, 0x00ED, 0x00EE, 0x00EF,
	0x00F0, 0x00F1, 0x00F2, 0x00F3,
	0x00F4, 0x00F5, 0x00F6, 0x00F7,
	0x00F8, 0x00F9, 0x00FA, 0x00FB,
	0x00FC, 0x00FD, 0x00FE, 0x00FF
};

#endif // USE_ICONV

Graphics::Font *GraphicsManager::createFont(int size, bool bold) const {
	// MS Gothic for the Japanese version
	// Arial or Arial Bold for everything else
	if (_vm->getLanguage() == Common::JA_JPN)
		return createMSGothicFont(size, bold);

	return createArialFont(size, bold);
}

Graphics::Font *GraphicsManager::createArialFont(int size, bool bold) const {
	// TODO: Make the monochrome mode optional
	// Win3.1 obviously only had raster fonts, but BIT Win3.1 will render
	// with the TrueType font on Win7/Win8 (at least)
	// FIXME: The font is slightly off from the original... need to check. Sizes are right though!

	Graphics::Font *font = SystemFontMan.createFont("Arial", Graphics::FontSize(size, Graphics::kFontSizeHeight), bold ? Graphics::kFontStyleBold : Graphics::kFontStyleNormal,  _vm->isTrueColor() ? Graphics::kFontRenderLight : Graphics::kFontRenderMonochrome);

	if (!font)
		error("Failed to load Arial%s font", bold ? " Bold" : "");

	return font;
}

Cursor GraphicsManager::setCursor(Cursor newCursor) {
	// Don't set the cursor again
	if (newCursor == _curCursor)
		return _curCursor;

	Cursor oldCursor = _curCursor;
	Graphics::Cursor *cursor = 0;
	Graphics::WinCursorGroup *cursorGroup = 0;

	if (newCursor == kCursorArrow) {
		cursor = Graphics::makeDefaultWinCursor();
	} else if (newCursor == kCursorWait) {
		cursor = Graphics::makeBusyWinCursor();
	} else {
		cursorGroup = _vm->_mainEXE->getCursorGroup(newCursor);

		if (!cursorGroup)
			return kCursorNone;

		cursor = cursorGroup->cursors[0].cursor;
	}

	if (!cursor)
		error("Failed to find cursor %d", newCursor);

	// TODO: Fallback mode for platforms without cursor palettes in 8bpp mode?

	CursorMan.replaceCursor(cursor->getSurface(), cursor->getWidth(), cursor->getHeight(),
			cursor->getHotspotX(), cursor->getHotspotY(), cursor->getKeyColor());
	CursorMan.replaceCursorPalette(cursor->getPalette(), cursor->getPaletteStartIndex(), cursor->getPaletteCount());

	if (cursorGroup)
		delete cursorGroup;
	else
		delete cursor;

	_curCursor = newCursor;
	return oldCursor;
}

Graphics::Surface *GraphicsManager::getBitmap(uint32 bitmapID) {
	Common::SeekableReadStream *stream = _vm->getBitmapStream(bitmapID);

	if (!stream)
		error("Could not find bitmap %d", bitmapID);

	Graphics::Surface *surface = getBitmap(stream);
	if (!surface)
		error("Failed to decode bitmap %d", bitmapID);

	return surface;
}

Graphics::Surface *GraphicsManager::getBitmap(const Common::String &fileName) {
	Common::SeekableReadStream *stream = SearchMan.createReadStreamForMember(fileName);

	if (!stream)
		error("Could not find bitmap '%s'", fileName.c_str());

	Graphics::Surface *surface = getBitmap(stream);
	if (!surface)
		error("Failed to decode bitmap '%s'", fileName.c_str());

	return surface;
}

Graphics::Surface *GraphicsManager::getBitmap(Common::SeekableReadStream *stream) {
	Image::BitmapDecoder decoder;
	if (!decoder.loadStream(*stream)) {
		delete stream;
		return 0;
	}

	delete stream;

	// Convert to the screen format, if required
	if (decoder.getSurface()->getFormat() != g_system->getScreenFormat()) {
		assert(_vm->isTrueColor());
		return decoder.getSurface()->convertTo(g_system->getScreenFormat(), decoder.getPalette());
	}

	// Remap the palette, if required
	if (!_vm->isTrueColor() && memcmp(decoder.getPalette() + 3, getDefaultPalette() + 3, 256 - 6) != 0)
		return remapPalettedFrame(decoder.getSurface(), decoder.getPalette());

	// Just copy the frame
	Graphics::Surface *surface = new Graphics::Surface();
	surface->copyFrom(*decoder.getSurface());
	return surface;
}

uint32 GraphicsManager::getColor(byte r, byte g, byte b) {
	if (_vm->isTrueColor())
		return g_system->getScreenFormat().RGBToColor(r, g, b);

	// Find the best match color
	int diff = 0x7FFFFFFF;
	byte best = 0;

	for (uint i = 0; i < 256 && diff > 0; i++) {
		int rDiff = (int)_palette[i * 3] - (int)r;
		int gDiff = (int)_palette[i * 3 + 1] - (int)g;
		int bDiff = (int)_palette[i * 3 + 2] - (int)b;

		int curDiff = rDiff * rDiff + gDiff * gDiff + bDiff * bDiff;

		if (curDiff < diff) {
			best = i;
			diff = curDiff;
		}
	}

	return best;
}

void GraphicsManager::invalidateRect(const Common::Rect &rect, bool erase) {
	if (_dirtyRect.isEmpty())
		_dirtyRect = rect;
	else
		_dirtyRect.extend(rect);

	_needsErase |= erase;
}

void GraphicsManager::updateScreen(bool drawWindows) {
	bool shouldUpdateScreen = _mouseMoved;
	_mouseMoved = false;

	if (!_dirtyRect.isEmpty()) {
		// Draw the main window, which will draw its children
		if (drawWindows)
			_vm->_mainWindow->updateWindow();

		// Copy just that rect
		g_system->copyRectToScreen(_screen->getBasePtr(_dirtyRect.left, _dirtyRect.top), _screen->getPitch(), _dirtyRect.left, _dirtyRect.top, _dirtyRect.width(), _dirtyRect.height());

		// Empty out the dirty rect
		_dirtyRect = Common::Rect();

		// Definitely update
		shouldUpdateScreen = true;
	}

	if (shouldUpdateScreen)
		g_system->updateScreen();

	_needsErase = false;
}

void GraphicsManager::blit(const Graphics::Surface *surface, int x, int y) {
	assert(surface->getFormat().bytesPerPixel == _screen->getFormat().bytesPerPixel);

	for (int i = 0; i < surface->getHeight(); i++)
		memcpy(_screen->getBasePtr(x, y + i), surface->getBasePtr(0, i), surface->getWidth() * surface->getFormat().bytesPerPixel);
}

void GraphicsManager::blit(const Graphics::Surface *surface, int x, int y, int width, int height) {
	assert(surface->getFormat().bytesPerPixel == _screen->getFormat().bytesPerPixel);

	for (int i = 0; i < height; i++)
		memcpy(_screen->getBasePtr(x, y + i), surface->getBasePtr(0, i), width * surface->getFormat().bytesPerPixel);
}

void GraphicsManager::blit(const Graphics::Surface *surface, const Common::Rect &srcRect, const Common::Rect &dstRect) {
	assert(surface->getFormat().bytesPerPixel == _screen->getFormat().bytesPerPixel);

	int width = MIN(srcRect.width(), dstRect.width());
	int height = MIN(srcRect.height(), dstRect.height());

	for (int i = 0; i < height; i++)
		memcpy(_screen->getBasePtr(dstRect.left, dstRect.top + i), surface->getBasePtr(srcRect.left, srcRect.top + i), width * surface->getFormat().bytesPerPixel);
}

void GraphicsManager::fillRect(const Common::Rect &rect, uint32 color) {
	_screen->fillRect(rect, color);
}

void GraphicsManager::opaqueTransparentBlit(Graphics::Surface *dst, int xDst, int yDst, int w, int h, const Graphics::Surface *src, int xSrc, int ySrc, int opacityValue, byte rTrans, byte gTrans, byte bTrans) {
	if (_vm->isTrueColor()) {
		uint32 transColor = getColor(rTrans, gTrans, bTrans);

		for (int y = 0; y < h; y++) {
			if (y + yDst < dst->getHeight() && y + yDst >= 0) {
				for (int x = 0; x < w; x++) {
					if (x + xDst < dst->getWidth() && x + xDst >= 0) {
						uint32 srcColor;

						if (src->getFormat().bytesPerPixel == 2)
							srcColor = *((const uint16 *)src->getBasePtr(x + xSrc, y + ySrc));
						else
							srcColor = *((const uint32 *)src->getBasePtr(x + xSrc, y + ySrc));

						if (srcColor == transColor)
							continue;

						int srcCycles, dstCycles;
						switch (opacityValue) {
						case 50:
							srcCycles = 1;
							dstCycles = 3;
							break;
						case 85:
							srcCycles = 17;
							dstCycles = 3;
							break;
						default:
							srcCycles = 1;
							dstCycles = 0;
							break;
						}

						byte rSrc, gSrc, bSrc;
						g_system->getScreenFormat().colorToRGB(srcColor, rSrc, gSrc, bSrc);

						uint32 dstColor;
						if (dst->getFormat().bytesPerPixel == 2)
							dstColor = *((uint16 *)dst->getBasePtr(x + xDst, y + yDst));
						else
							dstColor = *((uint32 *)dst->getBasePtr(x + xDst, y + yDst));

						byte rDst, gDst, bDst;
						g_system->getScreenFormat().colorToRGB(dstColor, rDst, gDst, bDst);

						byte r = (((int)rSrc * srcCycles) + ((int)rDst * dstCycles)) / (srcCycles + dstCycles);
						byte g = (((int)gSrc * srcCycles) + ((int)gDst * dstCycles)) / (srcCycles + dstCycles);
						byte b = (((int)bSrc * srcCycles) + ((int)bDst * dstCycles)) / (srcCycles + dstCycles);
						uint32 color = g_system->getScreenFormat().RGBToColor(r, g, b);

						if (dst->getFormat().bytesPerPixel == 2)
							*((uint16 *)dst->getBasePtr(x + xDst, y + yDst)) = color;
						else
							*((uint32 *)dst->getBasePtr(x + xDst, y + yDst)) = color;
					}
				}
			}
		}
	} else {
		// Find the palette index of the color
		int paletteIndex = -1;
		for (int i = 0; i < 256; i++) {
			if (_palette[i * 3] == rTrans && _palette[i * 3 + 1] == gTrans && _palette[i * 3 + 2] == bTrans) {
				paletteIndex = i;
				break;
			}
		}

		assert(paletteIndex >= 0);

		for (int y = 0; y < h; y++) {
			if (y + yDst < dst->getHeight() && y + yDst >= 0) {
				for (int x = 0; x < w; x++) {
					if (x + xDst < dst->getWidth() && x + xDst >= 0) {
						byte color = *((const byte *)src->getBasePtr(x + xSrc, y + ySrc));

						if (color == paletteIndex)
							continue;

						*((byte *)dst->getBasePtr(x + xDst, y + yDst)) = color;
					}
				}
			}
		}
	}
}

bool GraphicsManager::checkPointAgainstMaskedBitmap(const Graphics::Surface *bitmap, int x, int y, const Common::Point &point, byte rTrans, byte gTrans, byte bTrans) {
	if (_vm->isTrueColor()) {
		uint32 transColor = getColor(rTrans, gTrans, bTrans);
		uint32 color;

		if (bitmap->getFormat().bytesPerPixel == 2)
			color = *((const uint16 *)bitmap->getBasePtr(point.x - x, point.y - y));
		else
			color = *((const uint32 *)bitmap->getBasePtr(point.x - x, point.y - y));

		return transColor != color;
	} else {
		// Find the palette index of the color
		int paletteIndex = -1;
		for (int i = 0; i < 256; i++) {
			if (_palette[i * 3] == rTrans && _palette[i * 3 + 1] == gTrans && _palette[i * 3 + 2] == bTrans) {
				paletteIndex = i;
				break;
			}
		}

		assert(paletteIndex >= 0);

		return *((const byte *)bitmap->getBasePtr(point.x - x, point.y - y)) != paletteIndex;
	}
}

byte *GraphicsManager::createDefaultPalette() const {
	Common::SeekableReadStream *stream = _vm->getBitmapStream(700);

	if (!stream)
		error("Couldn't find bitmap 700");

	stream->skip(14);

	if (stream->readUint16LE() != 8)
		error("Trying to load palette from non-8bpp image 700");

	stream->skip(16);

	uint32 colorsUsed = stream->readUint32LE();

	if (colorsUsed != 0 && colorsUsed != 256)
		error("Bitmap 700 is missing a full palette");

	stream->skip(4);
	byte *palette = new byte[256 * 3];
	byte *ptr = palette;

	for (uint32 i = 0; i < 256; i++) {
		ptr[2] = stream->readByte();
		ptr[1] = stream->readByte();
		ptr[0] = stream->readByte();
		stream->readByte();
		ptr += 3;
	}

	delete stream;

	// Make sure the first entry is black and the last is white
	palette[0 * 3]   = palette[0 * 3 + 1]   = palette[0 * 3 + 2]   = 0x00;
	palette[255 * 3] = palette[255 * 3 + 1] = palette[255 * 3 + 2] = 0xFF;

	return palette;
}

Graphics::Surface *GraphicsManager::remapPalettedFrame(const Graphics::Surface *frame, const byte *palette) {
	// This is pretty much the same as the Cinepak one
	// It seems to work for the one video I know that needs it (SWLOGO.BTV)
	// TODO: Merge some of this with getColor()

	byte palMap[256];
	const byte *screenPal = getDefaultPalette();

	for (int i = 0; i < 256; i++) {
		int r = palette[i * 3];
		int g = palette[i * 3 + 1];
		int b = palette[i * 3 + 2];

		int diff = 0x7FFFFFFF;
		byte result = 0;

		for (int j = 0; j < 256; j++) {
			int bDiff = b - (int)screenPal[j * 3 + 2];
			int curDiffB = diff - (bDiff * bDiff);

			if (curDiffB > 0) {
				int gDiff = g - (int)screenPal[j * 3 + 1];
				int curDiffG = curDiffB - (gDiff * gDiff);

				if (curDiffG > 0) {
					int rDiff = r - (int)screenPal[j * 3];
					int curDiffR = curDiffG - (rDiff * rDiff);

					if (curDiffR > 0) {
						diff -= curDiffR;
						result = j;

						if (diff == 0)
							break;
					}
				}
			}
		}

		palMap[i] = result;
	}

	Graphics::Surface *convertedSurface = new Graphics::Surface();
	convertedSurface->create(frame->getWidth(), frame->getHeight(), frame->getFormat());

	for (int y = 0; y < frame->getHeight(); y++)
		for (int x = 0; x < frame->getWidth(); x++)
			*((byte *)convertedSurface->getBasePtr(x, y)) = palMap[*((const byte *)frame->getBasePtr(x, y))];

	return convertedSurface;
}

int GraphicsManager::computeHPushOffset(int speed) {
	switch (speed) {
	case 3:
		return 432;
	case 2:
		return 72;
	case 1:
		return 36;
	case 0:
		return 12;
	}

	return 432;
}

int GraphicsManager::computeVPushOffset(int speed) {
	switch (speed) {
	case 3:
		return 189;
	case 2:
		return 63;
	case 1:
		return 21;
	case 0:
		return 7;
	}

	return 189;
}

void GraphicsManager::crossBlit(Graphics::Surface *dst, int xDst, int yDst, int w, int h, const Graphics::Surface *src, int xSrc, int ySrc) {
	assert(dst->getFormat().bytesPerPixel == src->getFormat().bytesPerPixel);

	for (int y = 0; y < h; y++)
		memcpy(dst->getBasePtr(xDst, yDst + y), src->getBasePtr(xSrc, ySrc + y), w * src->getFormat().bytesPerPixel);
}

Graphics::Font *GraphicsManager::createMSGothicFont(int size, bool bold) const {
	// Force monochrome, since the original uses the bitmap glyphs in the font

	Graphics::Font *font = SystemFontMan.createFont("MS Gothic", Graphics::FontSize(size, Graphics::kFontSizeHeight), bold ? (Graphics::kFontStyleBold | Graphics::kFontStyleEmulate) : Graphics::kFontStyleNormal, Graphics::kFontRenderMonochrome);

	if (!font)
		error("Failed to load MS Gothic font");

	return font;
}

void GraphicsManager::renderText(Graphics::Surface *dst, Graphics::Font *font, const Common::String &text, int x, int y, int w, int h, uint32 color, int lineHeight, TextAlign textAlign, bool centerVertically) {
	if (text.empty())
		return;

#ifdef USE_ICONV
	// Convert to UTF-32 using iconv. Choose the codepage based on the language.
	const char *srcFormat = (_vm->getLanguage() == Common::JA_JPN) ? "CP932" : "CP1252";
	Common::U32String convString = Common::convertToU32String(srcFormat, text);
#else
	// Convert to UTF-32 using our codepage.
	Common::U32String convString(text, s_codePage1252);
#endif

	// Wrap the text
	Common::Array<Common::U32String> lines;
	font->wordWrapText(convString, w, lines);

	Graphics::TextAlign align = Graphics::kTextAlignLeft;
	switch (textAlign) {
	case kTextAlignLeft:
		align = Graphics::kTextAlignLeft;
		break;
	case kTextAlignCenter:
		align = Graphics::kTextAlignCenter;
		break;
	case kTextAlignRight:
		align = Graphics::kTextAlignRight;
		break;
	}

	if (centerVertically)
		y += (h - (lines.size() * lineHeight)) / 2;

	// Why is this needed? Dunno, but I guess Windows adds one row of space on the top
	y++;

	for (uint32 i = 0; i < lines.size(); i++) {
		font->drawString(dst, lines[i], x, y, w, color, align);
		y += lineHeight;
	}
}

void GraphicsManager::drawEllipse(const Common::Rect &rect, uint32 color) {
	// HACK: This just hardcodes the sizes of the rows of the ellipses
	// for the one thing in the game that needs it.

	static const int rows7[7] = { 7, 13, 15, 15, 15, 13, 7 };
	static const int rows10[10] = { 7, 11, 13, 15, 15, 15, 15, 13, 11, 7 };
	static const int rows12[12] = { 7, 11, 13, 13, 15, 15, 15, 15, 13, 13, 11, 7 };
	static const int rows15[15] = { 5, 9, 11, 13, 13, 15, 15, 15, 15, 15, 13, 13, 11, 9, 5 };

	const int *table = 0;
	switch (rect.height()) {
	case 7:
		table = rows7;
		break;
	case 10:
		table = rows10;
		break;
	case 12:
		table = rows12;
		break;
	case 15:
		table = rows15;
		break;
	}

	assert(table);

	for (int y = 0; y < rect.height(); y++) {
		int width = table[y];
		int x = rect.left + (rect.width() - width) / 2;
		_screen->hLine(x, y + rect.top, x + width, color);
	}
}

TempCursorChange::TempCursorChange(Cursor cursor) {
	_prevCursor = static_cast<BuriedEngine*>(g_engine)->_gfx->setCursor(cursor);
}

TempCursorChange::~TempCursorChange() {
	static_cast<BuriedEngine*>(g_engine)->_gfx->setCursor(_prevCursor);
}

} // End of namespace Buried
