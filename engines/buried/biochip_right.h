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

#ifndef BURIED_BIOCHIP_RIGHT_H
#define BURIED_BIOCHIP_RIGHT_H

#include "buried/window.h"

namespace Buried {

class BioChipRightWindow : public Window {
public:
	BioChipRightWindow(BuriedEngine *vm, Window *parent);
	~BioChipRightWindow();

	bool changeCurrentBioChip(int bioChipID);
	bool showBioChipMainView();
	bool destroyBioChipViewWindow();
	void sceneChanged();
	void disableEvidenceCapture();
	void jumpInitiated(bool redraw);
	void jumpEnded(bool redraw);

	void onPaint();
	void onEnable(bool enable);
	void onLButtonUp(const Common::Point &point, uint flags);

	// clone2727 says: These are labeled as HACKS, so I assume they are.
	bool _forceHelp;
	bool _forceComment;

private:
	int _curBioChip;
	int _status;
	Window *_bioChipViewWindow;
	bool _jumpInProgress;
};

} // End of namespace Buried

#endif
