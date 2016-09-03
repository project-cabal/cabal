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

// Based on the ScummVM (GPLv2+) file of the same name

#ifndef PRINCE_DETECTION_H
#define PRINCE_DETECTION_H

#include "prince/prince.h"
#include "engines/advancedDetector.h"

namespace Prince {

enum PrinceGameType {
	kPrinceDataUNK,
	kPrinceDataDE,
	kPrinceDataPL
};

struct PrinceGameDescription {
	ADGameDescription desc;
	PrinceGameType gameType;
};

static const PlainGameDescriptor princeGames[] = {
	{"prince", "The Prince and the Coward"},
	{0, 0}
};

static const PrinceGameDescription gameDescriptions[] = {
	{
		{
			"prince",
			"",
			AD_ENTRY1s("databank.ptc", "5fa03833177331214ec1354761b1d2ee", 3565031),
			Common::DE_DEU,
			Common::kPlatformWindows,
			ADGF_UNSTABLE,
			GUIO1(GUIO_NONE)
		},
		kPrinceDataDE
	},
	{
		{
			"prince",
			"",
			AD_ENTRY1s("databank.ptc", "48ec9806bda9d152acbea8ce31c93c49", 3435298),
			Common::PL_POL,
			Common::kPlatformWindows,
			ADGF_UNSTABLE,
			GUIO1(GUIO_NONE)
		},
		kPrinceDataPL
	},
	{
		{
			"prince",
			"",
			AD_ENTRY1s("databank.ptc", "5fa03833177331214ec1354761b1d2ee", 3565031),
			Common::EN_ANY,
			Common::kPlatformWindows,
			ADGF_UNSTABLE,
			GUIO1(GUIO_NONE)
		},
		kPrinceDataDE
	},
	{
		{
			"prince",
			"",
			AD_ENTRY1s("databank.ptc", "48ec9806bda9d152acbea8ce31c93c49", 3435298),
			Common::EN_ANY,
			Common::kPlatformWindows,
			ADGF_UNSTABLE,
			GUIO1(GUIO_NONE)
		},
		kPrinceDataPL
	},
	{ AD_TABLE_END_MARKER, kPrinceDataUNK }
};

const static char *directoryGlobs[] = {
	"all",
	0
};

class PrinceMetaEngine : public AdvancedMetaEngine {
public:
	PrinceMetaEngine() : AdvancedMetaEngine(Prince::gameDescriptions, sizeof(Prince::PrinceGameDescription), princeGames) {
		_maxScanDepth = 2;
		_directoryGlobs = directoryGlobs;
	}

	const char *getEngineID() const {
		return "prince";
	}

	const char *getName() const {
		return "Prince Engine";
	}

	const char *getOriginalCopyright() const {
		return "The Prince and the Coward (C) 1996-97 Metropolis";
	}

	bool createInstance(OSystem *syst, Engine **engine, const ADGameDescription *desc) const;
	bool hasFeature(MetaEngineFeature f) const;
	int getMaximumSaveSlot() const;
	SaveStateList listSaves(const char *target) const;
	SaveStateDescriptor querySaveMetaInfos(const char *target, int slot) const;
	void removeSaveState(const char *target, int slot) const;
};

} // End of namespace Prince

#endif
