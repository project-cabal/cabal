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

#ifndef ACCESS_ASURFACE_H
#define ACCESS_ASURFACE_H

#include "common/scummsys.h"
#include "common/array.h"
#include "common/memstream.h"
#include "common/rect.h"
#include "graphics/surface.h"
#include "access/data.h"

namespace Access {

class SpriteResource;
class SpriteFrame;

class ASurface : public Graphics::Surface {
private:
	Graphics::Surface _savedBlock;

	void flipHorizontal(ASurface &dest);
protected:
	Common::Rect _savedBounds;
public:
	int _leftSkip, _rightSkip;
	int _topSkip, _bottomSkip;
	int _lastBoundsX, _lastBoundsY;
	int _lastBoundsW, _lastBoundsH;
	int _orgX1, _orgY1;
	int _orgX2, _orgY2;
	int _lColor;

	Common::Point _printOrg;
	Common::Point _printStart;
	int _maxChars;
public:
	static int _clipWidth, _clipHeight;
public:
	ASurface();

	virtual ~ASurface();

	void create(uint16 width, uint16 height);

	bool empty() const { return getWidth() == 0 || getHeight() == 0 || getPixels() == nullptr; }

	void clearBuffer();

	bool clip(Common::Rect &r);

	void plotImage(SpriteResource *sprite, int frameNum, const Common::Point &pt);

	/**
	 * Scaled draw frame in forward orientation
	 */
	void sPlotF(SpriteFrame *frame, const Common::Rect &bounds);

	/**
	 * Scaled draw frame in backwards orientation
	 */
	void sPlotB(SpriteFrame *frame, const Common::Rect &bounds);

	/**
	 * Draw an image full-size in forward orientation
	 */
	void plotF(SpriteFrame *frame, const Common::Point &pt);

	/**
	 * Draw an image full-size in backwards orientation
	 */
	void plotB(SpriteFrame *frame, const Common::Point &pt);

	virtual void copyBlock(ASurface *src, const Common::Rect &bounds);

	virtual void restoreBlock();

	virtual void drawRect();

	virtual void drawLine(int x1, int y1, int x2, int y2, int col);

	virtual void drawLine();

	virtual void drawBox();
	
	virtual void transBlitFrom(ASurface *src, const Common::Point &destPos);

	virtual void transBlitFrom(ASurface *src, const Common::Rect &bounds);

	virtual void transBlitFrom(ASurface &src);

	virtual void blitFrom(const Graphics::Surface &src);

	virtual void copyBuffer(Graphics::Surface *src);

	virtual void addDirtyRect(const Common::Rect &r) {}

	void copyTo(ASurface *dest);

	void saveBlock(const Common::Rect &bounds);

	void moveBufferLeft();

	void moveBufferRight();

	void moveBufferUp();

	void moveBufferDown();
};

class SpriteFrame : public ASurface {
public:
	SpriteFrame(AccessEngine *vm, Common::SeekableReadStream *stream, int frameSize);
	~SpriteFrame();
};

class SpriteResource {
public:
	Common::Array<SpriteFrame *> _frames;
public:
	SpriteResource(AccessEngine *vm, Resource *res);
	~SpriteResource();

	int getCount() { return _frames.size(); }

	SpriteFrame *getFrame(int idx) { return _frames[idx]; }
};

enum ImageFlag {
	IMGFLAG_CROPPED = 1,
	IMGFLAG_BACKWARDS = 2,
	IMGFLAG_DRAWN = 4,
	IMGFLAG_UNSCALED = 8
};

class ImageEntry {
public:
	int _frameNumber;
	SpriteResource *_spritesPtr;
	int _offsetY;
	Common::Point _position;
	int _flags;
public:
	ImageEntry();
};

class ImageEntryList : public Common::Array<ImageEntry> {
public:
	void addToList(ImageEntry &ie);
};

} // End of namespace Access

#endif /* ACCESS_ASURFACE_H */
