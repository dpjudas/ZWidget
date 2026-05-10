#include "haiku_open_file_dialog.h"
#include "haiku_display_window.h"
#include <iostream>
#include <Entry.h>
#include <Path.h>
#include <FindDirectory.h>

HaikuOpenFilePanel::HaikuOpenFilePanel(BMessenger* target, BMessage* message, const entry_ref* start_directory, uint32 node_flavors, bool multiple_selection, bool allow_directories, BRefFilter* filter, const char* panel_name, bool modal)
	: BFilePanel(B_OPEN_PANEL, target, start_directory, node_flavors, multiple_selection, message, filter, modal, allow_directories)
{
	SetButtonLabel(B_DEFAULT_BUTTON, "Open");
	if (panel_name) SetPanelDirectory(start_directory);
}

void HaikuOpenFilePanel::MessageReceived(BMessage* message)
{
	switch (message->what)
	{
		case B_REFS_RECEIVED:
		{
			entry_ref ref;
			selected_files.clear();
			for (int i = 0; message->FindRef("refs", i, &ref) == B_OK; ++i)
			{
				BPath path(&ref);
				selected_files.push_back(path.Path());
			}
			if (!selected_files.empty())
			{
				selected_file = selected_files.front();
			}
			Window()->Quit(); // Close the panel
			break;
		}
		case B_CANCEL:
		{
			selected_file.clear();
			selected_files.clear();
			Window()->Quit(); // Close the panel
			break;
		}
		default:
			// BFilePanel::MessageReceived(message);
			break;
	}
}

void HaikuOpenFilePanel::SelectionChanged()
{
	// Not used for the current implementation
}

HaikuOpenFileDialog::HaikuOpenFileDialog(DisplayWindow* owner)
{
}

bool HaikuOpenFileDialog::Show()
{
	BMessage message(B_REFS_RECEIVED);
	BMessenger messenger(NULL, be_app);
	uint32 node_flavors = B_FILE_NODE;

	entry_ref initial_dir_ref;
	if (!initial_directory.empty())
	{
		BEntry entry(initial_directory.c_str());
		if (entry.InitCheck() == B_OK) {
			entry.GetRef(&initial_dir_ref);
		}
	}
	else
	{
		BPath path;
		find_directory(B_USER_DIRECTORY, &path);
		get_ref_for_path(path.Path(), &initial_dir_ref);
	}

	BMessenger msgr(be_app);
	HaikuOpenFilePanel* panel = new HaikuOpenFilePanel(
		&msgr,
		&message,
		&initial_dir_ref,
		node_flavors,
		multi_select,
		false,
		nullptr,
		title.empty() ? nullptr : title.c_str()
	);

	if (!initial_filename.empty())
	{
		panel->SetSaveText(initial_filename.c_str());
	}

	// Apply filters
	// Haiku's BFilePanel uses mime types for filtering. This needs a proper mapping.
	// For simplicity, we currently don't apply filters.

	panel->Show();

	// BFilePanel is asynchronous, so we need to wait for it to close
	while (panel->Window()->IsHidden() == false) {
		snooze(50000);
	}

	result_filename = panel->selected_file;
	result_filenames = panel->selected_files;

	delete panel;

	return !result_filename.empty();
}

std::string HaikuOpenFileDialog::Filename() const
{
	return result_filename;
}

std::vector<std::string> HaikuOpenFileDialog::Filenames() const
{
	return result_filenames;
}

void HaikuOpenFileDialog::SetMultiSelect(bool new_multi_select)
{
	multi_select = new_multi_select;
}

void HaikuOpenFileDialog::SetFilename(const std::string& filename)
{
	initial_filename = filename;
}

void HaikuOpenFileDialog::AddFilter(const std::string& filter_description, const std::string& filter_extension)
{
	filters.push_back({filter_description, filter_extension});
}

void HaikuOpenFileDialog::ClearFilters()
{
	filters.clear();
}

void HaikuOpenFileDialog::SetFilterIndex(int filter_index)
{
	this->filter_index = filter_index;
}

void HaikuOpenFileDialog::SetInitialDirectory(const std::string& path)
{
	initial_directory = path;
}

void HaikuOpenFileDialog::SetTitle(const std::string& newtitle)
{
	title = newtitle;
}

void HaikuOpenFileDialog::SetDefaultExtension(const std::string& extension)
{
	defaultext = extension;
}