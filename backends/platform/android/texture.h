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

#ifndef _ANDROID_TEXTURE_H_
#define _ANDROID_TEXTURE_H_

#if defined(__ANDROID__)

#include <GLES/gl.h>

#include "graphics/surface.h"
#include "graphics/pixelformat.h"

#include "common/rect.h"
#include "common/array.h"

class GLESBaseTexture {
public:
	static void initGLExtensions();

protected:
	GLESBaseTexture(GLenum glFormat, GLenum glType,
					Graphics::PixelFormat pixelFormat);

public:
	virtual ~GLESBaseTexture();

	void release();
	void reinit();
	void initSize();

	void setLinearFilter(bool value);

	virtual void allocBuffer(GLuint w, GLuint h) = 0;

	virtual void updateBuffer(GLuint x, GLuint y, GLuint width, GLuint height,
								const void *buf, int pitch_buf) = 0;
	virtual void fillBuffer(uint32 color) = 0;

	virtual void drawTexture(GLshort x, GLshort y, GLshort w, GLshort h);

	inline void setDrawRect(const Common::Rect &rect) {
		_draw_rect = rect;
	}

	inline void setDrawRect(int16 w, int16 h) {
		_draw_rect = Common::Rect(w, h);
	}

	inline void setDrawRect(int16 x1, int16 y1, int16 x2, int16 y2) {
		_draw_rect = Common::Rect(x1, y1, x2, y2);
	}

	inline const Common::Rect &getDrawRect() const {
		return _draw_rect;
	}

	inline void drawTextureRect() {
		drawTexture(_draw_rect.left, _draw_rect.top,
					_draw_rect.width(), _draw_rect.height());
	}

	inline void drawTextureOrigin() {
		drawTexture(0, 0, _surface.getWidth(), _surface.getHeight());
	}

	inline GLuint width() const {
		return _surface.getWidth();
	}

	inline GLuint height() const {
		return _surface.getHeight();
	}

	inline uint16 pitch() const {
		return _surface.getPitch();
	}

	inline bool isEmpty() const {
		return _surface.getWidth() == 0 || _surface.getHeight() == 0;
	}

	inline const Graphics::Surface *surface_const() const {
		return &_surface;
	}

	inline Graphics::Surface *surface() {
		setDirty();
		return &_surface;
	}

	virtual const byte *palette_const() const {
		return 0;
	};

	virtual byte *palette() {
		return 0;
	};

	inline bool hasPalette() const {
		return _palettePixelFormat.bytesPerPixel > 0;
	}

	inline bool dirty() const {
		return _all_dirty || !_dirty_rect.isEmpty();
	}

	virtual const Graphics::PixelFormat &getPixelFormat() const;

	inline const Graphics::PixelFormat &getPalettePixelFormat() const {
		return _palettePixelFormat;
	}

protected:
	inline void setDirty() {
		_all_dirty = true;
	}

	inline void clearDirty() {
		_all_dirty = false;
		_dirty_rect.top = 0;
		_dirty_rect.left = 0;
		_dirty_rect.bottom = 0;
		_dirty_rect.right = 0;
	}

	inline void setDirtyRect(const Common::Rect& r) {
		if (!_all_dirty) {
			if (_dirty_rect.isEmpty())
				_dirty_rect = r;
			else
				_dirty_rect.extend(r);
		}
	}

	void allocTexture(GLuint w, GLuint h);

	GLenum _glFormat;
	GLenum _glType;
	GLint _glFilter;

	GLuint _texture_name;
	Graphics::Surface _surface;
	GLuint _texture_width;
	GLuint _texture_height;

	Common::Rect _draw_rect;

	bool _all_dirty;
	Common::Rect _dirty_rect;

	Graphics::PixelFormat _pixelFormat;
	Graphics::PixelFormat _palettePixelFormat;
};

class GLESTexture : public GLESBaseTexture {
protected:
	GLESTexture(GLenum glFormat, GLenum glType,
				Graphics::PixelFormat pixelFormat);

public:
	virtual ~GLESTexture();

	virtual void allocBuffer(GLuint w, GLuint h);

	virtual void updateBuffer(GLuint x, GLuint y, GLuint width, GLuint height,
								const void *buf, int pitch_buf);
	virtual void fillBuffer(uint32 color);

	virtual void drawTexture(GLshort x, GLshort y, GLshort w, GLshort h);

protected:
	byte *_pixels;
	byte *_buf;
};

// RGBA4444 texture
class GLES4444Texture : public GLESTexture {
public:
	GLES4444Texture();
	virtual ~GLES4444Texture();

	static Graphics::PixelFormat pixelFormat() {
		return Graphics::PixelFormat(2, 4, 4, 4, 4, 12, 8, 4, 0);
	}
};

// RGBA5551 texture
class GLES5551Texture : public GLESTexture {
public:
	GLES5551Texture();
	virtual ~GLES5551Texture();

	static inline Graphics::PixelFormat pixelFormat() {
		return Graphics::PixelFormat(2, 5, 5, 5, 1, 11, 6, 1, 0);
	}
};

// RGB565 texture
class GLES565Texture : public GLESTexture {
public:
	GLES565Texture();
	virtual ~GLES565Texture();

	static inline Graphics::PixelFormat pixelFormat() {
		return Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0);
	}
};

// RGBA8888 texture
class GLES8888Texture : public GLESTexture {
public:
	GLES8888Texture();
	virtual ~GLES8888Texture();

	static inline Graphics::PixelFormat pixelFormat() {
		// We assume LE since all Android platforms are LE.
		return Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24);
	}
};

class GLESFakePaletteTexture : public GLESBaseTexture {
protected:
	GLESFakePaletteTexture(GLenum glFormat, GLenum glType,
							Graphics::PixelFormat pixelFormat);

public:
	virtual ~GLESFakePaletteTexture();

	virtual void allocBuffer(GLuint w, GLuint h);
	virtual void updateBuffer(GLuint x, GLuint y, GLuint width, GLuint height,
								const void *buf, int pitch_buf);
	virtual void fillBuffer(uint32 color);

	virtual void drawTexture(GLshort x, GLshort y, GLshort w, GLshort h);

	virtual const byte *palette_const() const {
		return (byte *)_palette;
	};

	virtual byte *palette() {
		setDirty();
		return (byte *)_palette;
	};

	virtual const Graphics::PixelFormat &getPixelFormat() const;

protected:
	Graphics::PixelFormat _fake_format;
	uint16 *_palette;
	byte *_pixels;
	uint16 *_buf;
};

class GLESFakePalette565Texture : public GLESFakePaletteTexture {
public:
	GLESFakePalette565Texture();
	virtual ~GLESFakePalette565Texture();
};

class GLESFakePalette5551Texture : public GLESFakePaletteTexture {
public:
	GLESFakePalette5551Texture();
	virtual ~GLESFakePalette5551Texture();
};

#endif
#endif
