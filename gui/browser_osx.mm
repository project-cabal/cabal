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

#include "gui/browser.h"

#include "backends/platform/darwin/cfref.h"
#include "common/config-manager.h"
#include "common/system.h"
#include "common/algorithm.h"
#include "common/translation.h"

#include <AppKit/NSNibDeclarations.h>
#include <AppKit/NSOpenPanel.h>
#include <AppKit/NSButton.h>
#include <Foundation/NSString.h>
#include <Foundation/NSURL.h>

@interface ShowHiddenFilesController : NSObject {
	NSOpenPanel* _panel;
}

- (id) init;
- (void) dealloc;
- (void) setOpenPanel : (NSOpenPanel*) panel;
- (IBAction) showHiddenFiles : (id) sender;

@end

@implementation ShowHiddenFilesController

- (id) init {
	self = [super init];
	_panel = 0;
	
	return self;
}

- (void) dealloc {
	[_panel release];
	[super dealloc];
}

- (void) setOpenPanel : (NSOpenPanel*) panel {
	_panel = panel;
	[_panel retain];
}


- (IBAction) showHiddenFiles : (id) sender {
	if ([sender state] == NSOnState) {
		[_panel setShowsHiddenFiles: YES];
		ConfMan.setBool("gui_browser_show_hidden", true, Common::ConfigManager::kApplicationDomain);
	} else {
		[_panel setShowsHiddenFiles: NO];
		ConfMan.setBool("gui_browser_show_hidden", false, Common::ConfigManager::kApplicationDomain);
	}
}

@end

namespace GUI {

BrowserDialog::BrowserDialog(const char *title, bool dirBrowser)
	: Dialog("Browser") {

	// remember whether this is a file browser or a directory browser.
	_isDirBrowser = dirBrowser;

	// Get current encoding
#ifdef USE_TRANSLATION
	ScopedCFRef<CFStringRef> encStr(CFStringCreateWithCString(NULL, TransMan.getCurrentCharset().c_str(), kCFStringEncodingASCII));
	CFStringEncoding stringEncoding = CFStringConvertIANACharSetNameToEncoding(encStr.get());
#else
	CFStringEncoding stringEncoding = kCFStringEncodingASCII;
#endif

	// Convert title to NSString
	_titleRef = CFStringCreateWithCString(0, title, stringEncoding);

	// Convert button text to NSString
	_chooseRef = CFStringCreateWithCString(0, _("Choose"), stringEncoding);
	_hiddenFilesRef = CFStringCreateWithCString(0, _("Show hidden files"), stringEncoding);
}

BrowserDialog::~BrowserDialog() {
	CFRelease(_titleRef);
	CFRelease(_chooseRef);
	CFRelease(_hiddenFilesRef);
}

int BrowserDialog::runModal() {
	bool choiceMade = false;

	// If in fullscreen mode, switch to windowed mode
	bool wasFullscreen = g_system->getFeatureState(OSystem::kFeatureFullscreenMode);
	if (wasFullscreen) {
		g_system->beginGFXTransaction();
		g_system->setFeatureState(OSystem::kFeatureFullscreenMode, false);
		g_system->endGFXTransaction();
	}

	// Temporarily show the real mouse
	CGDisplayShowCursor(kCGDirectMainDisplay);

	NSOpenPanel *panel = [NSOpenPanel openPanel];
	[panel setCanChooseFiles:!_isDirBrowser];
	[panel setCanChooseDirectories:_isDirBrowser];
	if (_isDirBrowser)
		[panel setTreatsFilePackagesAsDirectories:true];
	[panel setTitle:(NSString *)_titleRef];
	[panel setPrompt:(NSString *)_chooseRef];

	NSButton *showHiddenFilesButton = 0;
	ShowHiddenFilesController *showHiddenFilesController = 0;
	if ([panel respondsToSelector:@selector(setShowsHiddenFiles:)]) {
		showHiddenFilesButton = [[NSButton alloc] init];
		[showHiddenFilesButton setButtonType:NSSwitchButton];
		[showHiddenFilesButton setTitle:(NSString *)_hiddenFilesRef];
		[showHiddenFilesButton sizeToFit];
		if (ConfMan.getBool("gui_browser_show_hidden", Common::ConfigManager::kApplicationDomain)) {
			[showHiddenFilesButton setState:NSOnState];
			[panel setShowsHiddenFiles: YES];
		} else {
			[showHiddenFilesButton setState:NSOffState];
			[panel setShowsHiddenFiles: NO];
		}
		[panel setAccessoryView:showHiddenFilesButton];

		showHiddenFilesController = [[ShowHiddenFilesController alloc] init];
		[showHiddenFilesController setOpenPanel:panel];
		[showHiddenFilesButton setTarget:showHiddenFilesController];
		[showHiddenFilesButton setAction:@selector(showHiddenFiles:)];
	}

#if MAC_OS_X_VERSION_MAX_ALLOWED <= 1090
	if ([panel runModal] == NSOKButton) {
#else
	if ([panel runModal] == NSModalResponseOK) {
#endif
		NSURL *url = [panel URL];
		if ([url isFileURL]) {
			const char *filename = [[url path] UTF8String];
			_choice = Common::FSNode(filename);
			choiceMade = true;
		}
	}

	[showHiddenFilesButton release];
	[showHiddenFilesController release];

	// If we were in fullscreen mode, switch back
	if (wasFullscreen) {
		g_system->beginGFXTransaction();
		g_system->setFeatureState(OSystem::kFeatureFullscreenMode, true);
		g_system->endGFXTransaction();
	}

	return choiceMade;
}

} // End of namespace GUI
