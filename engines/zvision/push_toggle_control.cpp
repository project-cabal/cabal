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

#include "common/scummsys.h"

#include "common/stream.h"

#include "zvision/push_toggle_control.h"
#include "zvision/zvision.h"
#include "zvision/script_manager.h"
#include "zvision/cursor_manager.h"
#include "zvision/utility.h"

namespace ZVision {

PushToggleControl::PushToggleControl(ZVision *engine, uint32 key, Common::SeekableReadStream &stream)
		: MouseEvent(key),
		_engine(engine) {
	// Loop until we find the closing brace
	Common::String line = stream.readLine();
	trimCommentsAndWhiteSpace(&line);

	while (!stream.eos() && !line.contains('}')) {
		if (line.matchString("*_hotspot*", true)) {
			uint x;
			uint y;
			uint width;
			uint height;

			sscanf(line.c_str(), "%*[^(](%u,%u,%u,%u)", &x, &y, &width, &height);

			_hotspot = Common::Rect(x, y, x + width, y + height);
		} else if (line.matchString("cursor*", true)) {
			char nameBuffer[25];

			sscanf(line.c_str(), "%*[^(](%25[^)])", nameBuffer);

			_hoverCursor = Common::String(nameBuffer);
		}

		line = stream.readLine();
		trimCommentsAndWhiteSpace(&line);
	}

	if (_hotspot.isEmpty() || _hoverCursor.empty()) {
		warning("Push_toggle cursor %u was parsed incorrectly", key);
	}
}

	bool PushToggleControl::enable() {
		if (!_enabled) {
			_engine->registerMouseEvent(this);
			_enabled = true;
			return true;
		}

		debug("Control %u is already enabled", _key);
		return false;
	}

	bool PushToggleControl::disable() {
		if (_enabled) {
			_engine->removeMouseEvent(_key);
			_enabled = false;
			return true;
		}

		debug("Control %u is already disabled", _key);
		return false;
	}

	void PushToggleControl::onMouseDown(const Common::Point &screenSpacePos, const Common::Point backgroundImageSpacePos) {
		_engine->getScriptManager()->setStateValue(_key, 1);
	}

	bool PushToggleControl::onMouseMove(const Common::Point &screenSpacePos, const Common::Point backgroundImageSpacePos) {
		if (_hotspot.contains(backgroundImageSpacePos)) {
			_engine->getCursorManager()->changeCursor(_hoverCursor);
			return true;
		}

		return false;
	}

} // End of namespace ZVision
