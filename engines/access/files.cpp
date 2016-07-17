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

#include "common/substream.h"
#include "access/files.h"
#include "access/amazon/amazon_resources.h"
#include "access/martian/martian_resources.h"
#include "access/access.h"

namespace Access {

FileIdent::FileIdent() {
	_fileNum = -1;
	_subfile = 0;
}

void FileIdent::load(Common::SeekableReadStream &s) {
	_fileNum = s.readSint16LE();
	_subfile = s.readUint16LE();
}

/*------------------------------------------------------------------------*/

CellIdent::	CellIdent() {
	_cell = 0;
}

CellIdent::CellIdent(int cell, int fileNum, int subfile) {
	_cell = cell;
	_fileNum = fileNum;
	_subfile = subfile;
}

/*------------------------------------------------------------------------*/

Resource::Resource() {
	_stream = nullptr;
	_size = 0;
	_data = nullptr;
}

Resource::~Resource() {
	delete[] _data;
	delete _stream;
}

Resource::Resource(byte *p, int size) {
	_data = p;
	_size = size;
	_stream = new Common::MemoryReadStream(p, size);
}

byte *Resource::data() {
	if (_data == nullptr) {
		_data = new byte[_size];
		int pos = _stream->pos();
		_stream->seek(0);
		_stream->read(_data, _size);
		_stream->seek(pos);
	}

	return _data;
}

/*------------------------------------------------------------------------*/

FileManager::FileManager(AccessEngine *vm) : _vm(vm) {
	_fileNumber = -1;
	_setPaletteFlag = true;
}

FileManager::~FileManager() {
}

Resource *FileManager::loadFile(int fileNum, int subfile) {
	Resource *res = new Resource();
	setAppended(res, fileNum);
	gotoAppended(res, subfile);

	handleFile(res);
	return res;
}

Resource *FileManager::loadFile(const FileIdent &fileIdent) {
	return loadFile(fileIdent._fileNum, fileIdent._subfile);
}

Resource *FileManager::loadFile(const Common::String &filename) {
	Resource *res = new Resource();

	// Open the file
	openFile(res, filename);

	// Set up stream for the entire file
	res->_size = res->_file.size();
	res->_stream = res->_file.readStream(res->_size);

	handleFile(res);
	return res;
}

bool FileManager::existFile(const Common::String &filename) {
	Common::File f;
	return f.exists(filename);
}

void FileManager::openFile(Resource *res, const Common::String &filename) {
	// Open up the file
	_fileNumber = -1;
	if (!res->_file.open(filename))
		error("Could not open file - %s", filename.c_str());
}

void FileManager::loadScreen(Graphics::Surface *dest, int fileNum, int subfile) {
	Resource *res = loadFile(fileNum, subfile);
	handleScreen(dest, res);
	delete res;
}

void FileManager::handleScreen(Graphics::Surface *dest, Resource *res) {
	_vm->_screen->loadRawPalette(res->_stream);
	if (_setPaletteFlag)
		_vm->_screen->setPalette();
	_setPaletteFlag = true;

	// The remainder of the file after the palette may be separately compressed,
	// so call handleFile to handle it if it is
	res->_size -= res->_stream->pos();
	handleFile(res);

	Graphics::Surface tempScreen;
	if (dest != _vm->_screen) {
		// HACK: This is such an unbelievable hack. See e53417f. I can't
		// make heads or tails of exactly what the crap is going on. For now,
		// load tempScreen with the screen with the wrong width and same buffer.
		tempScreen.resetWithoutOwnership(_vm->_screen->getWidth(), dest->getHeight(), dest->getPitch(), static_cast<byte *>(dest->getPixels()), dest->getFormat());
		dest = &tempScreen;
	}

	if (dest->getWidth() == dest->getPitch()) {
		res->_stream->read((byte *)dest->getPixels(), dest->getWidth() * dest->getHeight());
	} else {
		for (int y = 0; y < dest->getHeight(); ++y) {
			byte *pDest = (byte *)dest->getBasePtr(0, y);
			res->_stream->read(pDest, dest->getWidth());
		}
	}

	if (dest == _vm->_screen)
		_vm->_screen->addDirtyRect(Common::Rect(dest->getWidth(), dest->getHeight()));
}

void FileManager::loadScreen(int fileNum, int subfile) {
	loadScreen(_vm->_screen, fileNum, subfile);
}

void FileManager::loadScreen(const Common::String &filename) {
	Resource *res = loadFile(filename);
	handleScreen(_vm->_screen, res);
	delete res;
}

void FileManager::handleFile(Resource *res) {
	char header[3];
	res->_stream->read(&header[0], 3);
	res->_stream->seek(-3, SEEK_CUR);

	bool isCompressed = !strncmp(header, "DBE", 3);

	// If the data is compressed, uncompress it and replace the stream
	// in the resource with the decompressed one
	if (isCompressed) {
		// Read in the entire compressed data
		byte *src = new byte[res->_size];
		res->_stream->read(src, res->_size);

		// Decompress the data
		res->_size = decompressDBE(src, &res->_data);

		// Replace the default resource stream with a stream for the decompressed data
		delete res->_stream;
		res->_file.close();
		res->_stream = new Common::MemoryReadStream(res->_data, res->_size);

		delete[] src;
	}
}

void FileManager::setAppended(Resource *res, int fileNum) {
	// Open the file for access
	if (!res->_file.open(_vm->_res->FILENAMES[fileNum]))
		error("Could not open file %s", _vm->_res->FILENAMES[fileNum].c_str());

	// If a different file has been opened then previously, load its index
	if (_fileNumber != fileNum) {
		_fileNumber = fileNum;

		// Read in the file index
		int count = res->_file.readUint16LE();
		assert(count <= 100);
		_fileIndex.resize(count);
		for (int i = 0; i < count; ++i)
			_fileIndex[i] = res->_file.readUint32LE();
	}
}

void FileManager::gotoAppended(Resource *res, int subfile) {
	uint32 offset = _fileIndex[subfile];
	uint32 size = (subfile == (int)_fileIndex.size() - 1) ? res->_file.size() - offset :
		_fileIndex[subfile + 1] - offset;

	res->_size = size;
	res->_stream = new Common::SeekableSubReadStream(&res->_file, offset, offset + size);
}

} // End of namespace Access
