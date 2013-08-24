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

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef ZVISION_PUSH_TOGGLE_CONTROL_H
#define ZVISION_PUSH_TOGGLE_CONTROL_H

#include "common/types.h"

#include "zvision/control.h"
#include "zvision/mouse_event.h"


namespace ZVision {

class PushToggleControl : public Control, public MouseEvent {
public:
	PushToggleControl(ZVision *engine, uint32 key, Common::SeekableReadStream &stream);
	bool enable();
	bool disable();

	/**
	 * Called when LeftMouse is pushed. Calls ScriptManager::setStateValue(_key, 1);
	 *
	 * @param screenSpacePos             The position of the mouse in screen space
	 * @param backgroundImageSpacePos    The position of the mouse in background image space
	 */
	void onMouseDown(const Common::Point &screenSpacePos, const Common::Point backgroundImageSpacePos);
	/**
	 * Called when LeftMouse is lifted. Does nothing
	 *
	 * @param screenSpacePos             The position of the mouse in screen space
	 * @param backgroundImageSpacePos    The position of the mouse in background image space
	 */
	void onMouseUp(const Common::Point &screenSpacePos, const Common::Point backgroundImageSpacePos) {}
	/**
	 * Called on every MouseMove. Tests if the mouse is inside _hotspot, and if so, sets the cursor.
	 *
	 * @param engine                     The base engine
	 * @param screenSpacePos             The position of the mouse in screen space
	 * @param backgroundImageSpacePos    The position of the mouse in background image space
	 * @return                           Was the cursor changed?
	 */
	bool onMouseMove(const Common::Point &screenSpacePos, const Common::Point backgroundImageSpacePos);

private:
	ZVision * _engine;
	/**
	 * The area that will trigger the event
	 * This is in image space coordinates, NOT screen space
	 */
	Common::Rect _hotspot;
	/** The cursor to use when hovering over _hotspot */
	Common::String _hoverCursor;
};

} // End of namespace ZVision

#endif
