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

#include "buried/biochip_right.h"
#include "buried/buried.h"
#include "buried/gameui.h"
#include "buried/graphics.h"
#include "buried/invdata.h"
#include "buried/inventory_window.h"
#include "buried/navarrow.h"
#include "buried/resources.h"
#include "buried/scene_view.h"
#include "buried/sound.h"
#include "buried/environ/scene_base.h"
#include "buried/environ/scene_common.h"

#include "common/system.h"
#include "graphics/font.h"

namespace Buried {

enum {
	GC_AIHW_STARTING_VALUE = 100,
	GC_AI_OT_WALK_DECREMENT = 2,
	GC_AI_OT_TURN_DECREMENT = 1,
	GC_AI_OT_WAIT_DECREMENT = 1,
	GC_AI_OT_WAIT_TIME_PERIOD = 10000
};

class BaseOxygenTimer : public SceneBase {
public:
	BaseOxygenTimer(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation);
	virtual int postEnterRoom(Window *viewWindow, const Location &priorLocation);
	virtual int preExitRoom(Window *viewWindow, const Location &priorLocation);
	virtual int timerCallback(Window *viewWindow);

protected:
	uint32 _entryStartTime;
	int _deathID;
	bool _jumped;
};

BaseOxygenTimer::BaseOxygenTimer(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation) :
		SceneBase(vm, viewWindow, sceneStaticData, priorLocation) {
	_deathID = 41;
	_jumped = false;
}

int BaseOxygenTimer::postEnterRoom(Window *viewWindow, const Location &priorLocation) {
	_entryStartTime = g_system->getMillis();
	return SC_TRUE;
}

int BaseOxygenTimer::preExitRoom(Window *viewWindow, const Location &newLocation) {
	// NOTE: v1.01 used 25% as the low threshold instead of ~14.2%

	if (newLocation.timeZone == -2) {
		_jumped = true;
		return SC_TRUE;
	}

	int currentValue = ((SceneViewWindow *)viewWindow)->getGlobalFlags().aiOxygenTimer;

	if (((SceneViewWindow *)viewWindow)->getGlobalFlags().generalWalkthroughMode == 0) {
		if (_staticData.location.node != newLocation.node) {
			if (currentValue <= GC_AI_OT_WALK_DECREMENT) {
				if (newLocation.timeZone != -2)
					((SceneViewWindow *)viewWindow)->showDeathScene(_deathID);
				return SC_DEATH;
			} else {
				currentValue -= GC_AI_OT_WALK_DECREMENT;
				((SceneViewWindow *)viewWindow)->getGlobalFlags().aiOxygenTimer = currentValue;

				if (currentValue < GC_AIHW_STARTING_VALUE / 7 || (currentValue % (GC_AIHW_STARTING_VALUE / 10)) == 0) {
					if (currentValue < GC_AIHW_STARTING_VALUE / 7) {
						Common::String oxygenMessage = _vm->getString(IDS_AI_OXY_LEVEL_TEXT_TEMPLATE_LOW);
						assert(!oxygenMessage.empty());
						oxygenMessage = Common::String::format(oxygenMessage.c_str(), currentValue * 100 / GC_AIHW_STARTING_VALUE);
						((SceneViewWindow *)viewWindow)->displayLiveText(oxygenMessage);
					} else {
						Common::String oxygenMessage = _vm->getString(IDS_AI_OXY_LEVEL_TEXT_TEMPLATE_NORM);
						assert(!oxygenMessage.empty());
						oxygenMessage = Common::String::format(oxygenMessage.c_str(), currentValue * 100 / GC_AIHW_STARTING_VALUE);
						((SceneViewWindow *)viewWindow)->displayLiveText(oxygenMessage);
					}
				}
			}
		} else {
			if (currentValue <= GC_AI_OT_TURN_DECREMENT) {
				if (newLocation.timeZone != -2)
					((SceneViewWindow *)viewWindow)->showDeathScene(_deathID);
				return SC_DEATH;
			} else {
				currentValue -= GC_AI_OT_TURN_DECREMENT;
				((SceneViewWindow *)viewWindow)->getGlobalFlags().aiOxygenTimer = currentValue;

				if (currentValue < GC_AIHW_STARTING_VALUE / 7 || (currentValue % (GC_AIHW_STARTING_VALUE / 10)) == 0) {
					if (currentValue < GC_AIHW_STARTING_VALUE / 7) {
						Common::String oxygenMessage = _vm->getString(IDS_AI_OXY_LEVEL_TEXT_TEMPLATE_LOW);
						assert(!oxygenMessage.empty());
						oxygenMessage = Common::String::format(oxygenMessage.c_str(), currentValue * 100 / GC_AIHW_STARTING_VALUE);
						((SceneViewWindow *)viewWindow)->displayLiveText(oxygenMessage);
					} else {
						Common::String oxygenMessage = _vm->getString(IDS_AI_OXY_LEVEL_TEXT_TEMPLATE_NORM);
						assert(!oxygenMessage.empty());
						oxygenMessage = Common::String::format(oxygenMessage.c_str(), currentValue * 100 / GC_AIHW_STARTING_VALUE);
						((SceneViewWindow *)viewWindow)->displayLiveText(oxygenMessage);
					}
				}
			}
		}
	}

	return SC_TRUE;
}

int BaseOxygenTimer::timerCallback(Window *viewWindow) {
	// NOTE: Earlier versions (1.01) used 25% as the low threshold instead of
	// ~14.2%

	if (_jumped)
		return SC_TRUE;

	if (((SceneViewWindow *)viewWindow)->getGlobalFlags().generalWalkthroughMode == 0) {
		if ((g_system->getMillis() - _entryStartTime) >= GC_AI_OT_WAIT_TIME_PERIOD) {
			int currentValue = ((SceneViewWindow *)viewWindow)->getGlobalFlags().aiOxygenTimer;

			if (currentValue <= GC_AI_OT_WAIT_DECREMENT) {
				((SceneViewWindow *)viewWindow)->showDeathScene(_deathID);
				return SC_DEATH;
			} else {
				currentValue -= GC_AI_OT_WAIT_DECREMENT;
				((SceneViewWindow *)viewWindow)->getGlobalFlags().aiOxygenTimer = currentValue;

				if (currentValue < GC_AIHW_STARTING_VALUE / 7 || (currentValue % (GC_AIHW_STARTING_VALUE / 10)) == 0) {
					if (currentValue < GC_AIHW_STARTING_VALUE / 7) {
						Common::String oxygenMessage = _vm->getString(IDS_AI_OXY_LEVEL_TEXT_TEMPLATE_LOW);
						assert(!oxygenMessage.empty());
						oxygenMessage = Common::String::format(oxygenMessage.c_str(), currentValue * 100 / GC_AIHW_STARTING_VALUE);
						((SceneViewWindow *)viewWindow)->displayLiveText(oxygenMessage);
					} else {
						Common::String oxygenMessage = _vm->getString(IDS_AI_OXY_LEVEL_TEXT_TEMPLATE_NORM);
						assert(!oxygenMessage.empty());
						oxygenMessage = Common::String::format(oxygenMessage.c_str(), currentValue * 100 / GC_AIHW_STARTING_VALUE);
						((SceneViewWindow *)viewWindow)->displayLiveText(oxygenMessage);
					}
				}
			}

			_entryStartTime = g_system->getMillis();
		}
	}

	return SC_TRUE;
}

class SpaceDoorTimer : public BaseOxygenTimer {
public:
	SpaceDoorTimer(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation,
			int left = -1, int top = -1, int right = -1, int bottom = -1, int openFrame = -1, int closedFrame = -1, int depth = -1,
			int transitionType = -1, int transitionData = -1, int transitionStartFrame = -1, int transitionLength = -1,
			int doorFlag = -1, int doorFlagValue = 0);
	int mouseDown(Window *viewWindow, const Common::Point &pointLocation);
	int mouseUp(Window *viewWindow, const Common::Point &pointLocation);
	int specifyCursor(Window *viewWindow, const Common::Point &pointLocation);

private:
	bool _clicked;
	Common::Rect _clickable;
	DestinationScene _destData;
	int _openFrame;
	int _closedFrame;
	int _doorFlag;
	int _doorFlagValue;
};

SpaceDoorTimer::SpaceDoorTimer(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation,
		int left, int top, int right, int bottom, int openFrame, int closedFrame, int depth,
		int transitionType, int transitionData, int transitionStartFrame, int transitionLength,
		int doorFlag, int doorFlagValue) :
		BaseOxygenTimer(vm, viewWindow, sceneStaticData, priorLocation) {
	_clicked = false;
	_openFrame = openFrame;
	_closedFrame = closedFrame;
	_doorFlag = doorFlag;
	_doorFlagValue = doorFlagValue;
	_clickable = Common::Rect(left, top, right, bottom);
	_destData.destinationScene = _staticData.location;
	_destData.destinationScene.depth = depth;
	_destData.transitionType = transitionType;
	_destData.transitionData = transitionData;
	_destData.transitionStartFrame = transitionStartFrame;
	_destData.transitionLength = transitionLength;
}

int SpaceDoorTimer::mouseDown(Window *viewWindow, const Common::Point &pointLocation) {
	if (_clickable.contains(pointLocation)) {
		_clicked = true;
		return SC_TRUE;
	}

	return SC_FALSE;
}

int SpaceDoorTimer::mouseUp(Window *viewWindow, const Common::Point &pointLocation) {
	if (_clicked) {
		// If we are facing the habitat wing death door in walkthrough mode,
		// keep it locked.
		if (((SceneViewWindow *)viewWindow)->getGlobalFlags().generalWalkthroughMode == 1 &&
				_staticData.location.timeZone == 6 && _staticData.location.environment == 1 &&
				_staticData.location.node == 3 && _staticData.location.facing == 1 &&
				_staticData.location.orientation == 2 && _staticData.location.depth == 0) {
			_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, 12));
			_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, 13));
			_clicked = false;
			return SC_TRUE;
		}

		// If we are facing the scanning room door and we have Arthur, automatically recall
		// to the future apartment
		if (_staticData.location.timeZone == 6 && _staticData.location.environment == 3 &&
				_staticData.location.node == 9 && _staticData.location.facing == 0 &&
				_staticData.location.orientation == 0 && _staticData.location.depth == 0 &&
				((GameUIWindow *)viewWindow->getParent())->_inventoryWindow->isItemInInventory(kItemBioChipAI)) {
			((SceneViewWindow *)viewWindow)->timeSuitJump(4);
			return SC_TRUE;
		}

		if (_doorFlag < 0 || ((SceneViewWindow *)viewWindow)->getGlobalFlagByte(_doorFlag) == _doorFlagValue) {
			// Change the still frame to the new one
			if (_openFrame >= 0) {
				_staticData.navFrameIndex = _openFrame;
				viewWindow->invalidateWindow(false);
				_vm->_sound->playSynchronousSoundEffect("BITDATA/AILAB/AI_LOCK.BTA"); // Broken in 1.01
			}

			((SceneViewWindow *)viewWindow)->moveToDestination(_destData);
		} else {
			// Display the closed frame
			if (_closedFrame >= 0) {
				int oldFrame = _staticData.navFrameIndex;
				_staticData.navFrameIndex = _closedFrame;
				viewWindow->invalidateWindow(false);

				_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, 12));
				_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, 13));

				_staticData.navFrameIndex = oldFrame;
				viewWindow->invalidateWindow(false);
			}
		}

		_clicked = false;
		return SC_TRUE;
	}

	return SC_FALSE;
}

int SpaceDoorTimer::specifyCursor(Window *viewWindow, const Common::Point &pointLocation) {
	// If we are in walkthrough mode and are at the death door in the habitat wing,
	// don't allow you to open the door.
	if (((SceneViewWindow *)viewWindow)->getGlobalFlags().generalWalkthroughMode == 1 &&
			_staticData.location.timeZone == 6 && _staticData.location.environment == 1 &&
			_staticData.location.node == 3 && _staticData.location.facing == 1 &&
			_staticData.location.orientation == 2 && _staticData.location.depth == 0) {
		return kCursorArrow;
	}

	if (_clickable.contains(pointLocation))
		return kCursorFinger;

	return kCursorArrow;
}

class UseCheeseGirlPropellant : public BaseOxygenTimer {
public:
	UseCheeseGirlPropellant(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation);
	int postEnterRoom(Window *viewWindow, const Location &priorLocation);
	int draggingItem(Window *viewWindow, int itemID, const Common::Point &pointLocation, int itemFlags);
	int droppedItem(Window *viewWindow, int itemID, const Common::Point &pointLocation, int itemFlags);

private:
	Common::Rect _badPos;
};

UseCheeseGirlPropellant::UseCheeseGirlPropellant(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation) :
		BaseOxygenTimer(vm, viewWindow, sceneStaticData, priorLocation) {
	_deathID = 40;
	_badPos = Common::Rect(144, 0, 288, 189);
}

int UseCheeseGirlPropellant::postEnterRoom(Window *viewWindow, const Location &priorLocation) {
	// Display text message about zero-g and no propulsion and oxygen level
	((SceneViewWindow *)viewWindow)->displayLiveText(_vm->getString(IDS_AI_IS_JUMP_IN_TEXT));
	return SC_TRUE;
}

int UseCheeseGirlPropellant::draggingItem(Window *viewWindow, int itemID, const Common::Point &pointLocation, int itemFlags) {
	if (itemID == kItemCheeseGirl && !_badPos.contains(pointLocation))
		return 1;

	return 0;
}

int UseCheeseGirlPropellant::droppedItem(Window *viewWindow, int itemID, const Common::Point &pointLocation, int itemFlags) {
	if (itemID == kItemCheeseGirl) {
		if (_badPos.contains(pointLocation)) {
			DestinationScene destData;
			destData.destinationScene.timeZone = 6;
			destData.destinationScene.environment = 10;
			destData.destinationScene.node = 1;
			destData.destinationScene.facing = 0;
			destData.destinationScene.orientation = 0;
			destData.destinationScene.depth = 0;
			destData.transitionType = TRANSITION_VIDEO;
			destData.transitionData = 1;
			destData.transitionStartFrame = -1;
			destData.transitionLength = -1;
			((SceneViewWindow *)viewWindow)->moveToDestination(destData);
		} else {
			DestinationScene destData;
			destData.destinationScene.timeZone = 6;
			destData.destinationScene.environment = 1;
			destData.destinationScene.node = 1;
			destData.destinationScene.facing = 1;
			destData.destinationScene.orientation = 2;
			destData.destinationScene.depth = 0;
			destData.transitionType = TRANSITION_VIDEO;
			destData.transitionData = 0;
			destData.transitionStartFrame = -1;
			destData.transitionLength = -1;
			((SceneViewWindow *)viewWindow)->moveToDestination(destData);
		}

		return SIC_ACCEPT;
	}

	return SIC_REJECT;
}

class PlayArthurOffsetTimed : public BaseOxygenTimer {
public:
	PlayArthurOffsetTimed(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation,
			int stingerVolume = 127, int lastStingerFlagOffset = -1, int effectIDFlagOffset = -1, int firstStingerFileID = -1,
			int lastStingerFileID = -1, int stingerDelay = 1);
	int postEnterRoom(Window *viewWindow, const Location &priorLocation);

private:
	int _stingerVolume;
	int _lastStingerFlagOffset;
	int _effectIDFlagOffset;
	int _firstStingerFileID;
	int _lastStingerFileID;
	int _stingerDelay;
	int _timerFlagOffset;
};

PlayArthurOffsetTimed::PlayArthurOffsetTimed(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation,
		int stingerVolume, int lastStingerFlagOffset, int effectIDFlagOffset, int firstStingerFileID,
		int lastStingerFileID, int stingerDelay) :
		BaseOxygenTimer(vm, viewWindow, sceneStaticData, priorLocation) {
	_stingerVolume = stingerVolume;
	_lastStingerFlagOffset = lastStingerFlagOffset;
	_effectIDFlagOffset = effectIDFlagOffset;
	_firstStingerFileID = firstStingerFileID;
	_lastStingerFileID = lastStingerFileID;
	_stingerDelay = stingerDelay;
}

int PlayArthurOffsetTimed::postEnterRoom(Window *viewWindow, const Location &priorLocation) {
	if (_effectIDFlagOffset >= 0 && (priorLocation.node != _staticData.location.node || priorLocation.environment != _staticData.location.environment)) {
		byte effectID = ((SceneViewWindow *)viewWindow)->getGlobalFlagByte(_effectIDFlagOffset);

		if (!_vm->_sound->isSoundEffectPlaying(effectID - 1)) {
			int lastStinger = ((SceneViewWindow *)viewWindow)->getGlobalFlagByte(_lastStingerFlagOffset) + 1;

			if ((lastStinger % _stingerDelay) == 0) {
				if (lastStinger <= (_lastStingerFileID - _firstStingerFileID) * _stingerDelay) {
					int fileNameIndex = _vm->computeFileNameResourceID(_staticData.location.timeZone, _staticData.location.environment, _firstStingerFileID + lastStinger / _stingerDelay - 1);
					byte newStingerID = 0;

					if (((GameUIWindow *)viewWindow->getParent())->_inventoryWindow->isItemInInventory(kItemBioChipAI)) {
						newStingerID = _vm->_sound->playSoundEffect(_vm->getFilePath(fileNameIndex), _stingerVolume / 2, false, true) + 1;
						byte &lastArthurComment = ((SceneViewWindow *)viewWindow)->getGlobalFlags().aiHWLastCommentPlayed;

						if ((lastStinger / 2) != 0 && lastArthurComment < 4) {
							lastArthurComment++;
							_vm->_sound->playSoundEffect(_vm->getFilePath(_staticData.location.timeZone, 10, lastArthurComment + 5), 128, false, true);
						}
					} else {
						newStingerID = _vm->_sound->playSoundEffect(_vm->getFilePath(fileNameIndex), _stingerVolume, false, true) + 1;
					}

					((SceneViewWindow *)viewWindow)->setGlobalFlagByte(_effectIDFlagOffset, newStingerID);
					((SceneViewWindow *)viewWindow)->setGlobalFlagByte(_lastStingerFlagOffset, lastStinger);
				}
			} else {
				((SceneViewWindow *)viewWindow)->setGlobalFlagByte(_effectIDFlagOffset, 0xFF);
				((SceneViewWindow *)viewWindow)->setGlobalFlagByte(_lastStingerFlagOffset, lastStinger);
			}
		}
	}

	return SC_TRUE;
}

class HabitatWingIceteroidDoor : public BaseOxygenTimer {
public:
	HabitatWingIceteroidDoor(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation);
	int mouseUp(Window *viewWindow, const Common::Point &pointLocation);
	int specifyCursor(Window *viewWindow, const Common::Point &pointLocation);

private:
	Common::Rect _doorHandle;
};

HabitatWingIceteroidDoor::HabitatWingIceteroidDoor(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation) :
		BaseOxygenTimer(vm, viewWindow, sceneStaticData, priorLocation) {
	_doorHandle = Common::Rect(122, 48, 246, 172);
}

int HabitatWingIceteroidDoor::mouseUp(Window *viewWindow, const Common::Point &pointLocation) {
	if (_doorHandle.contains(pointLocation)) {
		if (((GameUIWindow *)viewWindow->getParent())->_inventoryWindow->isItemInInventory(kItemBioChipAI)) {
			if (((SceneViewWindow * )viewWindow)->getGlobalFlags().aiHWIceDoorUnlocked == 0) {
				// Move to depth one (door open)
				DestinationScene destData;
				destData.destinationScene = _staticData.location;
				destData.destinationScene.depth = 1;
				destData.transitionType = TRANSITION_VIDEO;
				destData.transitionData = 6;
				destData.transitionStartFrame = -1;
				destData.transitionLength = -1;
				((SceneViewWindow *)viewWindow)->moveToDestination(destData);

				// Add the explosive charge to your inventory
				((GameUIWindow *)viewWindow->getParent())->_inventoryWindow->addItem(kItemExplosiveCharge);

				// Set the door unlocked flag
				((SceneViewWindow * )viewWindow)->getGlobalFlags().aiHWIceDoorUnlocked = 1;
			} else {
				// Move to depth one (door open)
				DestinationScene destData;
				destData.destinationScene = _staticData.location;
				destData.destinationScene.depth = 1;
				destData.transitionType = TRANSITION_VIDEO;
				destData.transitionData = 3;
				destData.transitionStartFrame = -1;
				destData.transitionLength = -1;
				((SceneViewWindow *)viewWindow)->moveToDestination(destData);
			}
		} else {
			// Play the closed door video clip
			((SceneViewWindow *)viewWindow)->playSynchronousAnimation(7);
		}

		return SC_TRUE;
	}

	return SC_FALSE;
}

int HabitatWingIceteroidDoor::specifyCursor(Window *viewWindow, const Common::Point &pointLocation) {
	if (_doorHandle.contains(pointLocation))
		return kCursorFinger;

	return kCursorArrow;
}

class IceteroidPodTimed : public BaseOxygenTimer {
public:
	IceteroidPodTimed(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation,
			int left = -1, int top = -1, int right = -1, int bottom = -1, int animID = -1, int timeZone = -1,
			int environment = -1, int node = -1, int facing = -1, int orientation = -1, int depth = -1);
	int mouseUp(Window *viewWindow, const Common::Point &pointLocation);
	int specifyCursor(Window *viewWindow, const Common::Point &pointLocation);

private:
	Common::Rect _engageButton;
	DestinationScene _clickDestination;
};

IceteroidPodTimed::IceteroidPodTimed(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation,
			int left, int top, int right, int bottom, int animID, int timeZone,
			int environment, int node, int facing, int orientation, int depth) :
		BaseOxygenTimer(vm, viewWindow, sceneStaticData, priorLocation) {
	_engageButton = Common::Rect(left, top, right, bottom);
	_clickDestination.destinationScene = Location(timeZone, environment, node, facing, orientation, depth);
	_clickDestination.transitionType = TRANSITION_VIDEO;
	_clickDestination.transitionData = animID;
	_clickDestination.transitionStartFrame = -1;
	_clickDestination.transitionLength = -1;
}

int IceteroidPodTimed::mouseUp(Window *viewWindow, const Common::Point &pointLocation) {
	if (_engageButton.contains(pointLocation)) // Make it so!
		((SceneViewWindow *)viewWindow)->moveToDestination(_clickDestination);

	return SC_FALSE;
}

int IceteroidPodTimed::specifyCursor(Window *viewWindow, const Common::Point &pointLocation) {
	if (_engageButton.contains(pointLocation))
		return kCursorFinger;

	return kCursorArrow;
}

class IceteroidElevatorExtremeControls : public BaseOxygenTimer {
public:
	IceteroidElevatorExtremeControls(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation,
			int upTimeZone = -1, int upEnvironment = -1, int upNode = -1, int upFacing = -1, int upOrientation = -1, int upDepth = -1, int upAnimID = -1,
			int downTimeZone = -1, int downEnvironment = -1, int downNode = -1, int downFacing = -1, int downOrientation = -1, int downDepth = -1, int downAnimID = -1);
	int mouseUp(Window *viewWindow, const Common::Point &pointLocation);
	int specifyCursor(Window *viewWindow, const Common::Point &pointLocation);

private:
	Common::Rect _up, _down;
	DestinationScene _upDestination;
	DestinationScene _downDestination;
};

IceteroidElevatorExtremeControls::IceteroidElevatorExtremeControls(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation,
		int upTimeZone, int upEnvironment, int upNode, int upFacing, int upOrientation, int upDepth, int upAnimID,
		int downTimeZone, int downEnvironment, int downNode, int downFacing, int downOrientation, int downDepth, int downAnimID) :
		BaseOxygenTimer(vm, viewWindow, sceneStaticData, priorLocation) {
	_up = Common::Rect(192, 123, 212, 143);
	_down = Common::Rect(192, 148, 212, 168);

	_upDestination.destinationScene = Location(upTimeZone, upEnvironment, upNode, upFacing, upOrientation, upDepth);
	_upDestination.transitionType = TRANSITION_VIDEO;
	_upDestination.transitionData = upAnimID;
	_upDestination.transitionStartFrame = -1;
	_upDestination.transitionLength = -1;

	_downDestination.destinationScene = Location(downTimeZone, downEnvironment, downNode, downFacing, downOrientation, downDepth);
	_downDestination.transitionType = TRANSITION_VIDEO;
	_downDestination.transitionData = downAnimID;
	_downDestination.transitionStartFrame = -1;
	_downDestination.transitionLength = -1;
}

int IceteroidElevatorExtremeControls::mouseUp(Window *viewWindow, const Common::Point &pointLocation) {
	if (_up.contains(pointLocation) && _upDestination.transitionData >= 0) {
		((SceneViewWindow *)viewWindow)->moveToDestination(_upDestination);
		return SC_TRUE;
	}

	if (_down.contains(pointLocation) && _downDestination.transitionData >= 0) {
		((SceneViewWindow *)viewWindow)->moveToDestination(_downDestination);
		return SC_TRUE;
	}

	return SC_FALSE;
}

int IceteroidElevatorExtremeControls::specifyCursor(Window *viewWindow, const Common::Point &pointLocation) {
	if (_up.contains(pointLocation) && _upDestination.transitionData >= 0)
		return kCursorFinger;

	if (_down.contains(pointLocation) && _downDestination.transitionData >= 0)
		return kCursorFinger;

	return kCursorArrow;
}

class NexusDoor : public BaseOxygenTimer {
public:
	NexusDoor(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation);
	int postEnterRoom(Window *viewWindow, const Location &priorLocation);
	int mouseUp(Window *viewWindow, const Common::Point &pointLocation);
	int specifyCursor(Window *viewWindow, const Common::Point &pointLocation);

private:
	uint32 _entryStartTime;
	Common::Rect _door;
	bool _jumped;
};

NexusDoor::NexusDoor(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation) :
		BaseOxygenTimer(vm, viewWindow, sceneStaticData, priorLocation) {
	_door = Common::Rect(148, 30, 328, 192);
}

int NexusDoor::postEnterRoom(Window *viewWindow, const Location &priorLocation) {
	BaseOxygenTimer::postEnterRoom(viewWindow, priorLocation);

	if (priorLocation.environment != _staticData.location.environment || priorLocation.timeZone != _staticData.location.timeZone) {
		((SceneViewWindow *)viewWindow)->getGlobalFlags().aiOxygenTimer = GC_AIHW_STARTING_VALUE;
		((SceneViewWindow *)viewWindow)->displayLiveText(_vm->getString(IDS_AI_ENTERING_NON_PRES_ENV_TEXT));
	}

	return SC_TRUE;
}

int NexusDoor::mouseUp(Window *viewWindow, const Common::Point &pointLocation) {
	if (_door.contains(pointLocation)) {
		DestinationScene destData;
		destData.destinationScene = Location(6, 5, 1, 0, 1, 0);
		destData.transitionType = TRANSITION_VIDEO;
		destData.transitionData = 0;
		destData.transitionStartFrame = -1;
		destData.transitionLength = -1;
		((SceneViewWindow *)viewWindow)->moveToDestination(destData);
		return SC_TRUE;
	}

	return SC_FALSE;
}

int NexusDoor::specifyCursor(Window *viewWindow, const Common::Point &pointLocation) {
	if (_door.contains(pointLocation))
		return kCursorFinger;

	return kCursorArrow;
}

class NexusPuzzle : public SceneBase {
public:
	NexusPuzzle(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation);
	int postEnterRoom(Window *viewWindow, const Location &priorLocation);
	int gdiPaint(Window *viewWindow);
	int mouseDown(Window *viewWindow, const Common::Point &pointLocation);
	int specifyCursor(Window *viewWindow, const Common::Point &pointLocation);

private:
	Common::Rect _lights[7];
	int _data[7];
	bool _resetMessage;
};

NexusPuzzle::NexusPuzzle(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation) :
		SceneBase(vm, viewWindow, sceneStaticData, priorLocation) {
	_data[0] = 2;
	_data[1] = 2;
	_data[2] = 2;
	_data[3] = 0;
	_data[4] = 1;
	_data[5] = 1;
	_data[6] = 1;

	_lights[0] = Common::Rect(209, 39, 225, 47);
	_lights[1] = Common::Rect(209, 52, 225, 63);
	_lights[2] = Common::Rect(209, 71, 225, 84);
	_lights[3] = Common::Rect(209, 90, 225, 106);
	_lights[4] = Common::Rect(209, 110, 225, 123);
	_lights[5] = Common::Rect(209, 126, 225, 137);
	_lights[6] = Common::Rect(209, 140, 225, 148);

	_resetMessage = false;
}

int NexusPuzzle::postEnterRoom(Window *viewWindow, const Location &priorLocation) {
	// If we came from outside (node zero), display the atmosphere message
	if (priorLocation.node == 0) {
		((SceneViewWindow *)viewWindow)->displayLiveText(_vm->getString(IDS_AI_ENTERING_PRES_ENV_TEXT));
		((SceneViewWindow *)viewWindow)->getGlobalFlags().aiOxygenTimer = GC_AIHW_STARTING_VALUE;
	}

	// Check to see if we heard the brain comment before
	if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiNXPlayedBrainComment == 0) {
		((SceneViewWindow *)viewWindow)->getGlobalFlags().aiNXPlayedBrainComment = 1;

		if (((SceneViewWindow *)viewWindow)->getGlobalFlags().generalWalkthroughMode == 0) {
			// Play a synchronous comment here to introduce the player to the puzzle
			_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, 9));
		} else {
			// Play a synchronous comment to introduce what is about to happen
			_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, 8));

			// Play the Farnstein voiceover
			_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, 14));

			// Move to the next node, playing the Arthur retrieval movie
			DestinationScene destData;
			destData.destinationScene = _staticData.location;
			destData.destinationScene.node = 2;
			destData.transitionType = TRANSITION_VIDEO;
			destData.transitionData = 1;
			destData.transitionStartFrame = -1;
			destData.transitionLength = -1;
			((SceneViewWindow *)viewWindow)->moveToDestination(destData);
		}
	}

	return SC_TRUE;
}

int NexusPuzzle::gdiPaint(Window *viewWindow) {
	// Puzzle is only in adventure mode
	if (((SceneViewWindow *)viewWindow)->getGlobalFlags().generalWalkthroughMode == 1)
		return SC_REPAINT;

	uint32 green = _vm->_gfx->getColor(0, 255, 0);
	uint32 red = _vm->_gfx->getColor(255, 0, 0);
	Common::Rect absoluteRect = viewWindow->getAbsoluteRect();

	for (int i = 0; i < 7; i++) {
		if (_data[i] != 0) {
			uint32 color = (_data[i] == 1) ? green : red;

			Common::Rect rect(_lights[i]);
			rect.translate(absoluteRect.left, absoluteRect.top);
			rect.left++;
			rect.top++;

			_vm->_gfx->drawEllipse(rect, color);
		}
	}

	return SC_REPAINT;
}
	
int NexusPuzzle::mouseDown(Window *viewWindow, const Common::Point &pointLocation) {
	// Puzzle is only in adventure mode
	if (((SceneViewWindow *)viewWindow)->getGlobalFlags().generalWalkthroughMode == 1)
		return SC_FALSE;

	// Reset the live text, if the puzzle was reset
	if (_resetMessage) {
		((SceneViewWindow *)viewWindow)->displayLiveText("");
		_resetMessage = false;
	}

	// Check to see if we clicked on any of the colored circles, and if a jump is allowed
	for (int i = 0; i < 7; i++) {
		if (_lights[i].contains(pointLocation) && _data[i] != 0) {
			if (_data[i] == 1) {
				// Can we move up one?
				if (i > 0) {
					if (_data[i - 1] == 0) {
						_data[i - 1] = _data[i];
						_data[i] = 0;
						viewWindow->invalidateWindow(false);
					} else if (i > 1 && _data[i - 2] == 0) {
						_data[i - 2] = _data[i];
						_data[i] = 0;
						viewWindow->invalidateWindow(false);
					}
				}
			} else {
				if (i < 6) {
					if (_data[i + 1] == 0) {
						_data[i + 1] = _data[i];
						_data[i] = 0;
						viewWindow->invalidateWindow(false);
					} else if (i < 5 && _data[i + 2] == 0) {
						_data[i + 2] = _data[i];
						_data[i] = 0;
						viewWindow->invalidateWindow(false);
					}
				}
			}

			// Check to see if we completed the puzzle
			if (_data[0] == 1 && _data[1] == 1 && _data[2] == 1 && _data[3] == 0 && _data[4] == 2 && _data[5] == 2 && _data[6] == 2) {
				// Play the Farnstein voiceover
				_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, 14));

				// Move to the next node, playing the Arthur retrieval movie
				DestinationScene destData;
				destData.destinationScene = _staticData.location;
				destData.destinationScene.node = 2;
				destData.transitionType = TRANSITION_VIDEO;
				destData.transitionData = 1;
				destData.transitionStartFrame = -1;
				destData.transitionLength = -1;
				((SceneViewWindow *)viewWindow)->moveToDestination(destData);
				return SC_TRUE;
			}

			// Check to see that we can still make valid moves
			bool validMove = false;
			for (int j = 0; j < 7; j++) {
				if (_data[j] == 1) {
					if (j > 1 && _data[j - 2] == 0) {
						validMove = true;
						break;
					}

					if (j > 0 && _data[j - 1] == 0) {
						validMove = true;
						break;
					}
				} else if (_data[j] == 2) {
					if (j < 5 && _data[j + 2] == 0) {
						validMove = true;
						break;
					}

					if (j < 6 && _data[j + 1] == 0) {
						validMove = true;
						break;
					}
				}
			}

			if (!validMove) {
				// Reset the puzzle
				_data[0] = 2;
				_data[1] = 2;
				_data[2] = 2;
				_data[3] = 0;
				_data[4] = 1;
				_data[5] = 1;
				_data[6] = 1;
				viewWindow->invalidateWindow(false);

				Common::String text;
				if (_vm->getVersion() >= MAKEVERSION(1, 0, 4, 0))
					text = _vm->getString(IDS_AI_NX_CODE_RESET_MESSAGE);
				else
					text = "Unable to complete in current state. Resetting code lock.";

				((SceneViewWindow *)viewWindow)->displayLiveText(text);
				_resetMessage = true;
			}

			return SC_TRUE;
		}
	}

	return SC_FALSE;
}

int NexusPuzzle::specifyCursor(Window *viewWindow, const Common::Point &pointLocation) {
	// Puzzle is only in adventure mode
	if (((SceneViewWindow *)viewWindow)->getGlobalFlags().generalWalkthroughMode == 1)
		return kCursorArrow;

	for (int i = 0; i < 7; i++)
		if (_lights[i].contains(pointLocation) && _data[i] != 0) // In the liiiiiight
			return kCursorFinger;

	return kCursorArrow;
}

class NexusEnd : public SceneBase {
public:
	NexusEnd(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation);
	int postEnterRoom(Window *viewWindow, const Location &priorLocation);
};

NexusEnd::NexusEnd(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation) :
		SceneBase(vm, viewWindow, sceneStaticData, priorLocation) {
}

int NexusEnd::postEnterRoom(Window *viewWindow, const Location &priorLocation) {
	if (!((GameUIWindow *)viewWindow->getParent())->_inventoryWindow->isItemInInventory(kItemBioChipAI)) {
		// Congrats, you have Arthur!

		// Swap and activate the chips
		((GameUIWindow *)viewWindow->getParent())->_inventoryWindow->removeItem(kItemBioChipBlank);
		((GameUIWindow *)viewWindow->getParent())->_inventoryWindow->addItem(kItemBioChipAI);
		((GameUIWindow *)viewWindow->getParent())->_bioChipRightWindow->changeCurrentBioChip(kItemBioChipAI);

		// Play Arthur's comments

		Cursor oldCursor = _vm->_gfx->setCursor(kCursorWait);

		((GameUIWindow *)viewWindow->getParent())->_bioChipRightWindow->_forceComment = true;
		((GameUIWindow *)viewWindow->getParent())->_bioChipRightWindow->invalidateWindow(false);
		_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, 10));

		((GameUIWindow *)viewWindow->getParent())->_bioChipRightWindow->_forceComment = false;
		((GameUIWindow *)viewWindow->getParent())->_bioChipRightWindow->_forceHelp = true;
		((GameUIWindow *)viewWindow->getParent())->_bioChipRightWindow->invalidateWindow(false);
		_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, 11));

		((GameUIWindow *)viewWindow->getParent())->_bioChipRightWindow->_forceHelp = false;
		((GameUIWindow *)viewWindow->getParent())->_bioChipRightWindow->invalidateWindow(false);
		_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, 12));
		_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, 13));
		_vm->_gfx->setCursor(oldCursor);
	}

	return SC_TRUE;
}

class HabitatWingLockedDoor : public BaseOxygenTimer {
public:
	HabitatWingLockedDoor(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation,
			int newFrameID = -1, int beepSoundID = -1, int voSoundID = -1, int left = 0, int top = 0, int right = 0, int bottom = 0);
	int mouseUp(Window *viewWindow, const Common::Point &pointLocation);
	int specifyCursor(Window *viewWindow, const Common::Point &pointLocation);

private:
	int _newFrameID;
	Common::Rect _clickRegion;
	int _beepSoundID;
	int _voSoundID;
};

HabitatWingLockedDoor::HabitatWingLockedDoor(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation,
		int newFrameID, int beepSoundID, int voSoundID, int left, int top, int right, int bottom) :
		BaseOxygenTimer(vm, viewWindow, sceneStaticData, priorLocation) {
	_clickRegion = Common::Rect(left, top, right, bottom);
	_newFrameID = newFrameID;
	_beepSoundID = beepSoundID;
	_voSoundID = voSoundID;
}

int HabitatWingLockedDoor::mouseUp(Window *viewWindow, const Common::Point &pointLocation) {
	if (_clickRegion.contains(pointLocation)) {
		int oldFrame = _staticData.navFrameIndex;
		_staticData.navFrameIndex = _newFrameID;
		viewWindow->invalidateWindow(false);

		if (_beepSoundID != -1)
			_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, _beepSoundID));

		if (_voSoundID != -1)
			_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, _voSoundID));

		_staticData.navFrameIndex = oldFrame;
		viewWindow->invalidateWindow(false);

		return SC_TRUE;
	}

	return SC_FALSE;
}

int HabitatWingLockedDoor::specifyCursor(Window *viewWindow, const Common::Point &pointLocation) {
	if (_clickRegion.contains(pointLocation))
		return kCursorFinger;

	return kCursorArrow;
}

class BaseOxygenTimerInSpace : public BaseOxygenTimer {
public:
	BaseOxygenTimerInSpace(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation);
};

BaseOxygenTimerInSpace::BaseOxygenTimerInSpace(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation) :
		BaseOxygenTimer(vm, viewWindow, sceneStaticData, priorLocation) {
	_deathID = 40;
}

class BaseOxygenTimerCapacitance : public SceneBase {
public:
	BaseOxygenTimerCapacitance(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation);
	virtual int postEnterRoom(Window *viewWindow, const Location &priorLocation);
	virtual int preExitRoom(Window *viewWindow, const Location &priorLocation);
	virtual int timerCallback(Window *viewWindow);

protected:
	uint32 _entryStartTime;
	bool _jumped;
};

BaseOxygenTimerCapacitance::BaseOxygenTimerCapacitance(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation) :
		SceneBase(vm, viewWindow, sceneStaticData, priorLocation) {
	_jumped = false;
}

int BaseOxygenTimerCapacitance::postEnterRoom(Window *viewWindow, const Location &priorLocation) {
	_entryStartTime = g_system->getMillis();
	return SC_TRUE;
}

int BaseOxygenTimerCapacitance::preExitRoom(Window *viewWindow, const Location &newLocation) {
	// This does the 25% warning, unlike BaseOxygenTimer

	if (((SceneViewWindow *)viewWindow)->getGlobalFlags().generalWalkthroughMode == 0 && ((SceneViewWindow *)viewWindow)->getGlobalFlags().aiCRPressurized == 0) {
		if (newLocation.timeZone == -2) {
			_jumped = true;
			return SC_TRUE;
		}

		int currentValue = ((SceneViewWindow *)viewWindow)->getGlobalFlags().aiOxygenTimer;

		if (_staticData.location.node != newLocation.node) {
			if (currentValue <= GC_AI_OT_WALK_DECREMENT) {
				if (newLocation.timeZone != -2)
					((SceneViewWindow *)viewWindow)->showDeathScene(41);
				return SC_DEATH;
			} else {
				currentValue -= GC_AI_OT_WALK_DECREMENT;
				((SceneViewWindow *)viewWindow)->getGlobalFlags().aiOxygenTimer = currentValue;

				if (currentValue < GC_AIHW_STARTING_VALUE / 4 || (currentValue % (GC_AIHW_STARTING_VALUE / 10)) == 0) {
					if (currentValue < GC_AIHW_STARTING_VALUE / 4) {
						Common::String oxygenMessage = _vm->getString(IDS_AI_OXY_LEVEL_TEXT_TEMPLATE_LOW);
						assert(!oxygenMessage.empty());
						oxygenMessage = Common::String::format(oxygenMessage.c_str(), currentValue * 100 / GC_AIHW_STARTING_VALUE);
						((SceneViewWindow *)viewWindow)->displayLiveText(oxygenMessage);
					} else {
						Common::String oxygenMessage = _vm->getString(IDS_AI_OXY_LEVEL_TEXT_TEMPLATE_NORM);
						assert(!oxygenMessage.empty());
						oxygenMessage = Common::String::format(oxygenMessage.c_str(), currentValue * 100 / GC_AIHW_STARTING_VALUE);
						((SceneViewWindow *)viewWindow)->displayLiveText(oxygenMessage);
					}
				}
			}
		} else {
			if (currentValue <= GC_AI_OT_TURN_DECREMENT) {
				if (newLocation.timeZone != -2)
					((SceneViewWindow *)viewWindow)->showDeathScene(41);
				return SC_DEATH;
			} else {
				currentValue -= GC_AI_OT_TURN_DECREMENT;
				((SceneViewWindow *)viewWindow)->getGlobalFlags().aiOxygenTimer = currentValue;

				if (currentValue < GC_AIHW_STARTING_VALUE / 4 || (currentValue % (GC_AIHW_STARTING_VALUE / 10)) == 0) {
					if (currentValue < GC_AIHW_STARTING_VALUE / 4) {
						Common::String oxygenMessage = _vm->getString(IDS_AI_OXY_LEVEL_TEXT_TEMPLATE_LOW);
						assert(!oxygenMessage.empty());
						oxygenMessage = Common::String::format(oxygenMessage.c_str(), currentValue * 100 / GC_AIHW_STARTING_VALUE);
						((SceneViewWindow *)viewWindow)->displayLiveText(oxygenMessage);
					} else {
						Common::String oxygenMessage = _vm->getString(IDS_AI_OXY_LEVEL_TEXT_TEMPLATE_NORM);
						assert(!oxygenMessage.empty());
						oxygenMessage = Common::String::format(oxygenMessage.c_str(), currentValue * 100 / GC_AIHW_STARTING_VALUE);
						((SceneViewWindow *)viewWindow)->displayLiveText(oxygenMessage);
					}
				}
			}
		}
	}

	return SC_TRUE;
}

int BaseOxygenTimerCapacitance::timerCallback(Window *viewWindow) {
	// This does the 25% warning, unlike BaseOxygenTimer

	if (((SceneViewWindow *)viewWindow)->getGlobalFlags().generalWalkthroughMode == 0 && ((SceneViewWindow *)viewWindow)->getGlobalFlags().aiCRPressurized == 0) {
		if (_jumped)
			return SC_TRUE;

		if ((g_system->getMillis() - _entryStartTime) >= GC_AI_OT_WAIT_TIME_PERIOD) {
			int currentValue = ((SceneViewWindow *)viewWindow)->getGlobalFlags().aiOxygenTimer;

			if (currentValue <= GC_AI_OT_WAIT_DECREMENT) {
				((SceneViewWindow *)viewWindow)->showDeathScene(41);
				return SC_DEATH;
			} else {
				currentValue -= GC_AI_OT_WAIT_DECREMENT;
				((SceneViewWindow *)viewWindow)->getGlobalFlags().aiOxygenTimer = currentValue;

				if (currentValue < GC_AIHW_STARTING_VALUE / 4 || (currentValue % (GC_AIHW_STARTING_VALUE / 10)) == 0) {
					if (currentValue < GC_AIHW_STARTING_VALUE / 4) {
						Common::String oxygenMessage = _vm->getString(IDS_AI_OXY_LEVEL_TEXT_TEMPLATE_LOW);
						assert(!oxygenMessage.empty());
						oxygenMessage = Common::String::format(oxygenMessage.c_str(), currentValue * 100 / GC_AIHW_STARTING_VALUE);
						((SceneViewWindow *)viewWindow)->displayLiveText(oxygenMessage);
					} else {
						Common::String oxygenMessage = _vm->getString(IDS_AI_OXY_LEVEL_TEXT_TEMPLATE_NORM);
						assert(!oxygenMessage.empty());
						oxygenMessage = Common::String::format(oxygenMessage.c_str(), currentValue * 100 / GC_AIHW_STARTING_VALUE);
						((SceneViewWindow *)viewWindow)->displayLiveText(oxygenMessage);
					}
				}
			}

			_entryStartTime = g_system->getMillis();
		}
	}

	return SC_TRUE;
}

class CapacitanceToHabitatDoorClosed : public BaseOxygenTimerCapacitance {
public:
	CapacitanceToHabitatDoorClosed(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation);
	int mouseDown(Window *viewWindow, const Common::Point &pointLocation);
	int mouseUp(Window *viewWindow, const Common::Point &pointLocation);
	int specifyCursor(Window *viewWindow, const Common::Point &pointLocation);
	int draggingItem(Window *viewWindow, int itemID, const Common::Point &pointLocation, int itemFlags);
	int droppedItem(Window *viewWindow, int itemID, const Common::Point &pointLocation, int itemFlags);

private:
	Common::Rect _metalBar;
	Common::Rect _door;
};

CapacitanceToHabitatDoorClosed::CapacitanceToHabitatDoorClosed(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation) :
		BaseOxygenTimerCapacitance(vm, viewWindow, sceneStaticData, priorLocation) {
	if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiCRGrabbedMetalBar == 1)
		_staticData.navFrameIndex = 7;
	else
		_staticData.navFrameIndex = 55;

	_metalBar = Common::Rect(184, 146, 264, 184);
	_door = Common::Rect(132, 14, 312, 180);
}

int CapacitanceToHabitatDoorClosed::mouseDown(Window *viewWindow, const Common::Point &pointLocation) {
	if (_metalBar.contains(pointLocation) && ((SceneViewWindow *)viewWindow)->getGlobalFlags().aiCRGrabbedMetalBar == 0) {
		_staticData.navFrameIndex = 7;
		((SceneViewWindow *)viewWindow)->getGlobalFlags().aiCRGrabbedMetalBar = 1;

		Common::Point ptInventoryWindow = viewWindow->convertPointToWindow(pointLocation, ((GameUIWindow *)viewWindow->getParent())->_inventoryWindow);
		((GameUIWindow *)viewWindow->getParent())->_inventoryWindow->startDraggingNewItem(kItemMetalBar, ptInventoryWindow);

		((GameUIWindow *)viewWindow->getParent())->_bioChipRightWindow->sceneChanged();
		return SC_TRUE;
	}

	return SC_FALSE;
}

int CapacitanceToHabitatDoorClosed::mouseUp(Window *viewWindow, const Common::Point &pointLocation) {
	if (_door.contains(pointLocation)) {
		if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiCRGrabbedMetalBar == 0) {
			_staticData.navFrameIndex = 96;
			viewWindow->invalidateWindow(false);

			// Wait for a second (why?)
			uint32 startTime = g_system->getMillis();

			while (!_vm->shouldQuit() && g_system->getMillis() < startTime + 1000) {
				_vm->yield();
				_vm->_sound->timerCallback();
			}

			DestinationScene destData;
			destData.destinationScene = _staticData.location;
			destData.destinationScene.depth = 1;
			destData.transitionType = TRANSITION_VIDEO;
			destData.transitionData = 1;
			destData.transitionStartFrame = -1;
			destData.transitionLength = -1;

			// Move to the final destination
			((SceneViewWindow *)viewWindow)->moveToDestination(destData);
			return SC_TRUE;
		} else {
			if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiCRPressurized == 0) {
				_staticData.navFrameIndex = 97;
				viewWindow->invalidateWindow(false);

				// Wait for a second (why?)
				uint32 startTime = g_system->getMillis();

				while (!_vm->shouldQuit() && g_system->getMillis() < startTime + 1000) {
					_vm->yield();
					_vm->_sound->timerCallback();
				}

				DestinationScene destData;
				destData.destinationScene = _staticData.location;
				destData.destinationScene.depth = 1;
				destData.transitionType = TRANSITION_VIDEO;
				destData.transitionData = 2;
				destData.transitionStartFrame = -1;
				destData.transitionLength = -1;

				// Move to the final destination
				((SceneViewWindow *)viewWindow)->moveToDestination(destData);
				return SC_TRUE;
			} else {
				int oldFrame = _staticData.navFrameIndex;
				_staticData.navFrameIndex = 121;
				viewWindow->invalidateWindow(false);

				_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment - 1, 12));
				_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment - 1, 13));

				_staticData.navFrameIndex = oldFrame;
				viewWindow->invalidateWindow(false);
				return SC_TRUE;
			}
		}
	}

	return SC_FALSE;
}

int CapacitanceToHabitatDoorClosed::specifyCursor(Window *viewWindow, const Common::Point &pointLocation) {
	if (_metalBar.contains(pointLocation) && ((SceneViewWindow *)viewWindow)->getGlobalFlags().aiCRGrabbedMetalBar == 0)
		return kCursorOpenHand;

	if (_door.contains(pointLocation))
		return kCursorFinger;

	return kCursorArrow;
}

int CapacitanceToHabitatDoorClosed::draggingItem(Window *viewWindow, int itemID, const Common::Point &pointLocation, int itemFlags) {
	if (itemID == kItemMetalBar && ((SceneViewWindow *)viewWindow)->getGlobalFlags().aiCRGrabbedMetalBar == 1)
		return 1;

	return 0;
}

int CapacitanceToHabitatDoorClosed::droppedItem(Window *viewWindow, int itemID, const Common::Point &pointLocation, int itemFlags) {
	if (pointLocation.x == -1 && pointLocation.y == -1)
		return 0; // ???

	return SIC_REJECT;
}

class CapacitanceToHabitatDoorOpen : public BaseOxygenTimerCapacitance {
public:
	CapacitanceToHabitatDoorOpen(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation);
	int postExitRoom(Window *viewWindow, const Location &newLocation);
	int mouseDown(Window *viewWindow, const Common::Point &pointLocation);
	int specifyCursor(Window *viewWindow, const Common::Point &pointLocation);
	int draggingItem(Window *viewWindow, int itemID, const Common::Point &pointLocation, int itemFlags);
	int droppedItem(Window *viewWindow, int itemID, const Common::Point &pointLocation, int itemFlags);

private:
	Common::Rect _metalBar;
};

CapacitanceToHabitatDoorOpen::CapacitanceToHabitatDoorOpen(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation) :
		BaseOxygenTimerCapacitance(vm, viewWindow, sceneStaticData, priorLocation) {
	if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiCRGrabbedMetalBar == 1) {
		_staticData.navFrameIndex = 101;
		_staticData.destForward.transitionStartFrame = 0;
		_staticData.destForward.transitionLength = 28;
	} else {
		_staticData.navFrameIndex = 100;
		_staticData.destForward.transitionStartFrame = 53;
		_staticData.destForward.transitionLength = 28;
	}

	_metalBar = Common::Rect(184, 146, 264, 184);
}

int CapacitanceToHabitatDoorOpen::postExitRoom(Window *viewWindow, const Location &newLocation) {
	// Play the door closing sound
	if (_staticData.location.timeZone == newLocation.timeZone)
		_vm->_sound->playSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, 14), 128, false, true);

	return SC_TRUE;
}

int CapacitanceToHabitatDoorOpen::mouseDown(Window *viewWindow, const Common::Point &pointLocation) {
	if (_metalBar.contains(pointLocation) && ((SceneViewWindow *)viewWindow)->getGlobalFlags().aiCRGrabbedMetalBar == 0) {
		_staticData.navFrameIndex = 101;
		((SceneViewWindow *)viewWindow)->getGlobalFlags().aiCRGrabbedMetalBar = 1;
		_staticData.destForward.transitionStartFrame = 0;
		_staticData.destForward.transitionLength = 28;

		Common::Point ptInventoryWindow = viewWindow->convertPointToWindow(pointLocation, ((GameUIWindow *)viewWindow->getParent())->_inventoryWindow);
		((GameUIWindow *)viewWindow->getParent())->_inventoryWindow->startDraggingNewItem(kItemMetalBar, ptInventoryWindow);

		((GameUIWindow *)viewWindow->getParent())->_bioChipRightWindow->sceneChanged();
		return SC_TRUE;
	}

	return SC_FALSE;
}

int CapacitanceToHabitatDoorOpen::specifyCursor(Window *viewWindow, const Common::Point &pointLocation) {
	if (_metalBar.contains(pointLocation) && ((SceneViewWindow *)viewWindow)->getGlobalFlags().aiCRGrabbedMetalBar == 0)
		return kCursorOpenHand;

	return kCursorArrow;
}

int CapacitanceToHabitatDoorOpen::draggingItem(Window *viewWindow, int itemID, const Common::Point &pointLocation, int itemFlags) {
	if (itemID == kItemMetalBar && ((SceneViewWindow *)viewWindow)->getGlobalFlags().aiCRGrabbedMetalBar == 1)
		return 1;

	return 0;
}

int CapacitanceToHabitatDoorOpen::droppedItem(Window *viewWindow, int itemID, const Common::Point &pointLocation, int itemFlags) {
	if (pointLocation.x == -1 && pointLocation.y == -1)
		return 0; // ???

	if (itemID == kItemMetalBar && ((SceneViewWindow *)viewWindow)->getGlobalFlags().aiCRGrabbedMetalBar == 1) {
		_staticData.navFrameIndex = 100;
		((SceneViewWindow *)viewWindow)->getGlobalFlags().aiCRGrabbedMetalBar = 0;
		viewWindow->invalidateWindow(false);
		_staticData.destForward.transitionStartFrame = 53;
		_staticData.destForward.transitionLength = 28;

		((GameUIWindow *)viewWindow->getParent())->_bioChipRightWindow->sceneChanged();
		return SIC_ACCEPT;
	}

	return SIC_REJECT;
}

class CapacitancePanelInterface : public BaseOxygenTimerCapacitance {
public:
	CapacitancePanelInterface(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation);
	~CapacitancePanelInterface();
	void preDestructor();
	int mouseUp(Window *viewWindow, const Common::Point &pointLocation);
	int specifyCursor(Window *viewWindow, const Common::Point &pointLocation);
	int gdiPaint(Window *viewWindow);

private:
	Common::Rect _stationRegions[15];
	int _currentSelection;
	int _currentTextIndex;
	int _lineHeight;
	Graphics::Font *_textFont;
	Common::Rect _leftTextRegion;
	Common::Rect _rightTextRegion;
};

CapacitancePanelInterface::CapacitancePanelInterface(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation) :
		BaseOxygenTimerCapacitance(vm, viewWindow, sceneStaticData, priorLocation) {
	_currentSelection = -1;
	_currentTextIndex = -1;
	_stationRegions[0] = Common::Rect(265, 110, 286, 135);
	_stationRegions[1] = Common::Rect(102, 45, 180, 134);
	_stationRegions[2] = Common::Rect(195, 106, 216, 133);
	_stationRegions[3] = Common::Rect(268, 72, 283, 87);
	_stationRegions[4] = Common::Rect(221, 46, 236, 74);
	_stationRegions[5] = Common::Rect(290, 72, 317, 108);
	_stationRegions[6] = Common::Rect(264, 55, 288, 67);
	_stationRegions[7] = Common::Rect(194, 74, 266, 84);
	_stationRegions[8] = Common::Rect(198, 62, 214, 74);
	_stationRegions[9] = Common::Rect(221, 106, 236, 134);
	_stationRegions[10] = Common::Rect(245, 46, 260, 74);
	_stationRegions[11] = Common::Rect(245, 106, 260, 134);
	_stationRegions[12] = Common::Rect(266, 92, 290, 109);
	_stationRegions[13] = Common::Rect(194, 96, 264, 106);
	_stationRegions[14] = Common::Rect(180, 85, 194, 94);
	_leftTextRegion = Common::Rect(83, 144, 211, 170);
	_rightTextRegion = Common::Rect(228, 144, 356, 170);
	_lineHeight = _vm->getLanguage() == Common::JA_JPN ? 10 : 13;
	_textFont = _vm->_gfx->createFont(_lineHeight);
}

CapacitancePanelInterface::~CapacitancePanelInterface() {
	preDestructor();
}

void CapacitancePanelInterface::preDestructor() {
	delete _textFont;
	_textFont = 0;
}

int CapacitancePanelInterface::mouseUp(Window *viewWindow, const Common::Point &pointLocation) {
	byte &oxygenReserves = ((SceneViewWindow *)viewWindow)->getGlobalFlags().aiOxygenReserves;

	if (_currentSelection == 2) {
		if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiMRPressurized == 0 && (_stationRegions[_currentSelection].contains(pointLocation) || _rightTextRegion.contains(pointLocation))) {
			if (oxygenReserves > 0) {
				// Decrement reserves flag
				oxygenReserves--;

				// Set the machine room to pressurized
				((SceneViewWindow *)viewWindow)->getGlobalFlags().aiMRPressurized = 1;

				// Display pressurizing message
				viewWindow->invalidateWindow(false);
				_currentTextIndex = IDS_AI_PRES_PANEL_PRES_ENV_TEXT;

				// Play sound file
				_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, 13), 128);

				// Display pressurized text
				viewWindow->invalidateWindow(false);
				_currentTextIndex = IDS_AI_PRES_PANEL_ENV_PRES_TEXT;
				return SC_TRUE;
			} else {
				// Not enough oxygen reserves
				viewWindow->invalidateWindow(false);
				_currentTextIndex = IDS_AI_PRES_PANEL_INSUF_OXYGEN;
				return SC_TRUE;
			}
		}
	} else if (_currentSelection == 3) {
		if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiCRPressurized == 0 && (_stationRegions[_currentSelection].contains(pointLocation) || _rightTextRegion.contains(pointLocation))) {
			if (oxygenReserves > 0) {
				if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiCRGrabbedMetalBar == 0) {
					if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiCRPressurizedAttempted == 0) {
						// Display pressurizing message
						viewWindow->invalidateWindow(false);
						_currentTextIndex = IDS_AI_PRES_PANEL_PRES_ENV_TEXT;

						// Play sound file
						_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, 13), 128);

						// Display bulkhead message
						viewWindow->invalidateWindow(false);
						_currentTextIndex = IDS_AI_PRES_PANEL_ENV_DEPRES_BREACH;

						// Play Mom audio
						// (Is this an Alien reference?)
						_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, 11));

						// Update attempt flag
						((SceneViewWindow *)viewWindow)->getGlobalFlags().aiCRPressurizedAttempted = 1;
					}
				} else {
					// Decrement reserves flag
					oxygenReserves--;

					// Set the capacitance array to pressurized
					((SceneViewWindow *)viewWindow)->getGlobalFlags().aiCRPressurized = 1;

					// Display pressurizing message
					viewWindow->invalidateWindow(false);
					_currentTextIndex = IDS_AI_PRES_PANEL_PRES_ENV_TEXT;

					// Play sound file
					_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, 13), 128);

					// Display pressurized text
					viewWindow->invalidateWindow(false);
					_currentTextIndex = IDS_AI_PRES_PANEL_ENV_PRES_TEXT;

					// Display oxygen text in the message window
					((SceneViewWindow *)viewWindow)->displayLiveText(_vm->getString(IDS_AI_ENTERING_PRES_ENV_TEXT));
					((SceneViewWindow *)viewWindow)->getGlobalFlags().aiOxygenTimer = GC_AIHW_STARTING_VALUE;
				}

				return SC_TRUE;
			} else {
				// Not enough oxygen reserves
				viewWindow->invalidateWindow(false);
				_currentTextIndex = IDS_AI_PRES_PANEL_INSUF_OXYGEN;
				return SC_TRUE;
			}
		}
	}

	// Check against the hotspots
	for (int i = 0; i < 15; i++) {
		if (_stationRegions[i].contains(pointLocation) && _currentSelection != i) {
			switch (i) {
			case 0:
				_staticData.navFrameIndex = 107;
				viewWindow->invalidateWindow(false);
				_currentSelection = i;
				_currentTextIndex = IDS_AI_PRES_PANEL_ENV_PRES_TEXT;
				return SC_TRUE;
			case 1:
				_staticData.navFrameIndex = 108;
				viewWindow->invalidateWindow(false);
				_currentSelection = i;
				_currentTextIndex = IDS_AI_PRES_PANEL_ZERO_PRES_ENV;
				return SC_TRUE;
			case 2:
				_staticData.navFrameIndex = 109;
				viewWindow->invalidateWindow(false);
				_currentSelection = i;

				if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiMRPressurized == 1)
					_currentTextIndex = IDS_AI_PRES_PANEL_ENV_PRES_TEXT;
				else
					_currentTextIndex = IDS_AI_PRES_PANEL_ENV_DEPRES;
				return SC_TRUE;
			case 3:
				_staticData.navFrameIndex = 110;
				viewWindow->invalidateWindow(false);
				_currentSelection = i;

				if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiCRPressurized == 1)
					_currentTextIndex = IDS_AI_PRES_PANEL_ENV_PRES_TEXT;
				else
					_currentTextIndex = IDS_AI_PRES_PANEL_ENV_DEPRES;
				return SC_TRUE;
			case 4:
				_staticData.navFrameIndex = 111;
				viewWindow->invalidateWindow(false);
				_currentSelection = i;
				_currentTextIndex = IDS_AI_PRES_PANEL_ENV_DEPRES_BREACH;
				return SC_TRUE;
			case 5:
				_staticData.navFrameIndex = 112;
				viewWindow->invalidateWindow(false);
				_currentSelection = i;
				_currentTextIndex = IDS_AI_PRES_PANEL_ENV_PRES_TEXT;
				return SC_TRUE;
			case 6:
				_staticData.navFrameIndex = 113;
				viewWindow->invalidateWindow(false);
				_currentSelection = i;
				_currentTextIndex = IDS_AI_PRES_PANEL_ENV_DEPRES_BREACH;
				return SC_TRUE;
			case 7:
				_staticData.navFrameIndex = 114;
				viewWindow->invalidateWindow(false);
				_currentSelection = i;
				_currentTextIndex = IDS_AI_PRES_PANEL_ENV_DEPRES_BREACH;
				return SC_TRUE;
			case 8:
				_staticData.navFrameIndex = 115;
				viewWindow->invalidateWindow(false);
				_currentSelection = i;
				_currentTextIndex = IDS_AI_PRES_PANEL_ENV_DEPRES_BREACH;
				return SC_TRUE;
			case 9:
				_staticData.navFrameIndex = 116;
				viewWindow->invalidateWindow(false);
				_currentSelection = i;
				_currentTextIndex = IDS_AI_PRES_PANEL_ENV_DEPRES_BREACH;
				return SC_TRUE;
			case 10:
				_staticData.navFrameIndex = 117;
				viewWindow->invalidateWindow(false);
				_currentSelection = i;
				_currentTextIndex = IDS_AI_PRES_PANEL_ENV_DEPRES_BREACH;
				return SC_TRUE;
			case 11:
				_staticData.navFrameIndex = 118;
				viewWindow->invalidateWindow(false);
				_currentSelection = i;
				_currentTextIndex = IDS_AI_PRES_PANEL_ENV_DEPRES_BREACH;
				return SC_TRUE;
			case 12:
				_staticData.navFrameIndex = 119;
				viewWindow->invalidateWindow(false);
				_currentSelection = i;
				_currentTextIndex = IDS_AI_PRES_PANEL_ENV_PRES_TEXT;
				return SC_TRUE;
			case 13:
			case 14:
				_staticData.navFrameIndex = 120;
				viewWindow->invalidateWindow(false);
				_currentSelection = i;
				_currentTextIndex = IDS_AI_PRES_PANEL_ENV_DEPRES_BREACH;
				return SC_TRUE;
			}
		}
	}

	// By default, return to depth zero (zoomed out)
	DestinationScene destData;
	destData.destinationScene = _staticData.location;
	destData.destinationScene.depth = 0;
	destData.transitionType = TRANSITION_NONE;
	destData.transitionData = -1;
	destData.transitionStartFrame = -1;
	destData.transitionLength = -1;
	((SceneViewWindow *)viewWindow)->moveToDestination(destData);
	return SC_TRUE;
}

int CapacitancePanelInterface::specifyCursor(Window *viewWindow, const Common::Point &pointLocation) {
	for (int i = 0; i < 15; i++)
		if (_stationRegions[i].contains(pointLocation))
			return kCursorFinger;

	return kCursorPutDown;
}

int CapacitancePanelInterface::gdiPaint(Window *viewWindow) {
	if (_currentSelection >= 0) {
		uint32 color = _vm->_gfx->getColor(208, 144, 24);

		Common::String location = _vm->getString(IDS_AI_PRES_PANEL_DESC_BASE + _currentSelection);
		if (_currentSelection == 3)
			location += _vm->getString(IDS_AI_PRES_PANEL_DESC_BASE + 19);

		Common::Rect absoluteRect = viewWindow->getAbsoluteRect();
		Common::Rect rect(_leftTextRegion);
		rect.translate(absoluteRect.left, absoluteRect.top);
		_vm->_gfx->renderText(_vm->_gfx->getScreen(), _textFont, location, rect.left, rect.top, rect.width(), rect.height(), color, _lineHeight, kTextAlignCenter, true);

		if (_currentTextIndex >= 0) {
			rect = _rightTextRegion;
			rect.translate(absoluteRect.left, absoluteRect.top);
			_vm->_gfx->renderText(_vm->_gfx->getScreen(), _textFont, _vm->getString(_currentTextIndex), rect.left, rect.top, rect.width(), rect.height(), color, _lineHeight, kTextAlignCenter, true);
		}
	}

	return SC_FALSE;
}

class PlayArthurOffsetCapacitance : public BaseOxygenTimerCapacitance {
public:
	PlayArthurOffsetCapacitance(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation,
			int stingerVolume = 127, int lastStingerFlagOffset = -1, int effectIDFlagOffset = -1, int firstStingerFileID = -1,
			int lastStingerFileID = -1, int stingerDelay = 1, int flagOffset = -1, int newStill = -1, int newNavStart = -1, int newNavLength = -1);
	int postEnterRoom(Window *viewWindow, const Location &priorLocation);

private:
	int _stingerVolume;
	int _lastStingerFlagOffset;
	int _effectIDFlagOffset;
	int _firstStingerFileID;
	int _lastStingerFileID;
	int _stingerDelay;
};

PlayArthurOffsetCapacitance::PlayArthurOffsetCapacitance(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation,
		int stingerVolume, int lastStingerFlagOffset, int effectIDFlagOffset, int firstStingerFileID,
		int lastStingerFileID, int stingerDelay, int flagOffset, int newStill, int newNavStart, int newNavLength) :
		BaseOxygenTimerCapacitance(vm, viewWindow, sceneStaticData, priorLocation) {
	_stingerVolume = stingerVolume;
	_lastStingerFlagOffset = lastStingerFlagOffset;
	_effectIDFlagOffset = effectIDFlagOffset;
	_firstStingerFileID = firstStingerFileID;
	_lastStingerFileID = lastStingerFileID;
	_stingerDelay = stingerDelay;

	if (flagOffset >= 0 && ((SceneViewWindow *)viewWindow)->getGlobalFlagByte(flagOffset) == 0) {
		// This is completely wrong.
		//if (newStill >= 0)
		//	_staticData.navFrameIndex;
		if (newNavStart >= 0)
			_staticData.destForward.transitionStartFrame = newNavStart;
		if (newNavLength >= 0)
			_staticData.destForward.transitionLength = newNavLength;
	}
}

int PlayArthurOffsetCapacitance::postEnterRoom(Window *viewWindow, const Location &priorLocation) {
	BaseOxygenTimerCapacitance::postEnterRoom(viewWindow, priorLocation);

	if (_effectIDFlagOffset >= 0) {
		byte effectID = ((SceneViewWindow *)viewWindow)->getGlobalFlagByte(_effectIDFlagOffset);

		if (!_vm->_sound->isSoundEffectPlaying(effectID - 1)) {
			byte lastStinger = ((SceneViewWindow *)viewWindow)->getGlobalFlagByte(_lastStingerFlagOffset) + 1;

			if ((lastStinger % _stingerDelay) == 0) {
				if (lastStinger < (_lastStingerFileID - _firstStingerFileID) * _stingerDelay) {
					int fileNameIndex = _vm->computeFileNameResourceID(_staticData.location.timeZone, _staticData.location.environment, _firstStingerFileID + (lastStinger / _stingerDelay) - 1);

					if (((GameUIWindow *)viewWindow->getParent())->_inventoryWindow->isItemInInventory(kItemBioChipAI) && (lastStinger / _stingerDelay) < 3) {
						_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(fileNameIndex));

						// Play an Arthur comment if we have the chip
						switch (lastStinger / _stingerDelay) {
						case 0:
							_vm->_sound->playSynchronousSoundEffect("BITDATA/AILAB/AICR_C01.BTA", 127);
							break;
						case 1:
							_vm->_sound->playSynchronousSoundEffect("BITDATA/AILAB/AICR_C02.BTA", 127);
							break;
						case 2:
							_vm->_sound->playSynchronousSoundEffect("BITDATA/AILAB/AICR_C03.BTA", 127);
							break;
						}

						// Update the global flags
						((SceneViewWindow *)viewWindow)->setGlobalFlagByte(_lastStingerFlagOffset, lastStinger);
					} else {
						byte newStingerID = _vm->_sound->playSoundEffect(_vm->getFilePath(fileNameIndex), _stingerVolume, false, true) + 1;

						// Update the global flags
						((SceneViewWindow *)viewWindow)->setGlobalFlagByte(_effectIDFlagOffset, newStingerID);
						((SceneViewWindow *)viewWindow)->setGlobalFlagByte(_lastStingerFlagOffset, lastStinger);
					}
				}
			} else {
				((SceneViewWindow *)viewWindow)->setGlobalFlagByte(_effectIDFlagOffset, 0xFF);
				((SceneViewWindow *)viewWindow)->setGlobalFlagByte(_lastStingerFlagOffset, lastStinger);
			}
		}
	}

	return SC_TRUE;
}

class ClickChangeSceneCapacitance : public BaseOxygenTimerCapacitance {
public:
	ClickChangeSceneCapacitance(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation,
			int left = -1, int top = -1, int right = -1, int bottom = -1, int cursorID = 0,
			int timeZone = -1, int environment = -1, int node = -1, int facing = -1, int orientation = -1, int depth = -1,
			int transitionType = -1, int transitionData = -1, int transitionStartFrame = -1, int transitionLength = -1);
	int mouseUp(Window *viewWindow, const Common::Point &pointLocation);
	int specifyCursor(Window *viewWindow, const Common::Point &pointLocation);

private:
	int _cursorID;
	Common::Rect _clickRegion;
	DestinationScene _clickDestination;
};

ClickChangeSceneCapacitance::ClickChangeSceneCapacitance(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation,
		int left, int top, int right, int bottom, int cursorID,
		int timeZone, int environment, int node, int facing, int orientation, int depth,
		int transitionType, int transitionData, int transitionStartFrame, int transitionLength) :
		BaseOxygenTimerCapacitance(vm, viewWindow, sceneStaticData, priorLocation) {
	_clickRegion = Common::Rect(left, top, right, bottom);
	_cursorID = cursorID;
	_clickDestination.destinationScene = Location(timeZone, environment, node, facing, orientation, depth);
	_clickDestination.transitionType = transitionType;
	_clickDestination.transitionData = transitionData;
	_clickDestination.transitionStartFrame = transitionStartFrame;
	_clickDestination.transitionLength = transitionLength;
}

int ClickChangeSceneCapacitance::mouseUp(Window *viewWindow, const Common::Point &pointLocation) {
	if (_clickRegion.contains(pointLocation))
		((SceneViewWindow *)viewWindow)->moveToDestination(_clickDestination);

	return SC_FALSE;
}

int ClickChangeSceneCapacitance::specifyCursor(Window *viewWindow, const Common::Point &pointLocation) {
	if (_clickRegion.contains(pointLocation))
		return _cursorID;

	return kCursorArrow;
}

class CapacitanceDockingBayDoor : public BaseOxygenTimerCapacitance {
public:
	CapacitanceDockingBayDoor(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation);
	int mouseUp(Window *viewWindow, const Common::Point &pointLocation);
	int specifyCursor(Window *viewWindow, const Common::Point &pointLocation);

private:
	Common::Rect _door;
};

CapacitanceDockingBayDoor::CapacitanceDockingBayDoor(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation) :
		BaseOxygenTimerCapacitance(vm, viewWindow, sceneStaticData, priorLocation) {
	_door = Common::Rect(160, 54, 276, 168);
}

int CapacitanceDockingBayDoor::mouseUp(Window *viewWindow, const Common::Point &pointLocation) {
	if (_door.contains(pointLocation)) {
		if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiCRPressurized == 1) {
			_staticData.navFrameIndex = 98;
			viewWindow->invalidateWindow(false);

			_vm->_sound->playSynchronousSoundEffect("BITDATA/AILAB/AI_LOCK.BTA");

			// Wait a second?
			uint32 startTime = g_system->getMillis();
			while (!_vm->shouldQuit() && g_system->getMillis() < startTime + 1000) {
				_vm->yield();
				_vm->_sound->timerCallback();
			}

			DestinationScene destData;
			destData.destinationScene = _staticData.location;
			destData.destinationScene.depth = 1;
			destData.transitionType = TRANSITION_VIDEO;
			destData.transitionData = 0;
			destData.transitionStartFrame = -1;
			destData.transitionLength = -1;

			// Move to the final destination
			((SceneViewWindow *)viewWindow)->moveToDestination(destData);
			return SC_TRUE;
		} else {
			int oldFrame = _staticData.navFrameIndex;
			_staticData.navFrameIndex = 99;
			viewWindow->invalidateWindow(false);

			_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment - 1, 12));
			_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment - 1, 13));

			_staticData.navFrameIndex = oldFrame;
			viewWindow->invalidateWindow(false);
			return SC_TRUE;
		}
	}

	return SC_FALSE;
}

int CapacitanceDockingBayDoor::specifyCursor(Window *viewWindow, const Common::Point &pointLocation) {
	if (_door.contains(pointLocation))
		return kCursorFinger;

	return kCursorArrow;
}

class ScanningRoomEntryScan : public SceneBase {
public:
	ScanningRoomEntryScan(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation);
	int postEnterRoom(Window *viewWindow, const Location &priorLocation);
	int postExitRoom(Window *viewWindow, const Location &newLocation);
	int timerCallback(Window *viewWindow);

private:
	DestinationScene _savedForwardData;
};

ScanningRoomEntryScan::ScanningRoomEntryScan(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation) :
		SceneBase(vm, viewWindow, sceneStaticData, priorLocation) {
	_savedForwardData = _staticData.destForward;

	if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCHeardInitialSpeech == 0)
		_staticData.destForward.destinationScene = Location(-1, -1, -1, -1, -1, -1);

	if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCInitialAudioChannel > 0) {
		if (_vm->_sound->isSoundEffectPlaying(((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCInitialAudioChannel - 1))
			_staticData.destForward.destinationScene = Location(-1, -1, -1, -1, -1, -1);
		else
			((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCInitialAudioChannel = 0;
	}

	if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCConversationStatus == 3)
		_staticData.destForward.destinationScene = Location(-1, -1, -1, -1, -1, -1);
}

int ScanningRoomEntryScan::postEnterRoom(Window *viewWindow, const Location &priorLocation) {
	if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCHeardInitialSpeech == 0) {
		// Play the scanning movie
		((SceneViewWindow *)viewWindow)->playSynchronousAnimation(7);

		// Set the flag
		((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCHeardInitialSpeech = 1;

		// Start the initial monologue
		byte channel = _vm->_sound->playSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, 12), 127, false, true) + 1;
		((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCInitialAudioChannel = channel;
	}

	return SC_TRUE;
}

int ScanningRoomEntryScan::postExitRoom(Window *viewWindow, const Location &newLocation) {
	if (newLocation.timeZone == 6 && newLocation.environment == 4 && newLocation.node != 3 && newLocation.node != 0 &&
			_staticData.location.timeZone == newLocation.timeZone) {
		_vm->_sound->playSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, 13), 128, false, true);
	}

	return SC_TRUE;
}

int ScanningRoomEntryScan::timerCallback(Window *viewWindow) {
	if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCInitialAudioChannel > 0 && !_vm->_sound->isSoundEffectPlaying(((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCInitialAudioChannel - 1)) {
		_staticData.destForward = _savedForwardData;
		((GameUIWindow *)viewWindow->getParent())->_navArrowWindow->updateAllArrows(_staticData);
		((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCInitialAudioChannel = 0;
	}

	return SC_TRUE;
}

class ScanningRoomWalkWarning : public SceneBase {
public:
	ScanningRoomWalkWarning(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation);
	int postExitRoom(Window *viewWindow, const Location &newLocation);
	int timerCallback(Window *viewWindow);

private:
	DestinationScene _savedForwardData;
};

ScanningRoomWalkWarning::ScanningRoomWalkWarning(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation) :
		SceneBase(vm, viewWindow, sceneStaticData, priorLocation) {
	_savedForwardData = _staticData.destForward;

	if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCInitialAudioChannel > 0) {
		if (_vm->_sound->isSoundEffectPlaying(((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCInitialAudioChannel - 1))
			_staticData.destForward.destinationScene = Location(-1, -1, -1, -1, -1, -1);
		else
			((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCInitialAudioChannel = 0;
	}
}

int ScanningRoomWalkWarning::postExitRoom(Window *viewWindow, const Location &newLocation) {
	if (newLocation.timeZone == 6 && newLocation.environment == 4 && newLocation.node != 3 && newLocation.node != 0 &&
			((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCMoveCenterWarning == 0) {
		if (_staticData.location.timeZone == newLocation.timeZone)
			_vm->_sound->playSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, 13), 128, false, true);
		((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCMoveCenterWarning = 1;
	}

	return SC_TRUE;
}

int ScanningRoomWalkWarning::timerCallback(Window *viewWindow) {
	if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCInitialAudioChannel > 0 && !_vm->_sound->isSoundEffectPlaying(((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCInitialAudioChannel - 1)) {
		_staticData.destForward = _savedForwardData;
		((GameUIWindow *)viewWindow->getParent())->_navArrowWindow->updateAllArrows(_staticData);
		((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCInitialAudioChannel = 0;
	}

	return SC_TRUE;
}

class ScanningRoomDockingBayDoor : public SceneBase {
public:
	ScanningRoomDockingBayDoor(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation);
	int mouseUp(Window *viewWindow, const Common::Point &pointLocation);
	int timerCallback(Window *viewWindow);
	int specifyCursor(Window *viewWindow, const Common::Point &pointLocation);

private:
	bool _audioEnded;
	Common::Rect _doorRegion;
};

ScanningRoomDockingBayDoor::ScanningRoomDockingBayDoor(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation) :
		SceneBase(vm, viewWindow, sceneStaticData, priorLocation) {
	_audioEnded = true;
	_doorRegion = Common::Rect(152, 34, 266, 148);

	if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCInitialAudioChannel > 0) {
		if (!_vm->_sound->isSoundEffectPlaying(((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCInitialAudioChannel - 1)) {
			((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCInitialAudioChannel = 0;
			_audioEnded = true;
		} else {
			_audioEnded = false;
		}
	}
}

int ScanningRoomDockingBayDoor::mouseUp(Window *viewWindow, const Common::Point &pointLocation) {
	if (_audioEnded && _doorRegion.contains(pointLocation)) {
		// Change the still frame
		int oldFrame = _staticData.navFrameIndex;
		_staticData.navFrameIndex = 46;
		viewWindow->invalidateWindow(false);

		// Play the beep and voiceovers
		_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, 1, 12));
		_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, 1, 13));

		// Reset the frame
		_staticData.navFrameIndex = oldFrame;
		viewWindow->invalidateWindow(false);

		// Play Arthur's voiceover
		if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCDBDoorWarning == 0) {
			_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, 11), 127);
			((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCDBDoorWarning = 1;
		}
	}

	return SC_FALSE;
}

int ScanningRoomDockingBayDoor::timerCallback(Window *viewWindow) {
	if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCInitialAudioChannel > 0 && !_vm->_sound->isSoundEffectPlaying(((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCInitialAudioChannel - 1)) {
		((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCInitialAudioChannel = 0;
		_audioEnded = true;
	}

	return SC_TRUE;
}

int ScanningRoomDockingBayDoor::specifyCursor(Window *viewWindow, const Common::Point &pointLocation) {
	if (_audioEnded && _doorRegion.contains(pointLocation))
		return kCursorFinger;

	return kCursorArrow;
}

class ScanningRoomScienceWingDoor : public SceneBase {
public:
	ScanningRoomScienceWingDoor(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation);
	int mouseUp(Window *viewWindow, const Common::Point &pointLocation);
	int specifyCursor(Window *viewWindow, const Common::Point &pointLocation);

private:
	Common::Rect _doorRegion;
};

ScanningRoomScienceWingDoor::ScanningRoomScienceWingDoor(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation) :
		SceneBase(vm, viewWindow, sceneStaticData, priorLocation) {
	_doorRegion = Common::Rect(152, 34, 266, 148);
}

int ScanningRoomScienceWingDoor::mouseUp(Window *viewWindow, const Common::Point &pointLocation) {
	if (_doorRegion.contains(pointLocation)) {
		// Change the still frame
		int oldFrame = _staticData.navFrameIndex;
		_staticData.navFrameIndex = 44;
		viewWindow->invalidateWindow(false);

		// Play the beep and voiceovers
		_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, 1, 12));
		_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, 1, 13));

		// Reset the frame
		_staticData.navFrameIndex = oldFrame;
		viewWindow->invalidateWindow(false);
	}

	return SC_FALSE;
}

int ScanningRoomScienceWingDoor::specifyCursor(Window *viewWindow, const Common::Point &pointLocation) {
	if (_doorRegion.contains(pointLocation))
		return kCursorFinger;

	return kCursorArrow;
}

class ArthurScanningRoomConversation : public SceneBase {
public:
	ArthurScanningRoomConversation(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation);
	int postEnterRoom(Window *viewWindow, const Location &priorLocation);
	int mouseUp(Window *viewWindow, const Common::Point &pointLocation);
	int specifyCursor(Window *viewWindow, const Common::Point &pointLocation);

private:
	Common::Rect _yes;
	Common::Rect _no;
};

ArthurScanningRoomConversation::ArthurScanningRoomConversation(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation) :
		SceneBase(vm, viewWindow, sceneStaticData, priorLocation) {
	_yes = Common::Rect(152, 54, 284, 124);
	_no = Common::Rect(194, 128, 244, 152);
}

int ArthurScanningRoomConversation::postEnterRoom(Window *viewWindow, const Location &priorLocation) {
	// If this is the initial entry, play Arthur's comment
	if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCConversationStatus == 0) {
		((SceneViewWindow *)viewWindow)->playSynchronousAnimation(9);
		((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCConversationStatus = 1;
		((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCPlayedNoStinger = 0;
	}

	_staticData.cycleStartFrame = 0;
	_staticData.cycleFrameCount = 20;
	_staticData.navFrameIndex = 37;
	viewWindow->invalidateWindow(false);
	return SC_TRUE;
}

int ArthurScanningRoomConversation::mouseUp(Window *viewWindow, const Common::Point &pointLocation) {
	if (_yes.contains(pointLocation)) {
		switch (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCConversationStatus) {
		case 1: // Proceed with scan, then ask about downloading into biochips
			((SceneViewWindow *)viewWindow)->playSynchronousAnimation(8);
			_staticData.navFrameIndex = 36;
			((SceneViewWindow *)viewWindow)->playSynchronousAnimation(10);
			_staticData.navFrameIndex = 37;
			((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCConversationStatus = 2;
			((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCPlayedNoStinger = 0;
			viewWindow->invalidateWindow(false); // Original doesn't do this, but I can't see how it works otherwise
			return SC_TRUE;
		case 2: { // Proceed with downloading
			((SceneViewWindow *)viewWindow)->playSynchronousAnimation(11);
			_staticData.navFrameIndex = 36;
			((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCConversationStatus = 3;

			// Move the player back and play the instructions for the door
			DestinationScene destData;
			destData.destinationScene = Location(6, 4, 1, 2, 1, 0);
			destData.transitionType = TRANSITION_VIDEO;
			destData.transitionData = 0;
			destData.transitionStartFrame = -1;
			destData.transitionLength = -1;
			((SceneViewWindow *)viewWindow)->moveToDestination(destData);
			return SC_TRUE;
		}
		}
	} else if (_no.contains(pointLocation)) {
		switch (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCConversationStatus) {
		case 1: { // No-go on the scan, so drop the player back and play the rejection sound file
			if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCPlayedNoStinger == 0) {
				_vm->_sound->playSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, 9), 128, false, true);
				((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCPlayedNoStinger = 1;
			}

			DestinationScene destData;
			destData.destinationScene = Location(6, 4, 1, 2, 1, 0);
			destData.transitionType = TRANSITION_VIDEO;
			destData.transitionData = 0;
			destData.transitionStartFrame = -1;
			destData.transitionLength = -1;
			((SceneViewWindow *)viewWindow)->moveToDestination(destData);
			return SC_TRUE;
		}
		case 2: { // No-go on the download, so drop the player back and play the rejection sound file
			if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCPlayedNoStinger == 0) {
				_vm->_sound->playSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, 10), 128, false, true);
				((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCPlayedNoStinger = 1;
			}

			DestinationScene destData;
			destData.destinationScene = Location(6, 4, 1, 2, 1, 0);
			destData.transitionType = TRANSITION_VIDEO;
			destData.transitionData = 0;
			destData.transitionStartFrame = -1;
			destData.transitionLength = -1;
			((SceneViewWindow *)viewWindow)->moveToDestination(destData);
			return SC_TRUE;
		}
		}
	}

	return SC_FALSE;
}

int ArthurScanningRoomConversation::specifyCursor(Window *viewWindow, const Common::Point &pointLocation) {
	if (_yes.contains(pointLocation) || _no.contains(pointLocation))
		return kCursorFinger;

	return kCursorArrow;
}

class ScanningRoomNexusDoorNormalFacing : public SceneBase {
public:
	ScanningRoomNexusDoorNormalFacing(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation);
	int postEnterRoom(Window *viewWindow, const Location &priorLocation);
	int mouseUp(Window *viewWindow, const Common::Point &pointLocation);
	int specifyCursor(Window *viewWindow, const Common::Point &pointLocation);

private:
	Common::Rect _clickable;
};

ScanningRoomNexusDoorNormalFacing::ScanningRoomNexusDoorNormalFacing(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation) :
		SceneBase(vm, viewWindow, sceneStaticData, priorLocation) {
	_clickable = Common::Rect(162, 67, 284, 189);
}

int ScanningRoomNexusDoorNormalFacing::postEnterRoom(Window *viewWindow, const Location &priorLocation) {
	if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCHeardNexusDoorComment == 0 && ((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCConversationStatus == 3) {
		_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, 8));
		((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCHeardNexusDoorComment = 1;
	}

	return SC_TRUE;
}

int ScanningRoomNexusDoorNormalFacing::mouseUp(Window *viewWindow, const Common::Point &pointLocation) {
	if (_clickable.contains(pointLocation)) {
		// Change the still frame
		int oldFrame = _staticData.navFrameIndex;
		_staticData.navFrameIndex = 43;
		viewWindow->invalidateWindow(false);

		// Play the beep and voiceovers
		_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, 1, 12));
		_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, 1, 13));

		// Reset the frame
		_staticData.navFrameIndex = oldFrame;
		viewWindow->invalidateWindow(false);
		return SC_TRUE;
	}

	return SC_FALSE;
}

int ScanningRoomNexusDoorNormalFacing::specifyCursor(Window *viewWindow, const Common::Point &pointLocation) {
	if (_clickable.contains(pointLocation))
		return kCursorFinger;

	return kCursorArrow;
}

class ScanningRoomNexusDoorZoomInCodePad : public SceneBase {
public:
	ScanningRoomNexusDoorZoomInCodePad(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation);
	int mouseUp(Window *viewWindow, const Common::Point &pointLocation);
	int specifyCursor(Window *viewWindow, const Common::Point &pointLocation);

private:
	Common::Rect _controls;
};

ScanningRoomNexusDoorZoomInCodePad::ScanningRoomNexusDoorZoomInCodePad(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation) :
		SceneBase(vm, viewWindow, sceneStaticData, priorLocation) {
	_controls = Common::Rect(160, 50, 282, 140);
}

int ScanningRoomNexusDoorZoomInCodePad::mouseUp(Window *viewWindow, const Common::Point &pointLocation) {
	if (_controls.contains(pointLocation)) {
		DestinationScene destinationData;
		destinationData.destinationScene = _staticData.location;
		destinationData.destinationScene.depth = 1;
		destinationData.transitionType = TRANSITION_VIDEO;
		destinationData.transitionData = 1;
		destinationData.transitionStartFrame = -1;
		destinationData.transitionLength = -1;
		((SceneViewWindow *)viewWindow)->moveToDestination(destinationData);
		return SC_TRUE;
	}

	return SC_FALSE;
}

int ScanningRoomNexusDoorZoomInCodePad::specifyCursor(Window *viewWindow, const Common::Point &pointLocation) {
	if (_controls.contains(pointLocation))
		return kCursorMagnifyingGlass;

	return kCursorArrow;
}

class ScanningRoomNexusDoorCodePad : public SceneBase {
public:
	ScanningRoomNexusDoorCodePad(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation);
	~ScanningRoomNexusDoorCodePad();
	void preDestructor();
	int postEnterRoom(Window *viewWindow, const Location &priorLocation);
	int mouseUp(Window *viewWindow, const Common::Point &pointLocation);
	int onCharacter(Window *viewWindow, const Common::KeyState &character);
	int gdiPaint(Window *viewWindow);
	int specifyCursor(Window *viewWindow, const Common::Point &pointLocation);

private:
	Common::Rect _numbers[10];
	Common::String _entries;
	Graphics::Font *_textFont;
	int _lineHeight;
	Common::Rect _display;
};

ScanningRoomNexusDoorCodePad::ScanningRoomNexusDoorCodePad(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation) :
		SceneBase(vm, viewWindow, sceneStaticData, priorLocation) {
	_numbers[0] = Common::Rect(200, 129, 229, 146);
	_numbers[1] = Common::Rect(165, 63, 194, 80);
	_numbers[2] = Common::Rect(200, 63, 229, 80);
	_numbers[3] = Common::Rect(235, 63, 264, 80);
	_numbers[4] = Common::Rect(165, 85, 194, 102);
	_numbers[5] = Common::Rect(200, 85, 229, 102);
	_numbers[6] = Common::Rect(235, 85, 264, 102);
	_numbers[7] = Common::Rect(165, 107, 194, 124);
	_numbers[8] = Common::Rect(200, 107, 229, 124);
	_numbers[9] = Common::Rect(235, 107, 264, 124);
	_display = Common::Rect(166, 40, 262, 58);
	_lineHeight = _vm->getLanguage() == Common::JA_JPN ? 12 : 14;
	_textFont = _vm->_gfx->createFont(_lineHeight);
}

ScanningRoomNexusDoorCodePad::~ScanningRoomNexusDoorCodePad() {
	preDestructor();
}

void ScanningRoomNexusDoorCodePad::preDestructor() {
	delete _textFont;
	_textFont = 0;
}

int ScanningRoomNexusDoorCodePad::postEnterRoom(Window *viewWindow, const Location &priorLocation) {
	// Play Arthur's comment, if applicable
	if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCHeardNexusDoorCode == 0 && ((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCConversationStatus == 3) {
		_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, 7));
		((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCHeardNexusDoorCode = 1;
	}

	return SC_TRUE;
}

int ScanningRoomNexusDoorCodePad::mouseUp(Window *viewWindow, const Common::Point &pointLocation) {
	for (int i = 0; i < 10; i++) {
		if (_numbers[i].contains(pointLocation)) {
			if (_entries.size() < 5) {
				// Append
				_entries += (char)('0' + i);
				viewWindow->invalidateWindow(false);

				if (_entries == "32770") {
					// If the answer is correct, move to the depth with the open hatch movie
					DestinationScene destinationData;
					destinationData.destinationScene = _staticData.location;
					destinationData.destinationScene.depth = 2;
					destinationData.transitionType = TRANSITION_VIDEO;
					destinationData.transitionData = 3;
					destinationData.transitionStartFrame = -1;
					destinationData.transitionLength = -1;
					((SceneViewWindow *)viewWindow)->moveToDestination(destinationData);
				}

				return SC_TRUE;
			} else {
				// Reset
				_entries = (char)('0' + i);
				viewWindow->invalidateWindow(false);
				return SC_TRUE;
			}
		}
	}

	DestinationScene destinationData;
	destinationData.destinationScene = _staticData.location;
	destinationData.destinationScene.depth = 0;
	destinationData.transitionType = TRANSITION_VIDEO;
	destinationData.transitionData = 2;
	destinationData.transitionStartFrame = -1;
	destinationData.transitionLength = -1;
	((SceneViewWindow *)viewWindow)->moveToDestination(destinationData);
	return SC_TRUE;
}

int ScanningRoomNexusDoorCodePad::onCharacter(Window *viewWindow, const Common::KeyState &character) {
	if (character.keycode >= Common::KEYCODE_0 && character.keycode <= Common::KEYCODE_9) {
		char c = (char)('0' + character.keycode - Common::KEYCODE_0);

		if (_entries.size() < 5) {
			// Append
			_entries += c;
			viewWindow->invalidateWindow(false);

			if (_entries == "32770") {
				// If the answer is correct, move to the depth with the open hatch movie
				DestinationScene destinationData;
				destinationData.destinationScene = _staticData.location;
				destinationData.destinationScene.depth = 2;
				destinationData.transitionType = TRANSITION_VIDEO;
				destinationData.transitionData = 3;
				destinationData.transitionStartFrame = -1;
				destinationData.transitionLength = -1;
				((SceneViewWindow *)viewWindow)->moveToDestination(destinationData);
			}

			return SC_TRUE;
		} else {
			// Reset
			_entries = c;
			viewWindow->invalidateWindow(false);
			return SC_TRUE;
		}
	}

	if ((character.keycode == Common::KEYCODE_BACKSPACE || character.keycode == Common::KEYCODE_DELETE) && !_entries.empty()) {
		// Delete last character
		_entries.deleteLastChar();
		viewWindow->invalidateWindow(false);
		return SC_TRUE;
	}

	return SC_FALSE;
}

int ScanningRoomNexusDoorCodePad::gdiPaint(Window *viewWindow) {
	if (!_entries.empty()) {
		uint32 textColor = _vm->_gfx->getColor(208, 144, 24);
		Common::Rect absoluteRect = viewWindow->getAbsoluteRect();
		Common::Rect rect(_display);
		rect.translate(absoluteRect.left, absoluteRect.top);
		_vm->_gfx->renderText(_vm->_gfx->getScreen(), _textFont, _entries, rect.left, rect.top, rect.width(), rect.height(), textColor, _lineHeight, kTextAlignLeft, true);
	}

	return SC_REPAINT;
}

int ScanningRoomNexusDoorCodePad::specifyCursor(Window *viewWindow, const Common::Point &pointLocation) {
	for (int i = 0; i < 10; i++)
		if (_numbers[i].contains(pointLocation))
			return kCursorFinger;

	return kCursorPutDown;
}

class ScanningRoomNexusDoorPullHandle : public SceneBase {
public:
	ScanningRoomNexusDoorPullHandle(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation);
	int postEnterRoom(Window *viewWindow, const Location &priorLocation);
	int mouseUp(Window *viewWindow, const Common::Point &pointLocation);
	int specifyCursor(Window *viewWindow, const Common::Point &pointLocation);

private:
	Common::Rect _handle;
};

ScanningRoomNexusDoorPullHandle::ScanningRoomNexusDoorPullHandle(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation) :
		SceneBase(vm, viewWindow, sceneStaticData, priorLocation) {
	_handle = Common::Rect(186, 44, 276, 154);
}

int ScanningRoomNexusDoorPullHandle::postEnterRoom(Window *viewWindow, const Location &priorLocation) {
	// Play Arthur's comment, if applicable
	if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCHeardNexusDoorCode == 0 && ((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCConversationStatus == 3) {
		_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, 7));
		((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCHeardNexusDoorCode = 1;
	}

	return SC_TRUE;
}

int ScanningRoomNexusDoorPullHandle::mouseUp(Window *viewWindow, const Common::Point &pointLocation) {
	if (_handle.contains(pointLocation)) {
		DestinationScene destinationData;
		destinationData.destinationScene = Location(6, 5, 0, 0, 1, 0);
		destinationData.transitionType = TRANSITION_VIDEO;
		destinationData.transitionData = 4;
		destinationData.transitionStartFrame = -1;
		destinationData.transitionLength = -1;
		((SceneViewWindow *)viewWindow)->moveToDestination(destinationData);
		return SC_TRUE;
	}

	return SC_FALSE;
}

int ScanningRoomNexusDoorPullHandle::specifyCursor(Window *viewWindow, const Common::Point &pointLocation) {
	if (_handle.contains(pointLocation))
		return kCursorFinger;

	return kCursorArrow;
}

class SpaceDoor : public SceneBase {
public:
	SpaceDoor(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation,
			int left = -1, int top = -1, int right = -1, int bottom = -1, int openFrame = -1, int closedFrame = -1, int depth = -1,
			int transitionType = -1, int transitionData = -1, int transitionStartFrame = -1, int transitionLength = -1,
			int doorFlag = -1, int doorFlagValue = 0);
	int mouseDown(Window *viewWindow, const Common::Point &pointLocation);
	int mouseUp(Window *viewWindow, const Common::Point &pointLocation);
	int specifyCursor(Window *viewWindow, const Common::Point &pointLocation);

private:
	bool _clicked;
	Common::Rect _clickable;
	DestinationScene _destData;
	int _openFrame;
	int _closedFrame;
	int _doorFlag;
	int _doorFlagValue;
};

SpaceDoor::SpaceDoor(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation,
		int left, int top, int right, int bottom, int openFrame, int closedFrame, int depth,
		int transitionType, int transitionData, int transitionStartFrame, int transitionLength,
		int doorFlag, int doorFlagValue) :
		SceneBase(vm, viewWindow, sceneStaticData, priorLocation) {
	_clicked = false;
	_openFrame = openFrame;
	_closedFrame = closedFrame;
	_doorFlag = doorFlag;
	_doorFlagValue = doorFlagValue;
	_clickable = Common::Rect(left, top, right, bottom);
	_destData.destinationScene = _staticData.location;
	_destData.destinationScene.depth = depth;
	_destData.transitionType = transitionType;
	_destData.transitionData = transitionData;
	_destData.transitionStartFrame = transitionStartFrame;
	_destData.transitionLength = transitionLength;
}

int SpaceDoor::mouseDown(Window *viewWindow, const Common::Point &pointLocation) {
	if (_clickable.contains(pointLocation)) {
		_clicked = true;
		return SC_TRUE;
	}

	return SC_FALSE;
}

int SpaceDoor::mouseUp(Window *viewWindow, const Common::Point &pointLocation) {
	if (_clicked) {
		// If we are facing the scanning room door and we have Arthur, automatically recall
		// to the future apartment
		if (_staticData.location.timeZone == 6 && _staticData.location.environment == 3 &&
				_staticData.location.node == 9 && _staticData.location.facing == 0 &&
				_staticData.location.orientation == 0 && _staticData.location.depth == 0 &&
				((GameUIWindow *)viewWindow->getParent())->_inventoryWindow->isItemInInventory(kItemBioChipAI)) {
			((SceneViewWindow *)viewWindow)->timeSuitJump(4);
			return SC_TRUE;
		}

		if (_doorFlag < 0 || ((SceneViewWindow *)viewWindow)->getGlobalFlagByte(_doorFlag) == _doorFlagValue) {
			// Change the still frame to the new one
			if (_openFrame >= 0) {
				_staticData.navFrameIndex = _openFrame;
				viewWindow->invalidateWindow(false);
				_vm->_sound->playSynchronousSoundEffect("BITDATA/AILAB/AI_LOCK.BTA");
			}

			((SceneViewWindow *)viewWindow)->moveToDestination(_destData);
		} else {
			// Display the closed frame
			if (_closedFrame >= 0) {
				_staticData.navFrameIndex = _closedFrame;
				viewWindow->invalidateWindow(false);
			}
		}

		_clicked = false;
		return SC_TRUE;
	}

	return SC_FALSE;
}

int SpaceDoor::specifyCursor(Window *viewWindow, const Common::Point &pointLocation) {
	if (_clickable.contains(pointLocation))
		return kCursorFinger;

	return kCursorArrow;
}

class ScanningRoomNexusDoorToGlobe : public SceneBase {
public:
	ScanningRoomNexusDoorToGlobe(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation);
};

ScanningRoomNexusDoorToGlobe::ScanningRoomNexusDoorToGlobe(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation) :
		SceneBase(vm, viewWindow, sceneStaticData, priorLocation) {
	if (((SceneViewWindow *)viewWindow)->getGlobalFlags().aiSCConversationStatus == 3)
		_staticData.destForward.destinationScene = Location(-1, -1, -1, -1, -1, -1);
}

class DockingBayPlaySoundEntering : public SceneBase {
public:
	DockingBayPlaySoundEntering(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation,
			int soundFileNameID = -1, int flagOffset = -1);
	int postEnterRoom(Window *viewWindow, const Location &priorLocation);

private:
	int _soundFileNameID;
	int _flagOffset;
};

DockingBayPlaySoundEntering::DockingBayPlaySoundEntering(BuriedEngine *vm, Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation,
		int soundFileNameID, int flagOffset) :
		SceneBase(vm, viewWindow, sceneStaticData, priorLocation) {
	_soundFileNameID = soundFileNameID;
	_flagOffset = flagOffset;

	if (((SceneViewWindow *)viewWindow)->getGlobalFlags().generalWalkthroughMode == 1)
		_staticData.destForward.destinationScene = Location(-1, -1, -1, -1, -1, -1);
}

int DockingBayPlaySoundEntering::postEnterRoom(Window *viewWindow, const Location &priorLocation) {
	if (_flagOffset >= 0 && ((SceneViewWindow *)viewWindow)->getGlobalFlagByte(_flagOffset) == 0) {
		_vm->_sound->playSynchronousSoundEffect(_vm->getFilePath(_staticData.location.timeZone, _staticData.location.environment, _soundFileNameID));
		((SceneViewWindow *)viewWindow)->setGlobalFlagByte(_flagOffset, 1);
	}

	return SC_TRUE;
}

bool SceneViewWindow::initializeAILabTimeZoneAndEnvironment(Window *viewWindow, int environment) {
	if (environment == -1) {
		GlobalFlags &flags = ((SceneViewWindow *)viewWindow)->getGlobalFlags();

		flags.aiHWStingerID = 0;
		flags.aiHWStingerChannelID = 0;
		flags.aiCRStingerID = 0;
		flags.aiCRStingerChannelID = 0;
		flags.aiDBStingerID = 0;
		flags.aiDBStingerChannelID = 0;
		flags.aiOxygenTimer = kAIHWStartingValue;
		flags.aiCRPressurized = flags.generalWalkthroughMode;
		flags.aiCRPressurizedAttempted = 0;
		flags.aiMRPressurized = flags.generalWalkthroughMode;
		flags.aiIceMined = 0;
		flags.aiOxygenReserves = 1;
		flags.aiSCHeardInitialSpeech = 0;
		flags.aiMRCorrectFreqSet = 4;
		flags.aiSCConversationStatus = 0;
		flags.aiSCHeardNexusDoorComment = 0;
		flags.aiSCHeardNexusDoorCode = 0;
		flags.aiNXPlayedBrainComment = 0;
		flags.aiDBPlayedSecondArthur = 0;
		flags.aiDBPlayedThirdArthur = 0;
		flags.aiDBPlayedFourthArthur = 0;
		flags.aiCRGrabbedMetalBar = ((GameUIWindow *)viewWindow->getParent())->_inventoryWindow->isItemInInventory(kItemMetalBar) ? 1 : 0;
		flags.aiICGrabbedWaterCanister = (((GameUIWindow *)viewWindow->getParent())->_inventoryWindow->isItemInInventory(kItemWaterCanEmpty) || ((GameUIWindow *)viewWindow->getParent())->_inventoryWindow->isItemInInventory(kItemWaterCanFull)) ? 1 : 0;
	} else if (environment == 1) {
		((SceneViewWindow *)viewWindow)->getGlobalFlags().scoreEnteredSpaceStation = 1;
	}

	return true;
}

bool SceneViewWindow::startAILabAmbient(int oldTimeZone, int oldEnvironment, int environment, bool fade) {
	_vm->_sound->setAmbientSound(_vm->getFilePath(6, environment, SF_AMBIENT), fade, 64);
	return true;
}

SceneBase *SceneViewWindow::constructAILabSceneObject(Window *viewWindow, const LocationStaticData &sceneStaticData, const Location &priorLocation) {
	// TODO

	switch (sceneStaticData.classID) {
	case 1:
		return new UseCheeseGirlPropellant(_vm, viewWindow, sceneStaticData, priorLocation);
	case 3:
		return new SpaceDoorTimer(_vm, viewWindow, sceneStaticData, priorLocation, 172, 46, 262, 136, 87, -1, 1, TRANSITION_VIDEO, 2, -1, -1, -1, -1);
	case 4:
		return new PlayArthurOffsetTimed(_vm, viewWindow, sceneStaticData, priorLocation, 127, offsetof(GlobalFlags, aiHWStingerID), offsetof(GlobalFlags, aiHWStingerChannelID), 4, 10, 1); // 1.01 uses a delay of 2, clone2727 likes that better
	case 5:
		return new SpaceDoorTimer(_vm, viewWindow, sceneStaticData, priorLocation, 144, 30, 268, 152, 88, -1, 1, TRANSITION_VIDEO, 4, -1, -1, -1, -1);
	case 6:
		return new PlaySoundExitingFromScene(_vm, viewWindow, sceneStaticData, priorLocation, 14);
	case 7:
		return new HabitatWingLockedDoor(_vm, viewWindow, sceneStaticData, priorLocation, 99, 12, 13, 166, 32, 286, 182);
	case 8:
		return new HabitatWingLockedDoor(_vm, viewWindow, sceneStaticData, priorLocation, 100, 12, 13, 130, 48, 290, 189);
	case 9:
		return new HabitatWingIceteroidDoor(_vm, viewWindow, sceneStaticData, priorLocation);
	case 11:
		return new BaseOxygenTimer(_vm, viewWindow, sceneStaticData, priorLocation);
	case 12:
		return new BaseOxygenTimerInSpace(_vm, viewWindow, sceneStaticData, priorLocation);
	case 20:
		return new PlayArthurOffsetCapacitance(_vm, viewWindow, sceneStaticData, priorLocation, 127, offsetof(GlobalFlags, aiCRStingerID), offsetof(GlobalFlags, aiCRStingerChannelID), 4, 11, 1);
	case 21:
		return new CapacitanceToHabitatDoorClosed(_vm, viewWindow, sceneStaticData, priorLocation);
	case 22:
		return new CapacitanceToHabitatDoorOpen(_vm, viewWindow, sceneStaticData, priorLocation);
	case 23:
		return new ClickChangeSceneCapacitance(_vm, viewWindow, sceneStaticData, priorLocation, 122, 32, 310, 140, kCursorMagnifyingGlass, 6, 2, 3, 0, 1, 1, TRANSITION_VIDEO, 3, -1, -1);
	case 24:
		return new CapacitancePanelInterface(_vm, viewWindow, sceneStaticData, priorLocation);
	case 25:
		return new CapacitanceDockingBayDoor(_vm, viewWindow, sceneStaticData, priorLocation);
	case 26:
		return new PlaySoundExitingFromScene(_vm, viewWindow, sceneStaticData, priorLocation, 14);
	case 27:
		return new PlayArthurOffsetCapacitance(_vm, viewWindow, sceneStaticData, priorLocation, 127, offsetof(GlobalFlags, aiCRStingerID), offsetof(GlobalFlags, aiCRStingerChannelID), 4, 11, 1, offsetof(GlobalFlags, aiCRGrabbedMetalBar), 73, 320, 40);
	case 28:
		return new PlayArthurOffsetCapacitance(_vm, viewWindow, sceneStaticData, priorLocation, 127, offsetof(GlobalFlags, aiCRStingerID), offsetof(GlobalFlags, aiCRStingerChannelID), 4, 11, 1, offsetof(GlobalFlags, aiCRGrabbedMetalBar), 66, 241, 25);
	case 30:
		return new PlaySoundEnteringScene(_vm, viewWindow, sceneStaticData, priorLocation, 5, offsetof(GlobalFlags, aiDBPlayedFirstArthur));
	case 31:
		return new SpaceDoor(_vm, viewWindow, sceneStaticData, priorLocation, 174, 70, 256, 152, 166, -1, 1, TRANSITION_VIDEO, 0, -1, -1, -1, 0);
	case 32:
		return new PlaySoundExitingFromScene(_vm, viewWindow, sceneStaticData, priorLocation, 14);
	case 33:
		return new SpaceDoor(_vm, viewWindow, sceneStaticData, priorLocation, 185, 42, 253, 110, 167, -1, 1, TRANSITION_VIDEO, 1, -1, -1, -1, 0);
	case 35:
		return new DockingBayPlaySoundEntering(_vm, viewWindow, sceneStaticData, priorLocation, 4, offsetof(GlobalFlags, aiDBPlayedMomComment));
	case 36:
		return new PlaySoundEnteringScene(_vm, viewWindow, sceneStaticData, priorLocation, 6, offsetof(GlobalFlags, aiDBPlayedSecondArthur));
	case 37:
		return new PlaySoundEnteringScene(_vm, viewWindow, sceneStaticData, priorLocation, 7, offsetof(GlobalFlags, aiDBPlayedThirdArthur));
	case 38:
		return new PlaySoundEnteringScene(_vm, viewWindow, sceneStaticData, priorLocation, 8, offsetof(GlobalFlags, aiDBPlayedFourthArthur));
	case 39:
		return new DisableForwardMovement(_vm, viewWindow, sceneStaticData, priorLocation, offsetof(GlobalFlags, generalWalkthroughMode), 1);
	case 40:
		return new ScanningRoomEntryScan(_vm, viewWindow, sceneStaticData, priorLocation);
	case 41:
		return new ScanningRoomWalkWarning(_vm, viewWindow, sceneStaticData, priorLocation);
	case 42:
		return new ScanningRoomDockingBayDoor(_vm, viewWindow, sceneStaticData, priorLocation);
	case 43:
		return new ScanningRoomScienceWingDoor(_vm, viewWindow, sceneStaticData, priorLocation);
	case 44:
		return new ArthurScanningRoomConversation(_vm, viewWindow, sceneStaticData, priorLocation);
	case 45:
		return new ScanningRoomNexusDoorNormalFacing(_vm, viewWindow, sceneStaticData, priorLocation);
	case 46:
		return new ScanningRoomNexusDoorZoomInCodePad(_vm, viewWindow, sceneStaticData, priorLocation);
	case 47:
		return new ScanningRoomNexusDoorCodePad(_vm, viewWindow, sceneStaticData, priorLocation);
	case 48:
		return new ScanningRoomNexusDoorPullHandle(_vm, viewWindow, sceneStaticData, priorLocation);
	case 49:
		return new ScanningRoomNexusDoorToGlobe(_vm, viewWindow, sceneStaticData, priorLocation);
	case 50:
		return new IceteroidPodTimed(_vm, viewWindow, sceneStaticData, priorLocation, 174, 96, 246, 118, 1, 6, 6, 1, 0, 1, 0);
	case 51:
		return new IceteroidPodTimed(_vm, viewWindow, sceneStaticData, priorLocation, 174, 96, 246, 118, 3, 6, 6, 0, 0, 1, 0);
	case 52:
		return new SpaceDoorTimer(_vm, viewWindow, sceneStaticData, priorLocation, 164, 40, 276, 140, -1, -1, 1, TRANSITION_VIDEO, 0, -1, -1, -1, -1);
	case 53:
		return new SpaceDoorTimer(_vm, viewWindow, sceneStaticData, priorLocation, 164, 40, 276, 140, -1, -1, 1, TRANSITION_VIDEO, 2, -1, -1, -1, -1);
	case 54:
		return new PlaySoundExitingFromSceneDeux(_vm, viewWindow, sceneStaticData, priorLocation, 14);
	case 55:
		return new IceteroidElevatorExtremeControls(_vm, viewWindow, sceneStaticData, priorLocation, 6, 6, 6, 0, 1, 0, 6);
	case 56:
		return new IceteroidElevatorExtremeControls(_vm, viewWindow, sceneStaticData, priorLocation, 6, 6, 3, 0, 1, 0, 5, 6, 6, 2, 0, 1, 0, 7);
	case 57:
		return new IceteroidElevatorExtremeControls(_vm, viewWindow, sceneStaticData, priorLocation, -1, -1, -1, -1, -1, -1, -1, 6, 6, 6, 0, 1, 0, 4);
	case 60:
		return new BaseOxygenTimer(_vm, viewWindow, sceneStaticData, priorLocation);
	case 63:
		return new IceteroidPodTimed(_vm, viewWindow, sceneStaticData, priorLocation, 174, 96, 246, 118, 14, 6, 6, 5, 0, 1, 0);
	case 64:
		return new IceteroidPodTimed(_vm, viewWindow, sceneStaticData, priorLocation, 174, 96, 246, 118, 15, 6, 6, 4, 0, 1, 0);
	case 65:
		return new SpaceDoorTimer(_vm, viewWindow, sceneStaticData, priorLocation, 164, 26, 268, 124, -1, -1, 1, TRANSITION_VIDEO, 13, -1, -1, -1, -1);
	case 66:
		return new SpaceDoorTimer(_vm, viewWindow, sceneStaticData, priorLocation, 164, 26, 268, 124, -1, -1, 1, TRANSITION_VIDEO, 16, -1, -1, -1, -1);
	case 68:
		return new PlaySoundExitingFromSceneDeux(_vm, viewWindow, sceneStaticData, priorLocation, 14);
	case 70:
		return new SpaceDoorTimer(_vm, viewWindow, sceneStaticData, priorLocation, 92, 92, 212, 189, 48, -1, 1, TRANSITION_VIDEO, 0, -1, -1, -1, -1);
	case 75:
		return new HabitatWingLockedDoor(_vm, viewWindow, sceneStaticData, priorLocation, 51, 4, 5, 146, 0, 396, 84);
	case 90:
		return new NexusDoor(_vm, viewWindow, sceneStaticData, priorLocation);
	case 91:
		return new NexusPuzzle(_vm, viewWindow, sceneStaticData, priorLocation);
	case 92:
		return new NexusEnd(_vm, viewWindow, sceneStaticData, priorLocation);
	case 93:
		return new BaseOxygenTimer(_vm, viewWindow, sceneStaticData, priorLocation);
	}

	warning("TODO: AI lab scene object %d", sceneStaticData.classID);
	return new SceneBase(_vm, viewWindow, sceneStaticData, priorLocation);
}

} // End of namespace Buried
