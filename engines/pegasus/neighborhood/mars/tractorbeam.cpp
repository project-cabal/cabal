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

// Additional copyright for this file:
// Copyright (C) 1995-1997 Presto Studios, Inc.

#include "pegasus/pegasus.h"
#include "pegasus/neighborhood/mars/constants.h"
#include "pegasus/neighborhood/mars/tractorbeam.h"

namespace Pegasus {

TractorBeam::TractorBeam() : DisplayElement(kNoDisplayElement) {
	setBounds(kShuttleTractorLeft, kShuttleTractorTop, kShuttleTractorLeft + kShuttleTractorWidth,
			kShuttleTractorTop + kShuttleTractorHeight);
	setDisplayOrder(kShuttleTractorBeamOrder);

}

static const int kHalfWidth = kShuttleTractorWidth >> 1;
static const int kHalfHeight = kShuttleTractorHeight >> 1;

static const int kW3Vert = kHalfHeight * kHalfHeight * kHalfHeight;
static const int kW3Div2Vert = kW3Vert >> 1;

static const int kW3Horiz = kHalfWidth * kHalfWidth * kHalfWidth;
static const int kW3Div2Horiz = kW3Horiz >> 1;

static const int kMaxLevel = 50;

static const int kAVert = -2 * kMaxLevel;
static const int kBVert = 3 * kMaxLevel * kHalfHeight;

#define READ_PIXEL(ptr) \
	if (screen->getFormat().bytesPerPixel == 2) \
		color = READ_UINT16(ptr); \
	else \
		color = READ_UINT32(ptr); \
	screen->getFormat().colorToRGB(color, r, g, b)

#define WRITE_PIXEL(ptr) \
	color = screen->getFormat().RGBToColor(r, g, b); \
	if (screen->getFormat().bytesPerPixel == 2) \
		WRITE_UINT16(ptr, color); \
	else \
		WRITE_UINT32(ptr, color)

#define DO_BLEND(ptr) \
	READ_PIXEL(ptr); \
	g += (((0xff - g) * blendHoriz) >> 8); \
	b += (((0xff - b) * blendHoriz) >> 8); \
	WRITE_PIXEL(ptr)

void TractorBeam::draw(const Common::Rect &) {
	Graphics::Surface *screen = ((PegasusEngine *)g_engine)->_gfx->getWorkArea();

	// Set up vertical DDA.
	int blendVert = 0;
	int dVert = 0;
	int d1Vert = kAVert + kBVert;
	int d2Vert = 6 * kAVert + 2 * kBVert;
	int d3Vert = 6 * kAVert;

	byte *rowPtrTop = (byte *)screen->getBasePtr(_bounds.left, _bounds.top);
	byte *rowPtrBottom = (byte *)screen->getBasePtr(_bounds.left, _bounds.top + ((kHalfHeight << 1) - 1));

	for (int y = kHalfHeight; y > 0; y--) {
		// Set up horizontal DDA
		int A = -2 * blendVert;
		int B = 3 * blendVert * kHalfWidth;
		int blendHoriz = 0;
		int dHoriz = 0;
		int d1Horiz = A + B;
		int d2Horiz = 6 * A + 2 * B;
		int d3Horiz = 6 * A;

		byte *pTopLeft = rowPtrTop;
		byte *pTopRight = rowPtrTop + (kHalfWidth * 2 - 1) * screen->getFormat().bytesPerPixel;
		byte *pBottomLeft = rowPtrBottom;
		byte *pBottomRight = rowPtrBottom + (kHalfWidth * 2 - 1) * screen->getFormat().bytesPerPixel;

		for (int x = kHalfWidth; x > 0; x--) {
			byte r, g, b;
			uint32 color;

			DO_BLEND(pTopLeft);
			DO_BLEND(pTopRight);
			DO_BLEND(pBottomLeft);
			DO_BLEND(pBottomRight);

			pTopLeft += screen->getFormat().bytesPerPixel;
			pBottomLeft += screen->getFormat().bytesPerPixel;
			pTopRight -= screen->getFormat().bytesPerPixel;
			pBottomRight -= screen->getFormat().bytesPerPixel;

			while (dHoriz > kW3Div2Horiz) {
				blendHoriz++;
				dHoriz -= kW3Horiz;
			}

			dHoriz += d1Horiz;
			d1Horiz += d2Horiz;
			d2Horiz += d3Horiz;
		}

		rowPtrTop += screen->getPitch();
		rowPtrBottom -= screen->getPitch();

		while (dVert > kW3Div2Vert) {
			blendVert++;
			dVert -= kW3Vert;
		}

		dVert += d1Vert;
		d1Vert += d2Vert;
		d2Vert += d3Vert;
	}
}

} // End of namespace Pegasus
