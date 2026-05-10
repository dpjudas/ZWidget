#pragma once

#include "systemdialogs/open_file_dialog.h"
#include <FilePanel.h>
#include <Path.h>
#include <Messenger.h>

class DisplayWindow;

class HaikuOpenFilePanel : public BFilePanel
{
public:
	HaikuOpenFilePanel(BMessenger* target, BMessage* message, const entry_ref* start_directory, uint32 node_flavors, bool multiple_selection, bool allow_directories, BRefFilter* filter = NULL, const char* panel_name = NULL, bool modal = false);

	void MessageReceived(BMessage* message);
	void SelectionChanged() override;

	std::string selected_file;
	std::vector<std::string> selected_files;
};

class HaikuOpenFileDialog : public OpenFileDialog
{
public:
	HaikuOpenFileDialog(DisplayWindow* owner);

	bool Show() override;
	std::string Filename() const override;
	std::vector<std::string> Filenames() const override;
	void SetMultiSelect(bool new_multi_select) override;
	void SetFilename(const std::string& filename) override;
	void AddFilter(const std::string& filter_description, const std::string& filter_extension) override;
	void ClearFilters() override;
	void SetFilterIndex(int filter_index) override;
	void SetInitialDirectory(const std::string& path) override;
	void SetTitle(const std::string& newtitle) override;
	void SetDefaultExtension(const std::string& extension) override;

private:
	bool multi_select = false;
	std::string initial_directory;
	std::string initial_filename;
	std::string title;
	std::string defaultext;
	
	std::vector<std::pair<std::string, std::string>> filters;
	int filter_index = 0;

	std::string result_filename;
	std::vector<std::string> result_filenames;
};