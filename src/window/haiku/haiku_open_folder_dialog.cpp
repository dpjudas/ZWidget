#include "haiku_open_folder_dialog.h"
#include "haiku_display_window.h"
#include <iostream>
#include <Entry.h>
#include <Path.h>
#include <FindDirectory.h>

HaikuOpenFolderPanel::HaikuOpenFolderPanel(BMessenger* target, BMessage* message, const entry_ref* start_directory, uint32 node_flavors, bool multiple_selection, bool allow_directories, BRefFilter* filter, const char* panel_name, bool modal)
	: BFilePanel(B_OPEN_PANEL, target, start_directory, node_flavors, multiple_selection, message, filter, modal, allow_directories)
{
	SetButtonLabel(B_DEFAULT_BUTTON, "Open");
	if (panel_name) SetPanelDirectory(start_directory);
}

void HaikuOpenFolderPanel::MessageReceived(BMessage* message)
{
	switch (message->what)
	{
		case B_REFS_RECEIVED:
		{
			entry_ref ref;
			if (message->FindRef("refs", 0, &ref) == B_OK)
			{
				BPath path(&ref);
				selected_path = path.Path();
			}
			Window()->Quit(); // Close the panel
			break;
		}
		case B_CANCEL:
		{
			selected_path.clear();
			Window()->Quit(); // Close the panel
			break;
		}
		default:
			// BFilePanel::MessageReceived(message);
			break;
	}
}

HaikuOpenFolderDialog::HaikuOpenFolderDialog(DisplayWindow* owner)
{
}

bool HaikuOpenFolderDialog::Show()
{
	BMessage message(B_REFS_RECEIVED);
	uint32 node_flavors = B_DIRECTORY_NODE;

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
	HaikuOpenFolderPanel* panel = new HaikuOpenFolderPanel(
		&msgr,
		&message,
		&initial_dir_ref,
		node_flavors,
		false,
		true,
		nullptr,
		title.empty() ? nullptr : title.c_str()
	);

	panel->Show();

	while (panel->Window()->IsHidden() == false) {
		snooze(50000);
	}

	result_path = panel->selected_path;

	delete panel;

	return !result_path.empty();
}

std::string HaikuOpenFolderDialog::SelectedPath() const
{
	return result_path;
}

void HaikuOpenFolderDialog::SetInitialDirectory(const std::string& path)
{
	initial_directory = path;
}

void HaikuOpenFolderDialog::SetTitle(const std::string& newtitle)
{
	title = newtitle;
}