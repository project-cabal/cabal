/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
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
 * $URL$
 * $Id$
 */

#ifdef __PSP__

#include "engines/engine.h"
#include "backends/fs/abstract-fs.h"

#include <sys/stat.h>
#include <unistd.h>

#define	ROOT_PATH	"ms0:/"

/**
 * Implementation of the ScummVM file system API based on PSPSDK API.
 * 
 * Parts of this class are documented in the base interface class, AbstractFilesystemNode.
 */
class PSPFilesystemNode : public AbstractFilesystemNode {
protected:
	String _displayName;
	String _path;
	bool _isDirectory;
	bool _isValid;
	
public:
	/**
	 * Creates a PSPFilesystemNode with the root node as path.
	 */
	PSPFilesystemNode();
	
	/**
	 * Creates a PSPFilesystemNode for a given path.
	 * 
	 * @param path String with the path the new node should point to.
	 * @param verify true if the isValid and isDirectory flags should be verified during the construction.
	 */
	PSPFilesystemNode(const Common::String &p, bool verify);

	virtual bool exists() const { return true; }		//FIXME: this is just a stub
	virtual String getDisplayName() const { return _displayName; }
	virtual String getName() const { return _displayName; }
	virtual String getPath() const { return _path; }
	virtual bool isDirectory() const { return _isDirectory; }
	virtual bool isReadable() const { return true; }	//FIXME: this is just a stub
	virtual bool isValid() const { return _isValid; }
	virtual bool isWritable() const { return true; }	//FIXME: this is just a stub

	virtual AbstractFilesystemNode *getChild(const String &n) const;
	virtual bool getChildren(AbstractFSList &list, ListMode mode, bool hidden) const;
	virtual AbstractFilesystemNode *getParent() const;
};

/**
 * Returns the last component of a given path.
 * 
 * Examples:
 * 			/foo/bar.txt would return /bar.txt
 * 			/foo/bar/    would return /bar/
 *  
 * @param str String containing the path.
 * @return Pointer to the first char of the last component inside str.
 */
static const char *lastPathComponent(const Common::String &str) {
	const char *start = str.c_str();
	const char *cur = start + str.size() - 2;

	while (cur >= start && *cur != '/') {
		--cur;
	}

	return cur + 1;
}

PSPFilesystemNode::PSPFilesystemNode() {
	_isDirectory = true;
	_displayName = "Root";
	_isValid = true;
	_path = ROOT_PATH;
}

PSPFilesystemNode::PSPFilesystemNode(const Common::String &p, bool verify) {
	assert(p.size() > 0);
        
	_path = p;
	_displayName = _path;
	_isValid = true;
	_isDirectory = true;

	if (verify) {
		struct stat st; 
		_isValid = (0 == stat(_path.c_str(), &st));
		_isDirectory = S_ISDIR(st.st_mode);
	}       
}

AbstractFilesystemNode *PSPFilesystemNode::getChild(const String &n) const {
	// FIXME: Pretty lame implementation! We do no error checking to speak
	// of, do not check if this is a special node, etc.
	assert(_isDirectory);
	
	String newPath(_path);
	if (_path.lastChar() != '/')
		newPath += '/';
	newPath += n;

	return new PSPFilesystemNode(newPath, true);
}

bool PSPFilesystemNode::getChildren(AbstractFSList &myList, ListMode mode, bool hidden) const {
	assert(_isDirectory);

	//TODO: honor the hidden flag

	int dfd  = sceIoDopen(_path.c_str());
	if (dfd > 0) {
		SceIoDirent dir;	   
		memset(&dir, 0, sizeof(dir));
	   
		while (sceIoDread(dfd, &dir) > 0) {
			// Skip 'invisible files
			if (dir.d_name[0] == '.') 
				continue;
               
			PSPFilesystemNode entry;
            
			entry._isValid = true;
			entry._displayName = dir.d_name;
			entry._path = _path;
			entry._path += dir.d_name;
			entry._isDirectory = dir.d_stat.st_attr & FIO_SO_IFDIR;
			
			if (entry._isDirectory)
				entry._path += "/";
            
			// Honor the chosen mode
			if ((mode == FilesystemNode::kListFilesOnly && entry._isDirectory) ||
			   (mode == FilesystemNode::kListDirectoriesOnly && !entry._isDirectory))
				continue;
            
			myList.push_back(new PSPFilesystemNode(entry));
		}

		sceIoDclose(dfd);
		return true;
	} else {
		return false;
	}
}

AbstractFilesystemNode *PSPFilesystemNode::getParent() const {
	assert(_isValid);
	
	if (_path == ROOT_PATH)
		return 0;
	
	const char *start = _path.c_str();
	const char *end = lastPathComponent(_path);
	
	return new PSPFilesystemNode(String(start, end - start), false);
}

#endif //#ifdef __PSP__
