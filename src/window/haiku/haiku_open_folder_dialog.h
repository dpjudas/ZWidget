#pragma once

#include "systemdialogs/open_folder_dialog.h"
#include <FilePanel.h>
#include <Path.h>
#include <Messenger.h>

class DisplayWindow;

class HaikuOpenFolderPanel : public BFilePanel
{
public:
	HaikuOpenFolderPanel(BMessenger* target, BMessage* message, const entry_ref* start_directory, uint32 node_flavors, bool multiple_selection, bool allow_directories, BRefFilter* filter = NULL, const char* panel_name = NULL, bool modal = false);

	void MessageReceived(BMessage* message);

	std::string selected_path;
};

class HaikuOpenFolderDialog : public OpenFolderDialog
{
public:
	HaikuOpenFolderDialog(DisplayWindow* owner);

	bool Show() override;
	std::string SelectedPath() const override;
	void SetInitialDirectory(const std::string& path) override;
	void SetTitle(const std::string& newtitle) override;

private:
	std::string initial_directory;
	std::string title;

	std::string result_path;
};