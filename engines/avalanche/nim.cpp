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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

/*
 * This code is based on the original source code of Lord Avalot d'Argent version 1.3.
 * Copyright (c) 1994-1995 Mike, Mark and Thomas Thurman.
 */

#include "avalanche/avalanche.h"
#include "avalanche/nim.h"

namespace Avalanche {

const char * const Nim::kNames[2] = {"Avalot", "Dogfood"};

Nim::Nim(AvalancheEngine *vm) {
	_vm = vm;

	resetVariables();
}

void Nim::resetVariables() {
	_playedNim = 0;
	_turns = 0;
	_dogfoodsTurn = false;
	_stonesLeft = 0;
	_clicked = false;
	_row = 0;
	_number = 0;
	_squeak = false;
	_mNum = 0;
	_mRow = 0;
	_lmo = false;

	for (int i = 0; i < 3; i++) {
		_old[i] = 0;
		_stones[i] = 0;
		_inAp[i] = 0;
		_r[i] = 0;
	}
}

void Nim::synchronize(Common::Serializer &sz) {
	if (sz.isLoading() && sz.getVersion() < 2)
		return;

	sz.syncAsByte(_playedNim);
}

void Nim::playNim() {
	if (_vm->_wonNim) { // Already won the game.
		_vm->_dialogs->displayScrollChain('Q', 6);
		return;
	}

	if (!_vm->_askedDogfoodAboutNim) {
		_vm->_dialogs->displayScrollChain('Q', 84);
		return;
	}

	_vm->_dialogs->displayScrollChain('Q', 3);
	_playedNim++;
	
	_vm->_graphics->saveScreen();
	_vm->fadeOut();

	CursorMan.showMouse(false);
	setup();
	board();
	CursorMan.showMouse(true);

	do {
		startMove();
		if (_dogfoodsTurn)
			dogFood();
		else
			takeSome();
		_stones[_row] -= _number;
		showChanges();
	} while (_stonesLeft != 0);

	endOfGame(); // Winning sequence is A1, B3, B1, C1, C1, btw.

	_vm->fadeOut();
	CursorMan.showMouse(false);

	_vm->_graphics->restoreScreen();
	_vm->_graphics->removeBackup();

	CursorMan.showMouse(true);
	_vm->fadeIn();

	if (_dogfoodsTurn) {
		// Dogfood won - as usual.
		if (_playedNim == 1)   // Your first game.
			_vm->_dialogs->displayScrollChain('Q', 4); // Goody! Play me again?
		else
			_vm->_dialogs->displayScrollChain('Q', 5); // Oh, look at that! I've won again!
		_vm->decreaseMoney(4); // And you've just lost 4d!
	} else {
		// You won - strange!
		_vm->_dialogs->displayScrollChain('Q', 7);
		_vm->_objects[kObjectLute - 1] = true;
		_vm->refreshObjectList();
		_vm->_wonNim = true;
		_vm->_background->draw(-1, -1, 0); // Show the settle with no lute on it.
		
		// 7 points for winning!
		_vm->incScore(7);
	}

	if (_playedNim == 1) {
		// 3 points for playing your 1st game.
		_vm->incScore(3);
	}
}

void Nim::chalk(int x, int y, Common::String text) {
	const Color greys[] = { kColorBlack, kColorDarkgray, kColorLightgray, kColorWhite };
	
	for (int i = 0; i < 4; i++) {
		_vm->_graphics->drawNormalText(text, _vm->_font, 8, x - i, y, greys[i]);
		_vm->_graphics->refreshScreen();
		int freq = i * 100 * text.size();
		if (freq == 0)
			_vm->_system->delayMillis(3);
		else
			_vm->_sound->playNote(freq, 3);
		_vm->_system->delayMillis(30);
	}
}

void Nim::setup() {
	_vm->fadeIn();
	_vm->_graphics->loadNim();

	_vm->_graphics->drawFilledRectangle(Common::Rect(0, 0, 640, 200), kColorBlack);
	// Upper left rectangle.
	_vm->_graphics->drawRectangle(Common::Rect(10, 5, 381, 71), kColorRed);
	_vm->_graphics->drawFilledRectangle(Common::Rect(11, 6, 380, 70), kColorBrown);
	// Bottom right rectangle.
	_vm->_graphics->drawRectangle(Common::Rect(394, 50, 635, 198), kColorRed);
	_vm->_graphics->drawFilledRectangle(Common::Rect(395, 51, 634, 197), kColorBrown);
		
	_vm->_graphics->drawNimLogo();
	_vm->_graphics->drawNimInitials();

	_vm->_graphics->drawNormalText("SCOREBOARD:", _vm->_font, 8, 475, 45, kColorWhite);
	_vm->_graphics->drawNormalText("Turn:", _vm->_font, 8, 420, 55, kColorYellow);
	_vm->_graphics->drawNormalText("Player:", _vm->_font, 8, 490, 55, kColorYellow);
	_vm->_graphics->drawNormalText("Move:", _vm->_font, 8, 570, 55, kColorYellow);
	
	chalk(27, 7, "Take pieces away with:");
	chalk(77, 17, "1) the mouse (click leftmost)");
	chalk(53, 27, "or 2) the keyboard:");
	chalk(220, 27, Common::String(24) + '/' + 25 + ": choose row,");
	chalk(164, 37, Common::String("+/- or ") + 27 + '/' + 26 + ": more/fewer,");
	chalk(204, 47, "Enter: take stones.");

	_vm->_graphics->refreshScreen();

	for (int i = 0; i < 3; i++)
		_stones[i] = i + 3;
	_stonesLeft = 12;

	_turns = 0;
	_dogfoodsTurn = true;

	_row = 0;
	_number = 1;
	for (int i = 0; i < 3; i++)
		_old[i] = 0;
}

void Nim::board() {
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < _stones[i]; j++)
			_vm->_graphics->drawNimStone(64 + j * 7 * 8 + j * 8, 75 + i * 35);
			// It's practically the body of the Pascal function "plotstone()", reimplemented.
			// It's the only place where we use it, so there's no reason to keep it separated as a function.
	_vm->_graphics->refreshScreen();
}

void Nim::startMove() {
	_turns++;
	Common::String turnsStr = Common::String::format("%d", _turns);
	int y = 55 + _turns * 10;
	_dogfoodsTurn = !_dogfoodsTurn;
	chalk(433, y, turnsStr);
	chalk(493, y, kNames[_dogfoodsTurn]);
	for (int i = 0; i < 3; i++)
		_old[i] = _stones[i];
}

void Nim::showChanges() {
	warning("STUB: Nim::showChanges()");
}

void Nim::blip() {
	warning("STUB: Nim::blip()");
}

void Nim::checkMouse() {
	warning("STUB: Nim::checkMouse()");
}

void Nim::less() {
	warning("STUB: Nim::less()");
}

void Nim::takeSome() {
	warning("STUB: Nim::takeSome()");
}

void Nim::endOfGame() {
	warning("STUB: Nim::endOfGame()");
}

void Nim::dogFood() {
	_lmo = false;
	byte live = 0;
	byte sr[3];

	for (int i = 0; i < 3; i++) {
		if (_stones[i] > 0) {
			_r[live] = i;
			sr[live] = _stones[i];
			live++;
		}
	}
	
	switch (live) {
	case 1: // Only one is free - so take 'em all!
		_row = _r[0];
		_number = _stones[_r[0]];
		return;
	case 2: // Two are free - make them equal!
		if (sr[0] > sr[1]) { // T > b
			_row = _r[0];
			_number = sr[0] - sr[1];
		} else if (sr[0] < sr[1]) { // B > t
			_row = _r[1];
			_number = sr[1] - sr[0];
		} else { // B = t... oh no, we've lost!
			_row = _r[0];
			_number = 1;
		}
		return;
	case 3: { 
		// Ho hum... this'll be difficult!
		// There are three possible courses of action when we have 3 lines left:
		// 1) Look for 2 equal lines, then take the odd one out.
		// 2) Look for A.P.s, and capitalise on them.
		// 3) Go any old where.
		const byte other[3][2] = { { 2, 3 }, { 1, 3 }, { 1, 2 } };

		for (int i = 0; i < 3; i++) { // Look for 2 equal lines.
			if (_stones[other[i][0]] == _stones[other[i][1]]) {
				_row = i; // This row.
				_number = _stones[i]; // All of 'em.
				return;
			}
		}

		bool sorted;
		do {
			sorted = true;
			for (int i = 0; i < 2; i++) {
				if (sr[i] > sr[i + 1]) {
					byte temp = sr[i + 1];
					sr[i + 1] = sr[i];
					sr[i] = temp;

					temp = _r[i + 1];
					_r[i + 1] = _r[i];
					_r[i] = temp;

					sorted = false;
				}
			}
		} while (sorted);
		
		// Now we look for A.P.s...
		for (int i = 0; i < 3; i++) {
			findAp(i, 1); // There are 3 "1"s.
			if (_lmo)
				return; // Cut - out.
		}
		findAp(1, 2); // Only "2" possible.
		if (_lmo)
			return;

		// A.P.search must have failed - use the default move.
		_row = _r[2];
		_number = 1;
		return;
		}
	default:
		break;
	}
}

bool Nim::find(byte x) {
	bool ret = false;
	for (int i = 0; i < 3; i++) {
		if (_stones[i] == x) {
			ret = true;
			_inAp[i] = true;
		}
	}
	return ret;
}

void Nim::findAp(byte start, byte stepSize) {
	byte thisOne = 0;
	byte matches = 0;
	for (int i = 0; i < 3; i++)
		_inAp[i] = 0; // Blank 'em all!
	for (int i = 0; i < 3; i++) {
		if (find(start + i * stepSize))
			matches++;
		else
			thisOne = i;
	}

	// Now..Matches must be 0, 1, 2, or 3.
	// 0 / 1 mean there are no A.P.s here, so we'll keep looking,
	// 2 means there is a potential A.P.that we can create (ideal!), and
	// 3 means that we're already in an A.P. (Trouble!)

	byte ooo = 0; // Odd one out.

	switch (matches) {
	case 2:
		for (int i = 0; i < 3; i++) { // Find which one didn't fit the A.P.
			if (!_inAp[i])
				ooo = i;
		}

		if (_stones[ooo] > (start + thisOne * stepSize)) { // Check if it's possible!
			// Create an A.P.
			_row = ooo; // Already calculated.
			// Start + thisone * stepsize will give the amount we SHOULD have here.
			_number = _stones[_row] - (start + thisOne * stepSize);
			_lmo = true;
			return;
		}
		break;
	case 3:  // We're actually IN an A.P! Trouble! Oooh dear.
		// Take 1 from the largest pile.
		_row = _r[2];
		_number = 1;
		_lmo = true;
		return;
	default:
		break;
	}
}

} // End of namespace Avalanche