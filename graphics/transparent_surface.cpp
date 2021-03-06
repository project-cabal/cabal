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

// The bottom part of this is file is adapted from SDL_rotozoom.c. The
// relevant copyright notice for those specific functions can be found at the
// top of that section.

#include "common/algorithm.h"
#include "common/endian.h"
#include "common/util.h"
#include "common/rect.h"
#include "common/math.h"
#include "common/textconsole.h"
#include "graphics/primitives.h"
#include "graphics/transparent_surface.h"
#include "graphics/transform_tools.h"

//#define ENABLE_BILINEAR

namespace Graphics {

static const int kBModShift = 0;//img->format.bShift;
static const int kGModShift = 8;//img->format.gShift;
static const int kRModShift = 16;//img->format.rShift;
static const int kAModShift = 24;//img->format.aShift;

#ifdef SCUMM_LITTLE_ENDIAN
static const int kAIndex = 0;
static const int kBIndex = 1;
static const int kGIndex = 2;
static const int kRIndex = 3;

#else
static const int kAIndex = 3;
static const int kBIndex = 2;
static const int kGIndex = 1;
static const int kRIndex = 0;
#endif

void doBlitOpaqueFast(byte *ino, byte *outo, uint32 width, uint32 height, uint32 pitch, int32 inStep, int32 inoStep);
void doBlitBinaryFast(byte *ino, byte *outo, uint32 width, uint32 height, uint32 pitch, int32 inStep, int32 inoStep);
void doBlitAlphaBlend(byte *ino, byte *outo, uint32 width, uint32 height, uint32 pitch, int32 inStep, int32 inoStep, uint32 color);
void doBlitAdditiveBlend(byte *ino, byte *outo, uint32 width, uint32 height, uint32 pitch, int32 inStep, int32 inoStep, uint32 color);
void doBlitSubtractiveBlend(byte *ino, byte *outo, uint32 width, uint32 height, uint32 pitch, int32 inStep, int32 inoStep, uint32 color);

TransparentSurface::TransparentSurface() : Surface(), _alphaMode(ALPHA_FULL) {}

TransparentSurface::TransparentSurface(const Surface &surf, bool copyData) : Surface(surf), _alphaMode(ALPHA_FULL) {
	if (copyData)
		copyFrom(surf);
}

/**
 * Optimized version of doBlit to be used w/opaque blitting (no alpha).
 */
void doBlitOpaqueFast(byte *ino, byte *outo, uint32 width, uint32 height, uint32 pitch, int32 inStep, int32 inoStep) {

	byte *in;
	byte *out;

	for (uint32 i = 0; i < height; i++) {
		out = outo;
		in = ino;
		memcpy(out, in, width * 4);
		for (uint32 j = 0; j < width; j++) {
			out[kAIndex] = 0xFF;
			out += 4;
		}
		outo += pitch;
		ino += inoStep;
	}
}

/**
 * Optimized version of doBlit to be used w/binary blitting (blit or no-blit, no blending).
 */
void doBlitBinaryFast(byte *ino, byte *outo, uint32 width, uint32 height, uint32 pitch, int32 inStep, int32 inoStep) {

	byte *in;
	byte *out;

	for (uint32 i = 0; i < height; i++) {
		out = outo;
		in = ino;
		for (uint32 j = 0; j < width; j++) {
			uint32 pix = *(uint32 *)in;
			int a = in[kAIndex];

			if (a != 0) {   // Full opacity (Any value not exactly 0 is Opaque here)
				*(uint32 *)out = pix;
				out[kAIndex] = 0xFF;
			}
			out += 4;
			in += inStep;
		}
		outo += pitch;
		ino += inoStep;
	}
}

/**
 * Optimized version of doBlit to be used with alpha blended blitting
 * @param ino a pointer to the input surface
 * @param outo a pointer to the output surface
 * @param width width of the input surface
 * @param height height of the input surface
 * @param pitch pitch of the output surface - that is, width in bytes of every row, usually bpp * width of the TARGET surface (the area we are blitting to might be smaller, do the math)
 * @inStep size in bytes to skip to address each pixel, usually bpp of the source surface
 * @inoStep width in bytes of every row on the *input* surface / kind of like pitch
 * @color colormod in 0xAARRGGBB format - 0xFFFFFFFF for no colormod
 */
void doBlitAlphaBlend(byte *ino, byte *outo, uint32 width, uint32 height, uint32 pitch, int32 inStep, int32 inoStep, uint32 color) {
	byte *in;
	byte *out;

	if (color == 0xffffffff) {

		for (uint32 i = 0; i < height; i++) {
			out = outo;
			in = ino;
			for (uint32 j = 0; j < width; j++) {

				if (in[kAIndex] != 0) {
					out[kAIndex] = 255;
					out[kRIndex] = ((in[kRIndex] * in[kAIndex]) + out[kRIndex] * (255 - in[kAIndex])) >> 8;
					out[kGIndex] = ((in[kGIndex] * in[kAIndex]) + out[kGIndex] * (255 - in[kAIndex])) >> 8;
					out[kBIndex] = ((in[kBIndex] * in[kAIndex]) + out[kBIndex] * (255 - in[kAIndex])) >> 8;
				}

				in += inStep;
				out += 4;
			}
			outo += pitch;
			ino += inoStep;
		}
	} else {

		byte ca = (color >> kAModShift) & 0xFF;
		byte cr = (color >> kRModShift) & 0xFF;
		byte cg = (color >> kGModShift) & 0xFF;
		byte cb = (color >> kBModShift) & 0xFF;

		for (uint32 i = 0; i < height; i++) {
			out = outo;
			in = ino;
			for (uint32 j = 0; j < width; j++) {

				uint32 ina = in[kAIndex] * ca >> 8;
				out[kAIndex] = 255;
				out[kBIndex] = (out[kBIndex] * (255 - ina) >> 8);
				out[kGIndex] = (out[kGIndex] * (255 - ina) >> 8);
				out[kRIndex] = (out[kRIndex] * (255 - ina) >> 8);

				out[kBIndex] = out[kBIndex] + (in[kBIndex] * ina * cb >> 16);
				out[kGIndex] = out[kGIndex] + (in[kGIndex] * ina * cg >> 16);
				out[kRIndex] = out[kRIndex] + (in[kRIndex] * ina * cr >> 16);

				in += inStep;
				out += 4;
			}
			outo += pitch;
			ino += inoStep;
		}
	}
}

/**
 * Optimized version of doBlit to be used with additive blended blitting
 */
void doBlitAdditiveBlend(byte *ino, byte *outo, uint32 width, uint32 height, uint32 pitch, int32 inStep, int32 inoStep, uint32 color) {
	byte *in;
	byte *out;

	if (color == 0xffffffff) {

		for (uint32 i = 0; i < height; i++) {
			out = outo;
			in = ino;
			for (uint32 j = 0; j < width; j++) {

				if (in[kAIndex] != 0) {
					out[kRIndex] = MIN((in[kRIndex] * in[kAIndex] >> 8) + out[kRIndex], 255);
					out[kGIndex] = MIN((in[kGIndex] * in[kAIndex] >> 8) + out[kGIndex], 255);
					out[kBIndex] = MIN((in[kBIndex] * in[kAIndex] >> 8) + out[kBIndex], 255);
				}

				in += inStep;
				out += 4;
			}
			outo += pitch;
			ino += inoStep;
		}
	} else {

		byte ca = (color >> kAModShift) & 0xFF;
		byte cr = (color >> kRModShift) & 0xFF;
		byte cg = (color >> kGModShift) & 0xFF;
		byte cb = (color >> kBModShift) & 0xFF;

		for (uint32 i = 0; i < height; i++) {
			out = outo;
			in = ino;
			for (uint32 j = 0; j < width; j++) {

				uint32 ina = in[kAIndex] * ca >> 8;

				if (cb != 255) {
					out[kBIndex] = MIN<uint>(out[kBIndex] + ((in[kBIndex] * cb * ina) >> 16), 255u);
				} else {
					out[kBIndex] = MIN<uint>(out[kBIndex] + (in[kBIndex] * ina >> 8), 255u);
				}

				if (cg != 255) {
					out[kGIndex] = MIN<uint>(out[kGIndex] + ((in[kGIndex] * cg * ina) >> 16), 255u);
				} else {
					out[kGIndex] = MIN<uint>(out[kGIndex] + (in[kGIndex] * ina >> 8), 255u);
				}

				if (cr != 255) {
					out[kRIndex] = MIN<uint>(out[kRIndex] + ((in[kRIndex] * cr * ina) >> 16), 255u);
				} else {
					out[kRIndex] = MIN<uint>(out[kRIndex] + (in[kRIndex] * ina >> 8), 255u);
				}

				in += inStep;
				out += 4;
			}
			outo += pitch;
			ino += inoStep;
		}
	}
}

/**
 * Optimized version of doBlit to be used with subtractive blended blitting
 */
void doBlitSubtractiveBlend(byte *ino, byte *outo, uint32 width, uint32 height, uint32 pitch, int32 inStep, int32 inoStep, uint32 color) {
	byte *in;
	byte *out;

	if (color == 0xffffffff) {

		for (uint32 i = 0; i < height; i++) {
			out = outo;
			in = ino;
			for (uint32 j = 0; j < width; j++) {

				if (in[kAIndex] != 0) {
					out[kRIndex] = MAX(out[kRIndex] - ((in[kRIndex] * out[kRIndex]) * in[kAIndex] >> 16), 0);
					out[kGIndex] = MAX(out[kGIndex] - ((in[kGIndex] * out[kGIndex]) * in[kAIndex] >> 16), 0);
					out[kBIndex] = MAX(out[kBIndex] - ((in[kBIndex] * out[kBIndex]) * in[kAIndex] >> 16), 0);
				}

				in += inStep;
				out += 4;
			}
			outo += pitch;
			ino += inoStep;
		}
	} else {

		byte cr = (color >> kRModShift) & 0xFF;
		byte cg = (color >> kGModShift) & 0xFF;
		byte cb = (color >> kBModShift) & 0xFF;

		for (uint32 i = 0; i < height; i++) {
			out = outo;
			in = ino;
			for (uint32 j = 0; j < width; j++) {

				out[kAIndex] = 255;
				if (cb != 255) {
					out[kBIndex] = MAX(out[kBIndex] - ((in[kBIndex] * cb  * (out[kBIndex]) * in[kAIndex]) >> 24), 0);
				} else {
					out[kBIndex] = MAX(out[kBIndex] - (in[kBIndex] * (out[kBIndex]) * in[kAIndex] >> 16), 0);
				}

				if (cg != 255) {
					out[kGIndex] = MAX(out[kGIndex] - ((in[kGIndex] * cg  * (out[kGIndex]) * in[kAIndex]) >> 24), 0);
				} else {
					out[kGIndex] = MAX(out[kGIndex] - (in[kGIndex] * (out[kGIndex]) * in[kAIndex] >> 16), 0);
				}

				if (cr != 255) {
					out[kRIndex] = MAX(out[kRIndex] - ((in[kRIndex] * cr * (out[kRIndex]) * in[kAIndex]) >> 24), 0);
				} else {
					out[kRIndex] = MAX(out[kRIndex] - (in[kRIndex] * (out[kRIndex]) * in[kAIndex] >> 16), 0);
				}

				in += inStep;
				out += 4;
			}
			outo += pitch;
			ino += inoStep;
		}
	}
}

Common::Rect TransparentSurface::blit(Graphics::Surface &target, int posX, int posY, int flipping, Common::Rect *pPartRect, uint color, int width, int height, TSpriteBlendMode blendMode) {
	// Check if we need to draw anything at all
	int ca = (color >> kAModShift) & 0xff;

	if (ca == 0)
		return Common::Rect();

	// TODO: Is the data really in the screen format?
	if (getFormat().bytesPerPixel != 4) {
		warning("TransparentSurface can only blit 32bpp images, but got %d", getFormat().bytesPerPixel * 8);
		return Common::Rect();
	}

	// Create an encapsulating surface for the data
	TransparentSurface srcImage(*this);

	if (pPartRect) {
		int xOffset = pPartRect->left;
		int yOffset = pPartRect->top;

		if (flipping & FLIP_V) {
			yOffset = srcImage.getHeight() - pPartRect->bottom;
		}

		if (flipping & FLIP_H) {
			xOffset = srcImage.getWidth() - pPartRect->right;
		}

		srcImage = getSubArea(Common::Rect(xOffset, yOffset, xOffset + pPartRect->width(), yOffset + pPartRect->height()));

		debug(6, "Blit(%d, %d, %d, [%d, %d, %d, %d], %08x, %d, %d)", posX, posY, flipping,
			  pPartRect->left, pPartRect->top, pPartRect->width(), pPartRect->height(), color, width, height);
	} else {

		debug(6, "Blit(%d, %d, %d, [%d, %d, %d, %d], %08x, %d, %d)", posX, posY, flipping, 0, 0,
			  srcImage.getWidth(), srcImage.getHeight(), color, width, height);
	}

	if (width == -1) {
		width = srcImage.getWidth();
	}
	if (height == -1) {
		height = srcImage.getHeight();
	}

#ifdef SCALING_TESTING
	// Hardcode scaling to 66% to test scaling
	width = width * 2 / 3;
	height = height * 2 / 3;
#endif

	Surface img;
	if ((width != srcImage.getWidth()) || (height != srcImage.getHeight())) {
		// Scale the image
		Common::ScopedPtr<Surface> scaledImage(srcImage.scale(width, height));
		img = *scaledImage;
	} else {
		img = srcImage;
	}

	// Handle off-screen clipping
	if (posY < 0) {
		img = img.getSubArea(Common::Rect(0, -posY, img.getWidth(), MAX(0, posY + img.getHeight()) - posY));
		posY = 0;
	}

	if (posX < 0) {
		img = img.getSubArea(Common::Rect(-posX, 0, MAX(0, posX + img.getWidth()) - posX, img.getHeight()));
		posX = 0;
	}

	uint realWidth = CLIP((int)img.getWidth(), 0, (int)MAX((int)target.getWidth() - posX, 0));
	uint realHeight = CLIP((int)img.getHeight(), 0, (int)MAX((int)target.getHeight() - posY, 0));
	if (realWidth < img.getWidth() || realHeight < img.getHeight())
		img = img.getSubArea(Common::Rect(realWidth, realHeight));

	if (img.getWidth() != 0 && img.getHeight() != 0) {
		int xp = 0, yp = 0;

		int inStep = 4;
		int inoStep = img.getPitch();
		if (flipping & FLIP_H) {
			inStep = -inStep;
			xp = img.getWidth() - 1;
		}

		if (flipping & FLIP_V) {
			inoStep = -inoStep;
			yp = img.getHeight() - 1;
		}

		byte *ino = (byte *)img.getBasePtr(xp, yp);
		byte *outo = (byte *)target.getBasePtr(posX, posY);

		if (color == 0xFFFFFFFF && blendMode == BLEND_NORMAL && _alphaMode == ALPHA_OPAQUE) {
			doBlitOpaqueFast(ino, outo, img.getWidth(), img.getHeight(), target.getPitch(), inStep, inoStep);
		} else if (color == 0xFFFFFFFF && blendMode == BLEND_NORMAL && _alphaMode == ALPHA_BINARY) {
			doBlitBinaryFast(ino, outo, img.getWidth(), img.getHeight(), target.getPitch(), inStep, inoStep);
		} else if (blendMode == BLEND_ADDITIVE) {
			doBlitAdditiveBlend(ino, outo, img.getWidth(), img.getHeight(), target.getPitch(), inStep, inoStep, color);
		} else if (blendMode == BLEND_SUBTRACTIVE) {
			doBlitSubtractiveBlend(ino, outo, img.getWidth(), img.getHeight(), target.getPitch(), inStep, inoStep, color);
		} else {
			assert(blendMode == BLEND_NORMAL);
			doBlitAlphaBlend(ino, outo, img.getWidth(), img.getHeight(), target.getPitch(), inStep, inoStep, color);
		}
	}

	return Common::Rect(img.getWidth(), img.getHeight());
}

/**
 * Writes a color key to the alpha channel of the surface
 * @param rKey  the red component of the color key
 * @param gKey  the green component of the color key
 * @param bKey  the blue component of the color key
 * @param overwriteAlpha if true, all other alpha will be set fully opaque
 */
void TransparentSurface::applyColorKey(uint8 rKey, uint8 gKey, uint8 bKey, bool overwriteAlpha) {
	assert(getFormat().bytesPerPixel == 4);
	for (int i = 0; i < getHeight(); i++) {
		for (int j = 0; j < getWidth(); j++) {
			uint32 pix = ((uint32 *)getPixels())[i * getWidth() + j];
			uint8 r, g, b, a;
			getFormat().colorToARGB(pix, a, r, g, b);
			if (r == rKey && g == gKey && b == bKey) {
				a = 0;
				((uint32 *)getPixels())[i * getWidth() + j] = getFormat().ARGBToColor(a, r, g, b);
			} else if (overwriteAlpha) {
				a = 255;
				((uint32 *)getPixels())[i * getWidth() + j] = getFormat().ARGBToColor(a, r, g, b);
			}
		}
	}
}

AlphaType TransparentSurface::getAlphaMode() const {
	return _alphaMode;
}

void TransparentSurface::setAlphaMode(AlphaType mode) {
	_alphaMode = mode;
}






/*

The below two functions are adapted from SDL_rotozoom.c,
taken from SDL_gfx-2.0.18.

Its copyright notice:

=============================================================================
SDL_rotozoom.c: rotozoomer, zoomer and shrinker for 32bit or 8bit surfaces

Copyright (C) 2001-2012  Andreas Schiffler

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.

Andreas Schiffler -- aschiffler at ferzkopp dot net
=============================================================================


The functions have been adapted for different structures and coordinate
systems.

*/





TransparentSurface *TransparentSurface::rotoscale(const TransformStruct &transform) const {
	assert(transform._angle != 0); // This would not be ideal; rotoscale() should never be called in conditional branches where angle = 0 anyway.

	Common::Point newHotspot;
	Common::Rect srcRect(0, 0, getWidth(), getHeight());
	Common::Rect rect = TransformTools::newRect(Common::Rect(srcRect), transform, &newHotspot);
	Common::Rect dstRect(0, 0, rect.right - rect.left, rect.bottom - rect.top);

	TransparentSurface *target = new TransparentSurface();
	assert(getFormat().bytesPerPixel == 4);

	int srcW = getWidth();
	int srcH = getHeight();
	int dstW = dstRect.width();
	int dstH = dstRect.height();

	target->create((uint16)dstW, (uint16)dstH, getFormat());

	if (transform._zoom.x == 0 || transform._zoom.y == 0) {
		return target;
	}

	uint32 invAngle = 360 - (transform._angle % 360);
	float invCos = cos(invAngle * M_PI / 180.0);
	float invSin = sin(invAngle * M_PI / 180.0);

	struct tColorRGBA { byte r; byte g; byte b; byte a; };
	int icosx = (int)(invCos * (65536.0f * kDefaultZoomX / transform._zoom.x));
	int isinx = (int)(invSin * (65536.0f * kDefaultZoomX / transform._zoom.x));
	int icosy = (int)(invCos * (65536.0f * kDefaultZoomY / transform._zoom.y));
	int isiny = (int)(invSin * (65536.0f * kDefaultZoomY / transform._zoom.y));


	bool flipx = false, flipy = false; // TODO: See mirroring comment in RenderTicket ctor

	int xd = (srcRect.left + transform._hotspot.x) << 16;
	int yd = (srcRect.top + transform._hotspot.y) << 16;
	int cx = newHotspot.x;
	int cy = newHotspot.y;

	int ax = -icosx * cx;
	int ay = -isiny * cx;
	int sw = srcW - 1;
	int sh = srcH - 1;

	tColorRGBA *pc = (tColorRGBA*)target->getBasePtr(0, 0);

	for (int y = 0; y < dstH; y++) {
		int t = cy - y;
		int sdx = ax + (isinx * t) + xd;
		int sdy = ay - (icosy * t) + yd;
		for (int x = 0; x < dstW; x++) {
			int dx = (sdx >> 16);
			int dy = (sdy >> 16);
			if (flipx) {
				dx = sw - dx;
			}
			if (flipy) {
				dy = sh - dy;
			}

#ifdef ENABLE_BILINEAR
			if ((dx > -1) && (dy > -1) && (dx < sw) && (dy < sh)) {
				const tColorRGBA *sp = (const tColorRGBA *)getBasePtr(dx, dy);
				tColorRGBA c00, c01, c10, c11, cswap;
				c00 = *sp;
				sp += 1;
				c01 = *sp;
				sp += (this->pitch / 4);
				c11 = *sp;
				sp -= 1;
				c10 = *sp;
				if (flipx) {
					cswap = c00; c00=c01; c01=cswap;
					cswap = c10; c10=c11; c11=cswap;
				}
				if (flipy) {
					cswap = c00; c00=c10; c10=cswap;
					cswap = c01; c01=c11; c11=cswap;
				}
				/*
				* Interpolate colors
				*/
				int ex = (sdx & 0xffff);
				int ey = (sdy & 0xffff);
				int t1, t2;
				t1 = ((((c01.r - c00.r) * ex) >> 16) + c00.r) & 0xff;
				t2 = ((((c11.r - c10.r) * ex) >> 16) + c10.r) & 0xff;
				pc->r = (((t2 - t1) * ey) >> 16) + t1;
				t1 = ((((c01.g - c00.g) * ex) >> 16) + c00.g) & 0xff;
				t2 = ((((c11.g - c10.g) * ex) >> 16) + c10.g) & 0xff;
				pc->g = (((t2 - t1) * ey) >> 16) + t1;
				t1 = ((((c01.b - c00.b) * ex) >> 16) + c00.b) & 0xff;
				t2 = ((((c11.b - c10.b) * ex) >> 16) + c10.b) & 0xff;
				pc->b = (((t2 - t1) * ey) >> 16) + t1;
				t1 = ((((c01.a - c00.a) * ex) >> 16) + c00.a) & 0xff;
				t2 = ((((c11.a - c10.a) * ex) >> 16) + c10.a) & 0xff;
				pc->a = (((t2 - t1) * ey) >> 16) + t1;
			}
#else
			if ((dx >= 0) && (dy >= 0) && (dx < srcW) && (dy < srcH)) {
				const tColorRGBA *sp = (const tColorRGBA *)getBasePtr(dx, dy);
				*pc = *sp;
			}
#endif
			sdx += icosx;
			sdy += isiny;
			pc++;
		}
	}
	return target;
}

TransparentSurface *TransparentSurface::scale(uint16 newWidth, uint16 newHeight) const {
	Common::Rect srcRect(0, 0, getWidth(), getHeight());
	Common::Rect dstRect(0, 0, newWidth, newHeight);

	TransparentSurface *target = new TransparentSurface();

	assert(getFormat().bytesPerPixel == 4);

	int srcW = srcRect.width();
	int srcH = srcRect.height();
	int dstW = dstRect.width();
	int dstH = dstRect.height();

	target->create((uint16)dstW, (uint16)dstH, getFormat());

#ifdef ENABLE_BILINEAR

	// NB: The actual order of these bytes may not be correct, but
	// since all values are treated equal, that does not matter.
	struct tColorRGBA { byte r; byte g; byte b; byte a; };

	bool flipx = false, flipy = false; // TODO: See mirroring comment in RenderTicket ctor


	int *sax = new int[dstW + 1];
	int *say = new int[dstH + 1];
	assert(sax && say);

	/*
	* Precalculate row increments
	*/
	int spixelw = (srcW - 1);
	int spixelh = (srcH - 1);
	int sx = (int) (65536.0f * (float) spixelw / (float) (dstW - 1));
	int sy = (int) (65536.0f * (float) spixelh / (float) (dstH - 1));

	/* Maximum scaled source size */
	int ssx = (srcW << 16) - 1;
	int ssy = (srcH << 16) - 1;

	/* Precalculate horizontal row increments */
	int csx = 0;
	int *csax = sax;
	for (int x = 0; x <= dstW; x++) {
		*csax = csx;
		csax++;
		csx += sx;

		/* Guard from overflows */
		if (csx > ssx) {
			csx = ssx;
		}
	}

	/* Precalculate vertical row increments */
	int csy = 0;
	int *csay = say;
	for (int y = 0; y <= dstH; y++) {
		*csay = csy;
		csay++;
		csy += sy;

		/* Guard from overflows */
		if (csy > ssy) {
			csy = ssy;
		}
	}

	const tColorRGBA *sp = (const tColorRGBA *) getBasePtr(0, 0);
	tColorRGBA *dp = (tColorRGBA *) target->getBasePtr(0, 0);
	int spixelgap = srcW;

	if (flipx) {
		sp += spixelw;
	}
	if (flipy) {
		sp += spixelgap * spixelh;
	}

	csay = say;
	for (int y = 0; y < dstH; y++) {
		const tColorRGBA *csp = sp;
		csax = sax;
		for (int x = 0; x < dstW; x++) {
			/*
			* Setup color source pointers
			*/
			int ex = (*csax & 0xffff);
			int ey = (*csay & 0xffff);
			int cx = (*csax >> 16);
			int cy = (*csay >> 16);

			const tColorRGBA *c00, *c01, *c10, *c11;
			c00 = sp;
			c01 = sp;
			c10 = sp;
			if (cy < spixelh) {
				if (flipy) {
					c10 -= spixelgap;
				} else {
					c10 += spixelgap;
				}
			}
			c11 = c10;
			if (cx < spixelw) {
				if (flipx) {
					c01--;
					c11--;
				} else {
					c01++;
					c11++;
				}
			}

			/*
			* Draw and interpolate colors
			*/
			int t1, t2;
			t1 = ((((c01->r - c00->r) * ex) >> 16) + c00->r) & 0xff;
			t2 = ((((c11->r - c10->r) * ex) >> 16) + c10->r) & 0xff;
			dp->r = (((t2 - t1) * ey) >> 16) + t1;
			t1 = ((((c01->g - c00->g) * ex) >> 16) + c00->g) & 0xff;
			t2 = ((((c11->g - c10->g) * ex) >> 16) + c10->g) & 0xff;
			dp->g = (((t2 - t1) * ey) >> 16) + t1;
			t1 = ((((c01->b - c00->b) * ex) >> 16) + c00->b) & 0xff;
			t2 = ((((c11->b - c10->b) * ex) >> 16) + c10->b) & 0xff;
			dp->b = (((t2 - t1) * ey) >> 16) + t1;
			t1 = ((((c01->a - c00->a) * ex) >> 16) + c00->a) & 0xff;
			t2 = ((((c11->a - c10->a) * ex) >> 16) + c10->a) & 0xff;
			dp->a = (((t2 - t1) * ey) >> 16) + t1;

			/*
			* Advance source pointer x
			*/
			int *salastx = csax;
			csax++;
			int sstepx = (*csax >> 16) - (*salastx >> 16);
			if (flipx) {
				sp -= sstepx;
			} else {
				sp += sstepx;
			}

			/*
			* Advance destination pointer x
			*/
			dp++;
		}
		/*
		* Advance source pointer y
		*/
		int *salasty = csay;
		csay++;
		int sstepy = (*csay >> 16) - (*salasty >> 16);
		sstepy *= spixelgap;
		if (flipy) {
			sp = csp - sstepy;
		} else {
			sp = csp + sstepy;
		}
	}

	delete[] sax;
	delete[] say;

#else

	int *scaleCacheX = new int[dstW];
	for (int x = 0; x < dstW; x++) {
		scaleCacheX[x] = (x * srcW) / dstW;
	}

	for (int y = 0; y < dstH; y++) {
		uint32 *destP = (uint32 *)target->getBasePtr(0, y);
		const uint32 *srcP = (const uint32 *)getBasePtr(0, (y * srcH) / dstH);
		for (int x = 0; x < dstW; x++) {
			*destP++ = srcP[scaleCacheX[x]];
		}
	}
	delete[] scaleCacheX;

#endif

	return target;

}

} // End of namespace Graphics
