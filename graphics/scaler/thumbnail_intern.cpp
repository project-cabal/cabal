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

#include "common/endian.h"
#include "common/scummsys.h"
#include "common/system.h"

#include "graphics/colormasks.h"
#include "graphics/scaler.h"
#include "graphics/scaler/intern.h"
#include "graphics/palette.h"

template<int bitFormat>
uint16 quadBlockInterpolate(const uint8 *src, uint32 srcPitch) {
	uint16 colorx1y1 = *(((const uint16 *)src));
	uint16 colorx2y1 = *(((const uint16 *)src) + 1);

	uint16 colorx1y2 = *(((const uint16 *)(src + srcPitch)));
	uint16 colorx2y2 = *(((const uint16 *)(src + srcPitch)) + 1);

	return interpolate16_1_1_1_1<Graphics::ColorMasks<bitFormat> >(colorx1y1, colorx2y1, colorx1y2, colorx2y2);
}

template<int bitFormat>
void createThumbnail_2(const uint8 *src, uint32 srcPitch, uint8 *dstPtr, uint32 dstPitch, int width, int height) {
	// Make sure the width and height is a multiple of 2.
	width &= ~1;
	height &= ~1;

	for (int y = 0; y < height; y += 2) {
		for (int x = 0; x < width; x += 2, dstPtr += 2) {
			*((uint16 *)dstPtr) = quadBlockInterpolate<bitFormat>(src + 2 * x, srcPitch);
		}
		dstPtr += (dstPitch - 2 * width / 2);
		src += 2 * srcPitch;
	}
}

template<int bitFormat>
void createThumbnail_4(const uint8 *src, uint32 srcPitch, uint8 *dstPtr, uint32 dstPitch, int width, int height) {
	// Make sure the width and height is a multiple of 4
	width &= ~3;
	height &= ~3;

	for (int y = 0; y < height; y += 4) {
		for (int x = 0; x < width; x += 4, dstPtr += 2) {
			uint16 upleft = quadBlockInterpolate<bitFormat>(src + 2 * x, srcPitch);
			uint16 upright = quadBlockInterpolate<bitFormat>(src + 2 * (x + 2), srcPitch);
			uint16 downleft = quadBlockInterpolate<bitFormat>(src + srcPitch * 2 + 2 * x, srcPitch);
			uint16 downright = quadBlockInterpolate<bitFormat>(src + srcPitch * 2 + 2 * (x + 2), srcPitch);

			*((uint16 *)dstPtr) = interpolate16_1_1_1_1<Graphics::ColorMasks<bitFormat> >(upleft, upright, downleft, downright);
		}
		dstPtr += (dstPitch - 2 * width / 4);
		src += 4 * srcPitch;
	}
}

static void scaleThumbnail(Graphics::Surface &in, Graphics::Surface &out) {
	uint16 inWidth = in.getWidth();
	uint16 inHeight = in.getHeight();

	while (inWidth / out.getWidth() >= 4 || inHeight / out.getHeight() >= 4) {
		createThumbnail_4<565>((const uint8 *)in.getPixels(), in.getPitch(), (uint8 *)in.getPixels(), in.getPitch(), inWidth, inHeight);
		inWidth /= 4;
		inHeight /= 4;
	}

	while (inWidth / out.getWidth() >= 2 || inHeight / out.getHeight() >= 2) {
		createThumbnail_2<565>((const uint8 *)in.getPixels(), in.getPitch(), (uint8 *)in.getPixels(), in.getPitch(), inWidth, inHeight);
		inWidth /= 2;
		inHeight /= 2;
	}

	if ((inWidth == out.getWidth() && inHeight < out.getHeight()) || (inWidth < out.getWidth() && inHeight == out.getHeight())) {
		// In this case we simply center the input surface in the output
		uint8 *dst = (uint8 *)out.getBasePtr((out.getWidth() - inWidth) / 2, (out.getHeight() - inHeight) / 2);
		const uint8 *src = (const uint8 *)in.getPixels();

		for (int y = 0; y < inHeight; ++y) {
			memcpy(dst, src, inWidth * in.getFormat().bytesPerPixel);
			src += in.getPitch();
			dst += out.getPitch();
		}
	} else {
		// Assure the aspect of the scaled image still matches the original.
		int targetWidth = out.getWidth(), targetHeight = out.getHeight();

		const float inputAspect = (float)inWidth / inHeight;
		const float outputAspect = (float)out.getWidth() / out.getHeight();

		if (inputAspect > outputAspect) {
			targetHeight = int(targetWidth / inputAspect);
		} else if (inputAspect < outputAspect) {
			targetWidth = int(targetHeight * inputAspect);
		}

		// Make sure we are still in the bounds of the output
		assert(targetWidth <= out.getWidth());
		assert(targetHeight <= out.getHeight());

		// Center the image on the output surface
		byte *dst = (byte *)out.getBasePtr((out.getWidth() - targetWidth) / 2, (out.getHeight() - targetHeight) / 2);
		const uint dstLineIncrease = out.getPitch() - targetWidth * out.getFormat().bytesPerPixel;

		const float scaleFactorX = (float)targetWidth / inWidth;
		const float scaleFactorY = (float)targetHeight / inWidth;

		for (int y = 0; y < targetHeight; ++y) {
			const float yFrac = (y / scaleFactorY);
			const int y1 = (int)yFrac;
			const int y2 = (y1 + 1 < inHeight) ? (y1 + 1) : (inHeight - 1);

			for (int x = 0; x < targetWidth; ++x) {
				const float xFrac = (x / scaleFactorX);
				const int x1 = (int)xFrac;
				const int x2 = (x1 + 1 < inWidth) ? (x1 + 1) : (inWidth - 1);

				// Look up colors at the points
				uint8 p1R, p1G, p1B;
				Graphics::colorToRGB<Graphics::ColorMasks<565> >(READ_UINT16(in.getBasePtr(x1, y1)), p1R, p1G, p1B);
				uint8 p2R, p2G, p2B;
				Graphics::colorToRGB<Graphics::ColorMasks<565> >(READ_UINT16(in.getBasePtr(x2, y1)), p2R, p2G, p2B);
				uint8 p3R, p3G, p3B;
				Graphics::colorToRGB<Graphics::ColorMasks<565> >(READ_UINT16(in.getBasePtr(x1, y2)), p3R, p3G, p3B);
				uint8 p4R, p4G, p4B;
				Graphics::colorToRGB<Graphics::ColorMasks<565> >(READ_UINT16(in.getBasePtr(x2, y2)), p4R, p4G, p4B);

				const float xDiff = xFrac - x1;
				const float yDiff = yFrac - y1;

				uint8 pR = (uint8)((1 - yDiff) * ((1 - xDiff) * p1R + xDiff * p2R) + yDiff * ((1 - xDiff) * p3R + xDiff * p4R));
				uint8 pG = (uint8)((1 - yDiff) * ((1 - xDiff) * p1G + xDiff * p2G) + yDiff * ((1 - xDiff) * p3G + xDiff * p4G));
				uint8 pB = (uint8)((1 - yDiff) * ((1 - xDiff) * p1B + xDiff * p2B) + yDiff * ((1 - xDiff) * p3B + xDiff * p4B));

				WRITE_UINT16(dst, Graphics::RGBToColor<Graphics::ColorMasks<565> >(pR, pG, pB));
				dst += 2;
			}

			// Move to the next line
			dst = (byte *)dst + dstLineIncrease;
		}
	}
}


/**
 * Copies the current screen contents to a new surface, using RGB565 format.
 * WARNING: surf->free() must be called by the user to avoid leaking.
 *
 * @param surf      the surface to store the data in it
 */
static bool grabScreen565(Graphics::Surface *surf) {
	Graphics::Surface *screen = g_system->lockScreen();
	if (!screen)
		return false;

	assert(screen->getFormat().bytesPerPixel == 1 || screen->getFormat().bytesPerPixel == 2);
	assert(screen->getPixels() != 0);

	Graphics::PixelFormat screenFormat = g_system->getScreenFormat();

	surf->create(screen->getWidth(), screen->getHeight(), Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0));

	byte *palette = 0;
	if (screenFormat.bytesPerPixel == 1) {
		palette = new byte[256 * 3];
		assert(palette);
		g_system->getPaletteManager()->grabPalette(palette, 0, 256);
	}

	for (uint y = 0; y < screen->getHeight(); ++y) {
		for (uint x = 0; x < screen->getWidth(); ++x) {
			byte r = 0, g = 0, b = 0;

			if (screenFormat.bytesPerPixel == 1) {
				uint8 pixel = *(uint8 *)screen->getBasePtr(x, y);
				r = palette[pixel * 3 + 0];
				g = palette[pixel * 3 + 1];
				b = palette[pixel * 3 + 2];
			} else if (screenFormat.bytesPerPixel == 2) {
				uint16 col = READ_UINT16(screen->getBasePtr(x, y));
				screenFormat.colorToRGB(col, r, g, b);
			}

			*((uint16 *)surf->getBasePtr(x, y)) = Graphics::RGBToColor<Graphics::ColorMasks<565> >(r, g, b);
		}
	}

	delete[] palette;

	g_system->unlockScreen();
	return true;
}

static bool createThumbnail(Graphics::Surface &out, Graphics::Surface &in) {
	int height;
	if ((in.getWidth() == 320 && in.getHeight() == 200) || (in.getWidth() == 640 && in.getHeight() == 400)) {
		height = kThumbnailHeight1;
	} else {
		height = kThumbnailHeight2;
	}

	out.create(kThumbnailWidth, height, Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0));
	scaleThumbnail(in, out);
	in.reset();
	return true;
}

bool createThumbnailFromScreen(Graphics::Surface *surf) {
	assert(surf);

	Graphics::Surface screen;

	if (!grabScreen565(&screen))
		return false;

	return createThumbnail(*surf, screen);
}

bool createThumbnail(Graphics::Surface *surf, const uint8 *pixels, int w, int h, const uint8 *palette) {
	assert(surf);

	Graphics::Surface screen;
	screen.create(w, h, Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0));

	for (uint y = 0; y < screen.getHeight(); ++y) {
		for (uint x = 0; x < screen.getWidth(); ++x) {
			byte r, g, b;
			r = palette[pixels[y * w + x] * 3];
			g = palette[pixels[y * w + x] * 3 + 1];
			b = palette[pixels[y * w + x] * 3 + 2];

			*((uint16 *)screen.getBasePtr(x, y)) = Graphics::RGBToColor<Graphics::ColorMasks<565> >(r, g, b);
		}
	}

	return createThumbnail(*surf, screen);
}

// this is somewhat awkward, but createScreenShot should logically be in graphics,
// but moving other functions in this file into that namespace breaks several engines
namespace Graphics {
bool createScreenShot(Graphics::Surface &surf) {
	Graphics::PixelFormat screenFormat = g_system->getScreenFormat();
	//convert surface to 2 bytes pixel format to avoid problems with palette saving and loading
	if ((screenFormat.bytesPerPixel == 1) || (screenFormat.bytesPerPixel == 2)) {
		return grabScreen565(&surf);
	} else {
		Graphics::Surface *screen = g_system->lockScreen();
		if (!screen) {
			return false;
		}
		surf.create(screen->getWidth(), screen->getHeight(), Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0));
		for (uint y = 0; y < screen->getHeight(); ++y) {
			for (uint x = 0; x < screen->getWidth(); ++x) {
				byte r = 0, g = 0, b = 0, a = 0;
				uint32 col = READ_UINT32(screen->getBasePtr(x, y));
				screenFormat.colorToARGB(col, a, r, g, b);
				*((uint32 *)surf.getBasePtr(x, y)) = Graphics::ARGBToColor<Graphics::ColorMasks<8888> >(a, r, g, b);
			}
		}
		g_system->unlockScreen();
		return true;
	}
}
} // End of namespace Graphics
