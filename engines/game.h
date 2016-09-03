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

#ifndef ENGINES_GAME_H
#define ENGINES_GAME_H

#include "common/array.h"
#include "common/hash-str.h"
#include "common/str.h"
#include "common/language.h"
#include "common/platform.h"

/**
 * A simple structure used to map gameids (like "monkey", "sword1", ...) to
 * nice human readable and descriptive game titles (like "The Secret of Monkey Island").
 * This is a plain struct to make it possible to declare NULL-terminated C arrays
 * consisting of PlainGameDescriptors.
 */
struct PlainGameDescriptor {
	const char *gameid;
	const char *description;
};

/**
 * Given a list of PlainGameDescriptors, returns the first PlainGameDescriptor
 * matching the given gameid. If not match is found return 0.
 * The end of the list must be marked by an entry with gameid 0.
 */
const PlainGameDescriptor *findPlainGameDescriptor(const char *gameid, const PlainGameDescriptor *list);

/**
 * Ths is an enum to describe how done a game is. This also indicates what level of support is expected.
 */
enum GameSupportLevel {
	kStableGame = 0, // the game is fully supported
	kUnstableGame // the game is not even ready for public testing yet
};

/**
 * A hashmap describing details about a given game. In a sense this is a refined
 * version of PlainGameDescriptor, as it also contains a gameid and a description string.
 * But in addition, platform and language settings, as well as arbitrary other settings,
 * can be contained in a GameDescriptor.
 * This is an essential part of the glue between the game engines and the launcher code.
 */
class GameDescriptor {
public:
	GameDescriptor();
	GameDescriptor(
		const Common::String &engineID,
		const PlainGameDescriptor &pgd,
		Common::String guioptions = Common::String());
	GameDescriptor(
		const Common::String &engineID,
		const Common::String &gameid,
		const Common::String &description,
		Common::Language language = Common::UNK_LANG,
		Common::Platform platform = Common::kPlatformUnknown,
		Common::String guioptions = Common::String(),
		GameSupportLevel gsl = kStableGame);

	/**
	 * Update the description string by appending (EXTRA/PLATFORM/LANG) to it.
	 * Values that are missing are omitted, so e.g. (EXTRA/LANG) would be
	 * added if no platform has been specified but a language and an extra string.
	 */
	void updateDesc(const char *extra = 0);

	void setGUIOptions(const Common::String &options);
	void appendGUIOptions(const Common::String &str);
	Common::String getGUIOptions() const { return _guiOptions; }

	/**
	 * What level of support is expected of this game
	 */
	GameSupportLevel getSupportLevel() const;
	void setSupportLevel(GameSupportLevel gsl);
	Common::String getSupportLevelString() const { return _gameSupportLevel; }

	void setEngineID(const Common::String &engineID) { _engineID = engineID; }
	Common::String getEngineID() const { return _engineID; }
	void setGameID(const Common::String &gameID) { _gameID = gameID; }
	Common::String getGameID() const { return _gameID; }
	void setPreferredTarget(const Common::String &target) { _preferredTarget = target; }
	Common::String getPreferredTarget() const {
		return _preferredTarget.empty() ? _gameID : _preferredTarget;
	}

	Common::String getDescription() const { return _description; }
	void setDescription(const Common::String &desc) { _description = desc; }

	Common::String getLanguageString() const { return _language; }
	void setLanguageString(const Common::String &lang) { _language = lang; }
	Common::Language getLanguage() const { return _language.empty() ? Common::UNK_LANG : Common::parseLanguage(_language); }

	Common::String getPlatformString() const { return _platform; }
	void setPlatformString(const Common::String &platform) { _platform = platform; }
	Common::Platform getPlatform() const { return _platform.empty() ? Common::kPlatformUnknown : Common::parsePlatform(_platform); }

	Common::String getPath() const { return _path; }
	void setPath(const Common::String &path) { _path = path; }

	Common::String getExtra() const { return _extra; }
	void setExtra(const Common::String &extra) { _extra = extra; }

private:
	Common::String _engineID;
	Common::String _gameID;
	Common::String _preferredTarget;
	Common::String _description;
	Common::String _language;
	Common::String _platform;
	Common::String _path;
	Common::String _guiOptions;
	Common::String _extra;
	Common::String _gameSupportLevel;
};

/** List of games. */
typedef Common::Array<GameDescriptor> GameList;

/// Construct a game list from an engine ID and a set of PlainGameDescriptors,
/// whose terminal entry has a NULL gameid.
GameList convertPlainGameDescriptors(const Common::String &engineID, const PlainGameDescriptor *gameList);

#endif
