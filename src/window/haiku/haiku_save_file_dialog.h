#pragma once

#include "systemdialogs/save_file_dialog.h"
#include <FilePanel.h>
#include <Path.h>
#include <Messenger.h>

class DisplayWindow;

class HaikuSaveFilePanel : public BFilePanel
{
public:
	HaikuSaveFilePanel(BMessenger* target, BMessage* message, const entry_ref* start_directory, uint32 node_flavors, bool multiple_selection, bool allow_directories, BRefFilter* filter = NULL, const char* panel_name = NULL, bool modal = false);

	void MessageReceived(BMessage* message);

	std::string selected_file;
};

class HaikuSaveFileDialog : public SaveFileDialog
{
public:
	HaikuSaveFileDialog(DisplayWindow* owner);

	bool Show() override;
	std::string Filename() const override;
	void SetFilename(const std::string& filename) override;
	void AddFilter(const std::string& filter_description, const std::string& filter_extension) override;
	void ClearFilters() override;
	void SetFilterIndex(int filter_index) override;
	void SetInitialDirectory(const std::string& path) override;
	void SetTitle(const std::string& newtitle) override;
	void SetDefaultExtension(const std::string& extension) override;

private:
	std::string initial_directory;
	std::string initial_filename;
	std::string title;
	std::string defaultext;
	
	std::vector<std::pair<std::string, std::string>> filters;
	int filter_index = 0;

	std::string result_filename;
};