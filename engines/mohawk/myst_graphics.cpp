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

#include "mohawk/myst.h"
#include "mohawk/myst_graphics.h"
#include "mohawk/resource.h"

#include "common/substream.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "engines/util.h"
#include "graphics/palette.h"
#include "image/pict.h"

namespace Mohawk {

MystGraphics::MystGraphics(MohawkEngine_Myst* vm) : GraphicsManager(), _vm(vm) {
	_bmpDecoder = new MystBitmap();

	_viewport = Common::Rect(544, 332);

	if (_vm->getFeatures() & GF_ME) {
		// High color
		initGraphics(_viewport.width(), _viewport.height(), true, NULL);

		if (_vm->_system->getScreenFormat().bytesPerPixel == 1)
			error("Myst ME requires greater than 256 colors to run");
	} else {
		// Paletted
		initGraphics(_viewport.width(), _viewport.height(), true);
		setBasePalette();
		setPaletteToScreen();
	}

	_pixelFormat = _vm->_system->getScreenFormat();

	// Initialize our buffer
	_backBuffer = new Graphics::Surface();
	_backBuffer->create(_vm->_system->getWidth(), _vm->_system->getHeight(), _pixelFormat);

	_nextAllowedDrawTime = _vm->_system->getMillis();
	_enableDrawingTimeSimulation = 0;
}

MystGraphics::~MystGraphics() {
	delete _bmpDecoder;
	delete _backBuffer;
}

MohawkSurface *MystGraphics::decodeImage(uint16 id) {
	// We need to grab the image from the current stack archive, however, we  don't know
	// if it's a PICT or WDIB resource. If it's Myst ME it's most likely a PICT, and if it's
	// original it's definitely a WDIB. However, Myst ME throws us another curve ball in
	// that PICT resources can contain WDIB's instead of PICT's.
	Common::SeekableReadStream *dataStream = NULL;

	if (_vm->getFeatures() & GF_ME && _vm->hasResource(ID_PICT, id)) {
		// The PICT resource exists. However, it could still contain a MystBitmap
		// instead of a PICT image...
		dataStream = _vm->getResource(ID_PICT, id);
	} else {
		// No PICT, so the WDIB must exist. Let's go grab it.
		dataStream = _vm->getResource(ID_WDIB, id);
	}

	bool isPict = false;

	if (_vm->getFeatures() & GF_ME) {
		// Here we detect whether it's really a PICT or a WDIB. Since a MystBitmap
		// would be compressed, there's no way to detect for the BM without a hack.
		// So, we search for the PICT version opcode for detection.
		dataStream->seek(512 + 10); // 512 byte pict header
		isPict = (dataStream->readUint32BE() == 0x001102FF);
		dataStream->seek(0);
	}

	MohawkSurface *mhkSurface = 0;

	if (isPict) {
		Image::PICTDecoder pict;

		if (!pict.loadStream(*dataStream))
			error("Could not decode Myst ME PICT");

		mhkSurface = new MohawkSurface(pict.getSurface()->convertTo(_pixelFormat));
	} else {
		mhkSurface = _bmpDecoder->decodeImage(dataStream);

		if (_vm->getFeatures() & GF_ME)
			mhkSurface->convertToTrueColor();
	}

	assert(mhkSurface);
	return mhkSurface;
}

void MystGraphics::copyImageSectionToScreen(uint16 image, Common::Rect src, Common::Rect dest) {
	Graphics::Surface *surface = findImage(image)->getSurface();

	// Make sure the image is bottom aligned in the dest rect
	dest.top = dest.bottom - MIN<int>(surface->getHeight(), dest.height());

	// Convert from bitmap coordinates to surface coordinates
	uint16 top = surface->getHeight() - (src.top + MIN<int>(surface->getHeight(), dest.height()));

	// Do not draw the top pixels if the image is too tall
	if (dest.height() > _viewport.height())
		top += dest.height() - _viewport.height();

	// Clip the destination rect to the screen
	if (dest.right > _vm->_system->getWidth() || dest.bottom > _vm->_system->getHeight())
		dest.debugPrint(4, "Clipping destination rect to the screen");
	dest.right = CLIP<int>(dest.right, 0, _vm->_system->getWidth());
	dest.bottom = CLIP<int>(dest.bottom, 0, _vm->_system->getHeight());

	uint16 width = MIN<int>(surface->getWidth(), dest.width());
	uint16 height = MIN<int>(surface->getHeight(), dest.height());

	// Clamp Width and Height to within src surface dimensions
	if (src.left + width > surface->getWidth())
		width = surface->getWidth() - src.left;
	if (src.top + height > surface->getHeight())
		height = surface->getHeight() - src.top;

	debug(3, "MystGraphics::copyImageSectionToScreen()");
	debug(3, "\tImage: %d", image);
	debug(3, "\tsrc.left: %d", src.left);
	debug(3, "\tsrc.top: %d", src.top);
	debug(3, "\tdest.left: %d", dest.left);
	debug(3, "\tdest.top: %d", dest.top);
	debug(3, "\twidth: %d", width);
	debug(3, "\theight: %d", height);

	simulatePreviousDrawDelay(dest);

	_vm->_system->copyRectToScreen(surface->getBasePtr(src.left, top), surface->getPitch(), dest.left, dest.top, width, height);
}

void MystGraphics::copyImageSectionToBackBuffer(uint16 image, Common::Rect src, Common::Rect dest) {
	MohawkSurface *mhkSurface = findImage(image);
	Graphics::Surface *surface = mhkSurface->getSurface();

	// Make sure the image is bottom aligned in the dest rect
	dest.top = dest.bottom - MIN<int>(surface->getHeight(), dest.height());

	// Convert from bitmap coordinates to surface coordinates
	uint16 top = surface->getHeight() - (src.top + MIN<int>(surface->getHeight(), dest.height()));

	// Do not draw the top pixels if the image is too tall
	if (dest.height() > _viewport.height()) {
		top += dest.height() - _viewport.height();
	}

	// Clip the destination rect to the screen
	if (dest.right > _vm->_system->getWidth() || dest.bottom > _vm->_system->getHeight())
		dest.debugPrint(4, "Clipping destination rect to the screen");
	dest.right = CLIP<int>(dest.right, 0, _vm->_system->getWidth());
	dest.bottom = CLIP<int>(dest.bottom, 0, _vm->_system->getHeight());

	uint16 width = MIN<int>(surface->getWidth(), dest.width());
	uint16 height = MIN<int>(surface->getHeight(), dest.height());

	// Clamp Width and Height to within src surface dimensions
	if (src.left + width > surface->getWidth())
		width = surface->getWidth() - src.left;
	if (src.top + height > surface->getHeight())
		height = surface->getHeight() - src.top;

	debug(3, "MystGraphics::copyImageSectionToBackBuffer()");
	debug(3, "\tImage: %d", image);
	debug(3, "\tsrc.left: %d", src.left);
	debug(3, "\tsrc.top: %d", src.top);
	debug(3, "\tdest.left: %d", dest.left);
	debug(3, "\tdest.top: %d", dest.top);
	debug(3, "\twidth: %d", width);
	debug(3, "\theight: %d", height);

	for (uint16 i = 0; i < height; i++)
		memcpy(_backBuffer->getBasePtr(dest.left, i + dest.top), surface->getBasePtr(src.left, top + i), width * surface->getFormat().bytesPerPixel);

	if (!(_vm->getFeatures() & GF_ME)) {
		// Make sure the palette is set
		assert(mhkSurface->getPalette());
		memcpy(_palette + 10 * 3, mhkSurface->getPalette() + 10 * 3, (256 - 10 * 2) * 3);
		setPaletteToScreen();
	}
}

void MystGraphics::copyImageToScreen(uint16 image, Common::Rect dest) {
	copyImageSectionToScreen(image, Common::Rect(544, 333), dest);
}

void MystGraphics::copyImageToBackBuffer(uint16 image, Common::Rect dest) {
	copyImageSectionToBackBuffer(image, Common::Rect(544, 333), dest);
}

void MystGraphics::copyBackBufferToScreen(Common::Rect r) {
	r.clip(_viewport);

	simulatePreviousDrawDelay(r);

	_vm->_system->copyRectToScreen(_backBuffer->getBasePtr(r.left, r.top), _backBuffer->getPitch(), r.left, r.top, r.width(), r.height());
}

void MystGraphics::runTransition(TransitionType type, Common::Rect rect, uint16 steps, uint16 delay) {

	// Do not artificially delay during transitions
	int oldEnableDrawingTimeSimulation = _enableDrawingTimeSimulation;
	_enableDrawingTimeSimulation = 0;

	switch (type) {
	case kTransitionLeftToRight:	{
			debugC(kDebugView, "Left to Right");

			uint16 step = (rect.right - rect.left) / steps;
			Common::Rect area = rect;
			for (uint i = 0; i < steps; i++) {
				area.left = rect.left + step * i;
				area.right = area.left + step;

				_vm->_system->delayMillis(delay);

				copyBackBufferToScreen(area);
				_vm->_system->updateScreen();
			}
			if (area.right < rect.right) {
				area.left = area.right;
				area.right = rect.right;

				copyBackBufferToScreen(area);
				_vm->_system->updateScreen();
			}
		}
		break;
	case kTransitionRightToLeft:	{
			debugC(kDebugView, "Right to Left");

			uint16 step = (rect.right - rect.left) / steps;
			Common::Rect area = rect;
			for (uint i = 0; i < steps; i++) {
				area.right = rect.right - step * i;
				area.left = area.right - step;

				_vm->_system->delayMillis(delay);

				copyBackBufferToScreen(area);
				_vm->_system->updateScreen();
			}
			if (area.left > rect.left) {
				area.right = area.left;
				area.left = rect.left;

				copyBackBufferToScreen(area);
				_vm->_system->updateScreen();
			}
		}
		break;
	case kTransitionSlideToLeft:
		debugC(kDebugView, "Slide to left");
		transitionSlideToLeft(rect, steps, delay);
		break;
	case kTransitionSlideToRight:
		debugC(kDebugView, "Slide to right");
		transitionSlideToRight(rect, steps, delay);
		break;
	case kTransitionDissolve: {
			debugC(kDebugView, "Dissolve");

			for (int16 step = 0; step < 8; step++) {
				simulatePreviousDrawDelay(rect);
				transitionDissolve(rect, step);
			}
		}
		break;
	case kTransitionTopToBottom:	{
			debugC(kDebugView, "Top to Bottom");

			uint16 step = (rect.bottom - rect.top) / steps;
			Common::Rect area = rect;
			for (uint i = 0; i < steps; i++) {
				area.top = rect.top + step * i;
				area.bottom = area.top + step;

				_vm->_system->delayMillis(delay);

				copyBackBufferToScreen(area);
				_vm->_system->updateScreen();
			}
			if (area.bottom < rect.bottom) {
				area.top = area.bottom;
				area.bottom = rect.bottom;

				copyBackBufferToScreen(area);
				_vm->_system->updateScreen();
			}
		}
		break;
	case kTransitionBottomToTop:	{
			debugC(kDebugView, "Bottom to Top");

			uint16 step = (rect.bottom - rect.top) / steps;
			Common::Rect area = rect;
			for (uint i = 0; i < steps; i++) {
				area.bottom = rect.bottom - step * i;
				area.top = area.bottom - step;

				_vm->_system->delayMillis(delay);

				copyBackBufferToScreen(area);
				_vm->_system->updateScreen();
			}
			if (area.top > rect.top) {
				area.bottom = area.top;
				area.top = rect.top;

				copyBackBufferToScreen(area);
				_vm->_system->updateScreen();
			}
		}
		break;
	case kTransitionSlideToTop:
		debugC(kDebugView, "Slide to top");
		transitionSlideToTop(rect, steps, delay);
		break;
	case kTransitionSlideToBottom:
		debugC(kDebugView, "Slide to bottom");
		transitionSlideToBottom(rect, steps, delay);
		break;
	case kTransitionPartToRight: {
			debugC(kDebugView, "Partial left to right");

			transitionPartialToRight(rect, 75, 3);
		}
		break;
	case kTransitionPartToLeft: {
			debugC(kDebugView, "Partial right to left");

			transitionPartialToLeft(rect, 75, 3);
		}
		break;
	case kTransitionCopy:
		copyBackBufferToScreen(rect);
		_vm->_system->updateScreen();
		break;
	default:
		error("Unknown transition %d", type);
	}

	_enableDrawingTimeSimulation = oldEnableDrawingTimeSimulation;
}

void MystGraphics::transitionDissolve(Common::Rect rect, uint step) {
	static const bool pattern[][4][4] = {
		{
			{ true,  false, false, false },
			{ false, false, false, false },
			{ false, false, true,  false },
			{ false, false, false, false }
		},
		{
			{ false, false, true,  false },
			{ false, false, false, false },
			{ true,  false, false, false },
			{ false, false, false, false }
		},
		{
			{ false, false, false, false },
			{ false, true,  false, false },
			{ false, false, false, false },
			{ false, false, false, true  }
		},
		{
			{ false, false, false, false },
			{ false, false, false, true  },
			{ false, false, false, false },
			{ false, true,  false, false }
		},
		{
			{ false, false, false, false },
			{ false, false, true,  false },
			{ false, true,  false, false },
			{ false, false, false, false }
		},
		{
			{ false, true,  false, false },
			{ false, false, false, false },
			{ false, false, false, false },
			{ false, false, true,  false }
		},
		{
			{ false, false, false, false },
			{ true,  false, false, false },
			{ false, false, false, true  },
			{ false, false, false, false }
		},
		{
			{ false, false, false, true  },
			{ false, false, false, false },
			{ false, false, false, false },
			{ true,  false, false, false }
		}
	};

	rect.clip(_viewport);

	Graphics::Surface *screen = _vm->_system->lockScreen();

	for (uint16 y = rect.top; y < rect.bottom; y++) {
		const bool *linePattern = pattern[step][y % 4];

		if (!linePattern[0] && !linePattern[1] && !linePattern[2] && !linePattern[3])
			continue;

		for (uint16 x = rect.left; x < rect.right; x++) {
			if (linePattern[x % 4]) {
				switch (_pixelFormat.bytesPerPixel) {
				case 1:
					*((byte *)screen->getBasePtr(x, y)) = *((const byte *)_backBuffer->getBasePtr(x, y));
					break;
				case 2:
					*((uint16 *)screen->getBasePtr(x, y)) = *((const uint16 *)_backBuffer->getBasePtr(x, y));
					break;
				case 4:
					*((uint32 *)screen->getBasePtr(x, y)) = *((const uint32 *)_backBuffer->getBasePtr(x, y));
					break;
				}
			}
		}
	}

	_vm->_system->unlockScreen();
	_vm->_system->updateScreen();
}

void MystGraphics::transitionSlideToLeft(Common::Rect rect, uint16 steps, uint16 delay) {
	rect.clip(_viewport);

	uint32 stepWidth = (rect.right - rect.left) / steps;
	Common::Rect srcRect = Common::Rect(rect.right, rect.top, rect.right, rect.bottom);
	Common::Rect dstRect = Common::Rect(rect.left, rect.top, rect.left, rect.bottom);

	for (uint step = 1; step <= steps; step++) {
		dstRect.right = dstRect.left + step * stepWidth;
		srcRect.left = srcRect.right - step * stepWidth;

		_vm->_system->delayMillis(delay);

		simulatePreviousDrawDelay(dstRect);
		_vm->_system->copyRectToScreen(_backBuffer->getBasePtr(dstRect.left, dstRect.top),
				_backBuffer->getPitch(), srcRect.left, srcRect.top, srcRect.width(), srcRect.height());
		_vm->_system->updateScreen();
	}

	if (dstRect.right != rect.right) {
		copyBackBufferToScreen(rect);
		_vm->_system->updateScreen();
	}
}

void MystGraphics::transitionSlideToRight(Common::Rect rect, uint16 steps, uint16 delay) {
	rect.clip(_viewport);

	uint32 stepWidth = (rect.right - rect.left) / steps;
	Common::Rect srcRect = Common::Rect(rect.left, rect.top, rect.left, rect.bottom);
	Common::Rect dstRect = Common::Rect(rect.right, rect.top, rect.right, rect.bottom);

	for (uint step = 1; step <= steps; step++) {
		dstRect.left = dstRect.right - step * stepWidth;
		srcRect.right = srcRect.left + step * stepWidth;

		_vm->_system->delayMillis(delay);

		simulatePreviousDrawDelay(dstRect);
		_vm->_system->copyRectToScreen(_backBuffer->getBasePtr(dstRect.left, dstRect.top),
				_backBuffer->getPitch(), srcRect.left, srcRect.top, srcRect.width(), srcRect.height());
		_vm->_system->updateScreen();
	}

	if (dstRect.left != rect.left) {
		copyBackBufferToScreen(rect);
		_vm->_system->updateScreen();
	}
}

void MystGraphics::transitionSlideToTop(Common::Rect rect, uint16 steps, uint16 delay) {
	rect.clip(_viewport);

	uint32 stepWidth = (rect.bottom - rect.top) / steps;
	Common::Rect srcRect = Common::Rect(rect.left, rect.bottom, rect.right, rect.bottom);
	Common::Rect dstRect = Common::Rect(rect.left, rect.top, rect.right, rect.top);

	for (uint step = 1; step <= steps; step++) {
		dstRect.bottom = dstRect.top + step * stepWidth;
		srcRect.top = srcRect.bottom - step * stepWidth;

		_vm->_system->delayMillis(delay);

		simulatePreviousDrawDelay(dstRect);
		_vm->_system->copyRectToScreen(_backBuffer->getBasePtr(dstRect.left, dstRect.top),
				_backBuffer->getPitch(), srcRect.left, srcRect.top, srcRect.width(), srcRect.height());
		_vm->_system->updateScreen();
	}


	if (dstRect.bottom < rect.bottom) {
		copyBackBufferToScreen(rect);
		_vm->_system->updateScreen();
	}
}

void MystGraphics::transitionSlideToBottom(Common::Rect rect, uint16 steps, uint16 delay) {
	rect.clip(_viewport);

	uint32 stepWidth = (rect.bottom - rect.top) / steps;
	Common::Rect srcRect = Common::Rect(rect.left, rect.top, rect.right, rect.top);
	Common::Rect dstRect = Common::Rect(rect.left, rect.bottom, rect.right, rect.bottom);

	for (uint step = 1; step <= steps; step++) {
		dstRect.top = dstRect.bottom - step * stepWidth;
		srcRect.bottom = srcRect.top + step * stepWidth;

		_vm->_system->delayMillis(delay);

		simulatePreviousDrawDelay(dstRect);
		_vm->_system->copyRectToScreen(_backBuffer->getBasePtr(dstRect.left, dstRect.top),
				_backBuffer->getPitch(), srcRect.left, srcRect.top, srcRect.width(), srcRect.height());
		_vm->_system->updateScreen();
	}


	if (dstRect.top > rect.top) {
		copyBackBufferToScreen(rect);
		_vm->_system->updateScreen();
	}
}

void MystGraphics::transitionPartialToRight(Common::Rect rect, uint32 width, uint32 steps) {
	rect.clip(_viewport);

	uint32 stepWidth = width / steps;
	Common::Rect srcRect = Common::Rect(rect.right, rect.top, rect.right, rect.bottom);
	Common::Rect dstRect = Common::Rect(rect.left, rect.top, rect.left, rect.bottom);

	for (uint step = 1; step <= steps; step++) {
		dstRect.right = dstRect.left + step * stepWidth;
		srcRect.left = srcRect.right - step * stepWidth;

		simulatePreviousDrawDelay(dstRect);
		_vm->_system->copyRectToScreen(_backBuffer->getBasePtr(dstRect.left, dstRect.top),
				_backBuffer->getPitch(), srcRect.left, srcRect.top, srcRect.width(), srcRect.height());
		_vm->_system->updateScreen();
	}

	copyBackBufferToScreen(rect);
	_vm->_system->updateScreen();
}

void MystGraphics::transitionPartialToLeft(Common::Rect rect, uint32 width, uint32 steps) {
	rect.clip(_viewport);

	uint32 stepWidth = width / steps;
	Common::Rect srcRect = Common::Rect(rect.left, rect.top, rect.left, rect.bottom);
	Common::Rect dstRect = Common::Rect(rect.right, rect.top, rect.right, rect.bottom);

	for (uint step = 1; step <= steps; step++) {
		dstRect.left = dstRect.right - step * stepWidth;
		srcRect.right = srcRect.left + step * stepWidth;

		simulatePreviousDrawDelay(dstRect);
		_vm->_system->copyRectToScreen(_backBuffer->getBasePtr(dstRect.left, dstRect.top),
				_backBuffer->getPitch(), srcRect.left, srcRect.top, srcRect.width(), srcRect.height());
		_vm->_system->updateScreen();
	}

	copyBackBufferToScreen(rect);
	_vm->_system->updateScreen();
}

void MystGraphics::drawRect(Common::Rect rect, RectState state) {
	rect.clip(_viewport);

	// Useful with debugging. Shows where hotspots are on the screen and whether or not they're active.
	if (!rect.isValidRect() || rect.width() == 0 || rect.height() == 0)
		return;

	Graphics::Surface *screen = _vm->_system->lockScreen();

	if (state == kRectEnabled)
		screen->frameRect(rect, (_vm->getFeatures() & GF_ME) ? _pixelFormat.RGBToColor(0, 255, 0) : 250);
	else if (state == kRectUnreachable)
		screen->frameRect(rect, (_vm->getFeatures() & GF_ME) ? _pixelFormat.RGBToColor(0, 0, 255) : 252);
	else
		screen->frameRect(rect, (_vm->getFeatures() & GF_ME) ? _pixelFormat.RGBToColor(255, 0, 0) : 249);

	_vm->_system->unlockScreen();
}

void MystGraphics::drawLine(const Common::Point &p1, const Common::Point &p2, uint32 color) {
	_backBuffer->drawLine(p1.x, p1.y, p2.x, p2.y, color);
}

void MystGraphics::enableDrawingTimeSimulation(bool enable) {
	if (enable)
		_enableDrawingTimeSimulation++;
	else
		_enableDrawingTimeSimulation--;

	if (_enableDrawingTimeSimulation < 0)
		_enableDrawingTimeSimulation = 0;
}

void MystGraphics::simulatePreviousDrawDelay(const Common::Rect &dest) {
	uint32 time = 0;

	if (_enableDrawingTimeSimulation) {
		time = _vm->_system->getMillis();

		// Do not draw anything new too quickly after the previous draw call
		// so that images stay at least a little while on screen
		// This is enabled only for scripted draw calls
		if (time < _nextAllowedDrawTime)
			_vm->_system->delayMillis(_nextAllowedDrawTime - time);
	}

	// Next draw call allowed at DELAY + AERA * COEFF milliseconds from now
	time = _vm->_system->getMillis();
	_nextAllowedDrawTime = time + _constantDrawDelay + dest.height() * dest.width() / _proportionalDrawDelay;
}

void MystGraphics::fadeToBlack() {
	// This is only for the demo
	assert(!(_vm->getFeatures() & GF_ME));

	// Linear fade in 64 steps
	for (int i = 63; i >= 0; i--) {
		byte palette[256 * 3];
		byte *src = _palette;
		byte *dst = palette;

		for (uint j = 0; j < sizeof(palette); j++)
			*dst++ = *src++ * i / 64;

		_vm->_system->getPaletteManager()->setPalette(palette, 0, 256);
		_vm->_system->updateScreen();
	}
}

void MystGraphics::fadeFromBlack() {
	// This is only for the demo
	assert(!(_vm->getFeatures() & GF_ME));

	copyBackBufferToScreen(_viewport);

	// Linear fade in 64 steps
	for (int i = 0; i < 64; i++) {
		byte palette[256 * 3];
		byte *src = _palette;
		byte *dst = palette;

		for (uint j = 0; j < sizeof(palette); j++)
			*dst++ = *src++ * i / 64;

		_vm->_system->getPaletteManager()->setPalette(palette, 0, 256);
		_vm->_system->updateScreen();
	}

	// Set the full palette
	_vm->_system->getPaletteManager()->setPalette(_palette, 0, 256);
	_vm->_system->updateScreen();
}

void MystGraphics::clearScreenPalette() {
	// Set the palette to all black
	byte palette[256 * 3];
	memset(palette, 0, sizeof(palette));
	_vm->_system->getPaletteManager()->setPalette(palette, 0, 256);
}

void MystGraphics::setBasePalette() {
	// Entries [0, 9] of the palette
	static const byte lowPalette[] = {
		0xFF, 0xFF, 0xFF,
		0x80, 0x00, 0x00,
		0x00, 0x80, 0x00,
		0x80, 0x80, 0x00,
		0x00, 0x00, 0x80,
		0x80, 0x00, 0x80,
		0x00, 0x80, 0x80,
		0xC0, 0xC0, 0xC0,
		0xC0, 0xDC, 0xC0,
		0xA6, 0xCA, 0xF0
	};

	// Entries [246, 255] of the palette
	static const byte highPalette[] = {
		0xFF, 0xFB, 0xF0,
		0xA0, 0xA0, 0xA4,
		0x80, 0x80, 0x80,
		0xFF, 0x00, 0x00,
		0x00, 0xFF, 0x00,
		0xFF, 0xFF, 0x00,
		0x00, 0x00, 0xFF,
		0xFF, 0x00, 0xFF,
		0x00, 0xFF, 0xFF,
		0x00, 0x00, 0x00
	};

	// Note that 0 and 255 are different from normal Windows.
	// Myst seems to hack that to white, resp. black (probably for Mac compat).

	memcpy(_palette, lowPalette, sizeof(lowPalette));
	memset(_palette + sizeof(lowPalette), 0, sizeof(_palette) - sizeof(lowPalette) - sizeof(highPalette));
	memcpy(_palette + sizeof(_palette) - sizeof(highPalette), highPalette, sizeof(highPalette));
}

void MystGraphics::setPaletteToScreen() {
	_vm->_system->getPaletteManager()->setPalette(_palette, 0, 256);
}

} // End of namespace Mohawk
