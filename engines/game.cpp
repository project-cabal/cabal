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

#include "engines/game.h"
#include "common/gui_options.h"


const PlainGameDescriptor *findPlainGameDescriptor(const char *gameid, const PlainGameDescriptor *list) {
	const PlainGameDescriptor *g = list;
	while (g->gameid) {
		if (0 == scumm_stricmp(gameid, g->gameid))
			return g;
		g++;
	}
	return 0;
}

GameDescriptor::GameDescriptor() {
}

GameDescriptor::GameDescriptor(const Common::String &engineID, const PlainGameDescriptor &pgd, Common::String guioptions) {
	_engineID = engineID;
	_gameID = pgd.gameid;
	_description = pgd.description;

	if (!guioptions.empty())
		_guiOptions = Common::getGameGUIOptionsDescription(guioptions);
}

GameDescriptor::GameDescriptor(const Common::String &engineID, const Common::String &g, const Common::String &d, Common::Language l, Common::Platform p, Common::String guioptions, GameSupportLevel gsl) {
	_engineID = engineID;
	_gameID = g;
	_description = d;

	if (l != Common::UNK_LANG)
		_language = Common::getLanguageCode(l);
	if (p != Common::kPlatformUnknown)
		_platform = Common::getPlatformCode(p);
	if (!guioptions.empty())
		_guiOptions = Common::getGameGUIOptionsDescription(guioptions);

	setSupportLevel(gsl);
}

void GameDescriptor::setGUIOptions(const Common::String &guioptions) {
	if (guioptions.empty())
		_guiOptions.clear();
	else
		_guiOptions = Common::getGameGUIOptionsDescription(guioptions);
}

void GameDescriptor::appendGUIOptions(const Common::String &str) {
	if (!_guiOptions.empty())
		_guiOptions += " ";

	_guiOptions += str;
}

void GameDescriptor::updateDesc(const char *extra) {
	const bool hasCustomLanguage = (getLanguage() != Common::UNK_LANG);
	const bool hasCustomPlatform = (getPlatform() != Common::kPlatformUnknown);
	const bool hasExtraDesc = (extra && extra[0]);

	// Adapt the description string if custom platform/language is set.
	if (hasCustomLanguage || hasCustomPlatform || hasExtraDesc) {
		Common::String descr = getDescription();

		descr += " (";
		if (hasExtraDesc)
			descr += extra;
		if (hasCustomPlatform) {
			if (hasExtraDesc)
				descr += "/";
			descr += Common::getPlatformDescription(getPlatform());
		}
		if (hasCustomLanguage) {
			if (hasExtraDesc || hasCustomPlatform)
				descr += "/";
			descr += Common::getLanguageDescription(getLanguage());
		}
		descr += ")";
		_description = descr;
	}
}

GameSupportLevel GameDescriptor::getSupportLevel() const {
	if (_gameSupportLevel == "unstable")
		return kUnstableGame;

	return kStableGame;
}

void GameDescriptor::setSupportLevel(GameSupportLevel gsl) {
	switch (gsl) {
	case kUnstableGame:
		_gameSupportLevel = "unstable";
		break;
	case kStableGame:
		// Fall Through intended
	default:
		_gameSupportLevel.clear();
		break;
	}
}

GameList convertPlainGameDescriptors(const Common::String &engineID, const PlainGameDescriptor *gameList) {
	GameList output;
	while (gameList->gameid) {
		output.push_back(GameDescriptor(engineID, *gameList));
		gameList++;
	}

	return output;
}
