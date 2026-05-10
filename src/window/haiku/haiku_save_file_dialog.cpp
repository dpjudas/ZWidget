#include "haiku_save_file_dialog.h"
#include "haiku_display_window.h"
#include <iostream>
#include <Entry.h>
#include <Path.h>
#include <FindDirectory.h>

HaikuSaveFilePanel::HaikuSaveFilePanel(BMessenger* target, BMessage* message, const entry_ref* start_directory, uint32 node_flavors, bool multiple_selection, bool allow_directories, BRefFilter* filter, const char* panel_name, bool modal)
	: BFilePanel(B_SAVE_PANEL, target, start_directory, node_flavors, multiple_selection, message, filter, modal, allow_directories)
{
	SetButtonLabel(B_DEFAULT_BUTTON, "Save");
	if (panel_name) SetPanelDirectory(start_directory);
}

void HaikuSaveFilePanel::MessageReceived(BMessage* message)
{
	switch (message->what)
	{
		case B_SAVE_REQUESTED:
		{
			entry_ref dir_ref;
			const char* name;
			if (message->FindRef("directory", &dir_ref) == B_OK && message->FindString("name", &name) == B_OK)
			{
				BPath path(&dir_ref);
				path.Append(name);
				selected_file = path.Path();
			}
			Window()->Quit(); // Close the panel
			break;
		}
		case B_CANCEL:
		{
			selected_file.clear();
			Window()->Quit(); // Close the panel
			break;
		}
		default:
			// BFilePanel::MessageReceived(message);
			break;
	}
}

HaikuSaveFileDialog::HaikuSaveFileDialog(DisplayWindow* owner)
{
}

bool HaikuSaveFileDialog::Show()
{
	BMessage message(B_SAVE_REQUESTED);
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
	HaikuSaveFilePanel* panel = new HaikuSaveFilePanel(
		&msgr,
		&message,
		&initial_dir_ref,
		node_flavors,
		false,
		false,
		nullptr,
		title.empty() ? nullptr : title.c_str()
	);
	
	if (!initial_filename.empty())
	{
		panel->SetSaveText(initial_filename.c_str());
	}

	panel->Show();

	while (panel->Window()->IsHidden() == false) {
		snooze(50000);
	}

	result_filename = panel->selected_file;

	delete panel;

	return !result_filename.empty();
}

std::string HaikuSaveFileDialog::Filename() const
{
	return result_filename;
}

void HaikuSaveFileDialog::SetFilename(const std::string& filename)
{
	initial_filename = filename;
}

void HaikuSaveFileDialog::AddFilter(const std::string& filter_description, const std::string& filter_extension)
{
	filters.push_back({filter_description, filter_extension});
}

void HaikuSaveFileDialog::ClearFilters()
{
	filters.clear();
}

void HaikuSaveFileDialog::SetFilterIndex(int filter_index)
{
	this->filter_index = filter_index;
}

void HaikuSaveFileDialog::SetInitialDirectory(const std::string& path)
{
	initial_directory = path;
}

void HaikuSaveFileDialog::SetTitle(const std::string& newtitle)
{
	title = newtitle;
}

void HaikuSaveFileDialog::SetDefaultExtension(const std::string& extension)
{
	defaultext = extension;
}