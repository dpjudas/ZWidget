#include "gzdoom_launcher.h"
#include "zwidget/widgets/textlabel/textlabel.h"
#include "zwidget/widgets/lineedit/lineedit.h"
#include "zwidget/widgets/pushbutton/pushbutton.h"
#include "zwidget/widgets/listview/listview.h"
#include "zwidget/widgets/dropdown/dropdown.h"
#include "zwidget/widgets/textedit/textedit.h"
#include "zwidget/widgets/checkboxlabel/checkboxlabel.h"
#include "zwidget/window/window.h"
#include "zwidget/systemdialogs/open_file_dialog.h"
#include "zwidget/core/span_layout.h"
#include <sstream>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>  // For SHGetFolderPath
#else
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#endif

// Helper function to get config file path
static std::string GetConfigFilePath()
{
#ifdef _WIN32
	// Windows: %APPDATA%/gzdoom-launcher/config.txt
	char appDataPath[MAX_PATH];
	if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath) == S_OK)
	{
		std::string configDir = std::string(appDataPath) + "\\gzdoom-launcher";
		CreateDirectoryA(configDir.c_str(), NULL); // Create if doesn't exist
		return configDir + "\\gzdoom_launcher_config.txt";
	}
#else
	// Linux/macOS: $HOME/.config/gzdoom-launcher/config.txt
	const char* home = getenv("HOME");
	if (!home)
	{
		// Fallback to getpwuid if HOME not set
		struct passwd* pw = getpwuid(getuid());
		if (pw)
			home = pw->pw_dir;
	}

	if (home)
	{
		std::string configDir = std::string(home) + "/.config/gzdoom-launcher";
		// Create directory if it doesn't exist
		mkdir(configDir.c_str(), 0755);
		return configDir + "/gzdoom_launcher_config.txt";
	}
#endif

	// Fallback: use current directory (legacy behavior)
	return "gzdoom_launcher_config.txt";
}

GZDoomLauncher::GZDoomLauncher() : Widget(nullptr, WidgetType::Window)
{
	SetWindowTitle("GZDoom Loader v3.0 - Multiplayer Edition");
	SetWindowBackground(Colorf::fromRgba8(32, 32, 32, 255));

	CreateUI();
	SetupCallbacks();
	LoadConfig();
	UpdatePWADList();
	UpdatePresetList();
	UpdateRecentConfigsList();
	UpdateMultiplayerUI();
	UpdateCommandPreview();
}

std::string RecentConfig::GetDisplayName() const
{
	std::ostringstream oss;
	std::string iwadName = iwadPath.substr(iwadPath.find_last_of("/\\") + 1);
	oss << iwadName;
	if (!pwadPaths.empty())
	{
		oss << " + " << pwadPaths.size() << " mod(s)";
	}
	if (!warpMap.empty())
	{
		oss << " [MAP" << warpMap << "]";
	}
	return oss.str();
}

// PresetNameDialog implementation
PresetNameDialog::PresetNameDialog(Widget* parent, std::function<void(const std::string&, const std::string&)> onAccept)
	: Widget(nullptr, WidgetType::Window), onAcceptCallback(onAccept)
{
	SetWindowTitle("Save Preset");
	SetFrameGeometry(0.0, 0.0, 450.0, 250.0);

	// Center dialog on parent
	if (parent)
	{
		Rect parentGeom = parent->GetFrameGeometry();
		double x = parentGeom.x + (parentGeom.width - 450.0) / 2.0;
		double y = parentGeom.y + (parentGeom.height - 250.0) / 2.0;
		SetFrameGeometry(x, y, 450.0, 250.0);
	}

	double y = 20.0;

	// Name label and input
	auto nameLabel = new TextLabel(this);
	nameLabel->SetText("Preset Name:");
	nameLabel->SetFrameGeometry(20.0, y, 410.0, 20.0);
	y += 25.0;

	nameEdit = new LineEdit(this);
	nameEdit->SetFrameGeometry(20.0, y, 410.0, 24.0);
	nameEdit->SetText("My Preset");
	y += 35.0;

	// Description label and input
	auto descLabel = new TextLabel(this);
	descLabel->SetText("Description (optional):");
	descLabel->SetFrameGeometry(20.0, y, 410.0, 20.0);
	y += 25.0;

	descriptionEdit = new TextEdit(this);
	descriptionEdit->SetFrameGeometry(20.0, y, 410.0, 80.0);
	y += 90.0;

	// Buttons
	okButton = new PushButton(this);
	okButton->SetText("OK");
	okButton->SetFrameGeometry(250.0, y, 80.0, 30.0);
	okButton->OnClick = [this]() { OnOK(); };

	cancelButton = new PushButton(this);
	cancelButton->SetText("Cancel");
	cancelButton->SetFrameGeometry(340.0, y, 80.0, 30.0);
	cancelButton->OnClick = [this]() { OnCancel(); };

	// Focus name field
	nameEdit->SetFocus();
	nameEdit->SelectAll();

	// Show the dialog
	Show();
}

void PresetNameDialog::OnOK()
{
	if (onAcceptCallback)
	{
		std::string name = nameEdit ? nameEdit->GetText() : "";
		std::string desc = descriptionEdit ? descriptionEdit->GetText() : "";
		onAcceptCallback(name, desc);
	}
	delete this;
}

void PresetNameDialog::OnCancel()
{
	delete this;
}

// DraggableListView implementation
DraggableListView::DraggableListView(Widget* parent) : ListView(parent)
{
}

bool DraggableListView::OnMouseDown(const Point& pos, InputKey key)
{
	// Only handle left mouse button
	if (key == InputKey::LeftMouse)
	{
		// Calculate which item was clicked
		double itemHeight = getItemHeight();
		int itemIndex = static_cast<int>(pos.y / itemHeight);

		if (itemIndex >= 0 && itemIndex < static_cast<int>(GetItemAmount()))
		{
			// Store drag start position and item index
			dragStartPos = pos;
			draggedItemIndex = itemIndex;
			// Don't start dragging yet - wait for threshold
		}
	}

	// Always call parent to preserve normal ListView behavior (selection, etc.)
	return ListView::OnMouseDown(pos, key);
}

void DraggableListView::OnMouseMove(const Point& pos)
{
	if (draggedItemIndex >= 0 && !isDragging)
	{
		// Check if mouse moved beyond threshold
		double dx = pos.x - dragStartPos.x;
		double dy = pos.y - dragStartPos.y;
		double distSq = dx * dx + dy * dy;

		if (distSq > DRAG_THRESHOLD * DRAG_THRESHOLD)
		{
			// Start dragging
			isDragging = true;
			SetPointerCapture();
			SetCursor(StandardCursor::size_ns);
		}
	}

	if (isDragging)
	{
		// Update drop target indicator
		double itemHeight = getItemHeight();
		int hoverIndex = static_cast<int>(pos.y / itemHeight);

		// Clamp to valid range
		if (hoverIndex < 0) hoverIndex = 0;
		if (hoverIndex >= static_cast<int>(GetItemAmount()))
			hoverIndex = static_cast<int>(GetItemAmount()) - 1;

		if (dropTargetIndex != hoverIndex)
		{
			dropTargetIndex = hoverIndex;
			Update(); // Trigger repaint to show drop indicator
		}
	}

	ListView::OnMouseMove(pos);
}

bool DraggableListView::OnMouseUp(const Point& pos, InputKey key)
{
	if (key == InputKey::LeftMouse && isDragging)
	{
		// Perform the reorder
		if (dropTargetIndex >= 0 &&
		    dropTargetIndex != draggedItemIndex &&
		    dropTargetIndex < static_cast<int>(GetItemAmount()))
		{
			// Notify parent to reorder the data
			if (OnReordered)
			{
				OnReordered(draggedItemIndex, dropTargetIndex);
			}
		}

		// Clean up drag state
		isDragging = false;
		draggedItemIndex = -1;
		dropTargetIndex = -1;
		ReleasePointerCapture();
		SetCursor(StandardCursor::arrow);
		Update(); // Clear visual feedback

		// Don't call parent - we handled the mouse up during drag
		return true;
	}
	else if (draggedItemIndex >= 0)
	{
		// Mouse up without dragging - just clear state
		draggedItemIndex = -1;
	}

	return ListView::OnMouseUp(pos, key);
}

void DraggableListView::OnPaint(Canvas* canvas)
{
	// Draw normal ListView content
	ListView::OnPaint(canvas);

	// Draw drop target indicator if dragging
	if (isDragging && dropTargetIndex >= 0)
	{
		double itemHeight = getItemHeight();
		double y = dropTargetIndex * itemHeight;

		// Draw a horizontal line to show drop position
		Rect lineRect = Rect::xywh(0.0, y - 1.0, GetWidth(), 2.0);
		canvas->fillRect(lineRect, Colorf::fromRgba8(0, 120, 215, 255)); // Blue indicator
	}
}

void GZDoomLauncher::CreateUI()
{
	double y = 20.0;
	double leftMargin = 20.0;
	double lineHeight = 30.0;
	double spacing = 10.0;

	// Title
	auto titleLabel = new TextLabel(this);
	titleLabel->SetText("GZDoom Launcher v3.0 - Multiplayer Edition");
	titleLabel->SetTextAlignment(TextLabelAlignment::Left);
	titleLabel->SetFrameGeometry(leftMargin, y, 670.0, 30.0);
	y += 40.0;

	// ===== IWAD Section =====
	auto iwadLabel = new TextLabel(this);
	iwadLabel->SetText("IWAD (Base Game):");
	iwadLabel->SetTextAlignment(TextLabelAlignment::Left);
	iwadLabel->SetFrameGeometry(leftMargin, y, 200.0, lineHeight);
	y += lineHeight + 5.0;

	iwadPathEdit = new LineEdit(this);
	iwadPathEdit->SetFrameGeometry(leftMargin, y, 460.0, lineHeight);
	iwadPathEdit->SetReadOnly(true);

	browseIWADButton = new PushButton(this);
	browseIWADButton->SetText("Browse...");
	browseIWADButton->SetFrameGeometry(490.0, y, 95.0, lineHeight);

	autoDetectIWADButton = new PushButton(this);
	autoDetectIWADButton->SetText("Auto-Detect");
	autoDetectIWADButton->SetFrameGeometry(595.0, y, 95.0, lineHeight);
	y += lineHeight + 5.0;

	// IWAD info label
	iwadInfoLabel = new TextLabel(this);
	iwadInfoLabel->SetText("");
	iwadInfoLabel->SetTextAlignment(TextLabelAlignment::Left);
	iwadInfoLabel->SetFrameGeometry(leftMargin, y, 670.0, lineHeight);
	y += lineHeight + spacing;

	// ===== GZDoom Executable Section =====
	auto gzdoomLabel = new TextLabel(this);
	gzdoomLabel->SetText("GZDoom Executable:");
	gzdoomLabel->SetTextAlignment(TextLabelAlignment::Left);
	gzdoomLabel->SetFrameGeometry(leftMargin, y, 200.0, lineHeight);
	y += lineHeight + 5.0;

	gzdoomPathEdit = new LineEdit(this);
	gzdoomPathEdit->SetFrameGeometry(leftMargin, y, 460.0, lineHeight);
	gzdoomPathEdit->SetReadOnly(true);

	browseGZDoomButton = new PushButton(this);
	browseGZDoomButton->SetText("Browse...");
	browseGZDoomButton->SetFrameGeometry(490.0, y, 95.0, lineHeight);

	autoDetectGZDoomButton = new PushButton(this);
	autoDetectGZDoomButton->SetText("Auto-Detect");
	autoDetectGZDoomButton->SetFrameGeometry(595.0, y, 95.0, lineHeight);
	y += lineHeight + spacing * 2;

	// ===== PWAD/PK3 Section =====
	auto pwadLabel = new TextLabel(this);
	pwadLabel->SetText("Mods (PWADs/PK3s) - Load Order:");
	pwadLabel->SetTextAlignment(TextLabelAlignment::Left);
	pwadLabel->SetFrameGeometry(leftMargin, y, 300.0, lineHeight);
	y += lineHeight + 5.0;

	// PWAD List
	pwadListView = new DraggableListView(this);
	pwadListView->SetFrameGeometry(leftMargin, y, 560.0, 150.0);
	pwadListView->SetColumnWidths({ 560.0 });

	// PWAD Control Buttons (right side)
	double buttonX = 590.0;
	double buttonWidth = 100.0;

	addPWADsButton = new PushButton(this);
	addPWADsButton->SetText("Add Files...");
	addPWADsButton->SetFrameGeometry(buttonX, y, buttonWidth, lineHeight);

	removePWADButton = new PushButton(this);
	removePWADButton->SetText("Remove");
	removePWADButton->SetFrameGeometry(buttonX, y + 35.0, buttonWidth, lineHeight);

	moveUpButton = new PushButton(this);
	moveUpButton->SetText("Move Up");
	moveUpButton->SetFrameGeometry(buttonX, y + 70.0, buttonWidth, lineHeight);

	moveDownButton = new PushButton(this);
	moveDownButton->SetText("Move Down");
	moveDownButton->SetFrameGeometry(buttonX, y + 105.0, buttonWidth, lineHeight);

	y += 160.0;

	// ===== Launch Options =====
	auto optionsLabel = new TextLabel(this);
	optionsLabel->SetText("Launch Options:");
	optionsLabel->SetTextAlignment(TextLabelAlignment::Left);
	optionsLabel->SetFrameGeometry(leftMargin, y, 200.0, lineHeight);
	y += lineHeight + 5.0;

	// Skill Level
	auto skillLabel = new TextLabel(this);
	skillLabel->SetText("Skill:");
	skillLabel->SetTextAlignment(TextLabelAlignment::Left);
	skillLabel->SetFrameGeometry(leftMargin, y, 80.0, lineHeight);

	skillDropdown = new Dropdown(this);
	skillDropdown->SetFrameGeometry(leftMargin + 80.0, y, 150.0, lineHeight);
	skillDropdown->AddItem("1 - I'm Too Young to Die");
	skillDropdown->AddItem("2 - Hey, Not Too Rough");
	skillDropdown->AddItem("3 - Hurt Me Plenty");
	skillDropdown->AddItem("4 - Ultra-Violence");
	skillDropdown->AddItem("5 - Nightmare!");
	skillDropdown->SetSelectedItem(2); // Default to skill 3

	// Warp Map
	auto warpLabel = new TextLabel(this);
	warpLabel->SetText("Warp:");
	warpLabel->SetTextAlignment(TextLabelAlignment::Left);
	warpLabel->SetFrameGeometry(leftMargin + 250.0, y, 60.0, lineHeight);

	warpEdit = new LineEdit(this);
	warpEdit->SetFrameGeometry(leftMargin + 310.0, y, 100.0, lineHeight);
	y += lineHeight + spacing;

	// Custom Parameters
	auto customLabel = new TextLabel(this);
	customLabel->SetText("Custom Params:");
	customLabel->SetTextAlignment(TextLabelAlignment::Left);
	customLabel->SetFrameGeometry(leftMargin, y, 120.0, lineHeight);

	customParamsEdit = new LineEdit(this);
	customParamsEdit->SetFrameGeometry(leftMargin + 120.0, y, 560.0, lineHeight);
	y += lineHeight + spacing * 2;

	// ===== Multiplayer Section =====
	multiplayerLabel = new TextLabel(this);
	multiplayerLabel->SetText("Multiplayer:");
	multiplayerLabel->SetTextAlignment(TextLabelAlignment::Left);
	multiplayerLabel->SetFrameGeometry(leftMargin, y, 200.0, lineHeight);
	y += lineHeight + 5.0;

	// Mode selection
	singlePlayerCheck = new CheckboxLabel(this);
	singlePlayerCheck->SetText("Single Player");
	singlePlayerCheck->SetFrameGeometry(leftMargin, y, 140.0, lineHeight);
	singlePlayerCheck->SetRadioStyle(true);
	singlePlayerCheck->SetChecked(true);

	hostGameCheck = new CheckboxLabel(this);
	hostGameCheck->SetText("Host Game");
	hostGameCheck->SetFrameGeometry(leftMargin + 150.0, y, 120.0, lineHeight);
	hostGameCheck->SetRadioStyle(true);

	joinGameCheck = new CheckboxLabel(this);
	joinGameCheck->SetText("Join Game");
	joinGameCheck->SetFrameGeometry(leftMargin + 280.0, y, 120.0, lineHeight);
	joinGameCheck->SetRadioStyle(true);
	y += lineHeight + spacing;

	// Host options (initially hidden/shown based on mode)
	auto hostPlayersLabel = new TextLabel(this);
	hostPlayersLabel->SetText("Players:");
	hostPlayersLabel->SetTextAlignment(TextLabelAlignment::Left);
	hostPlayersLabel->SetFrameGeometry(leftMargin, y, 80.0, lineHeight);

	hostPlayersEdit = new LineEdit(this);
	hostPlayersEdit->SetFrameGeometry(leftMargin + 80.0, y, 50.0, lineHeight);
	hostPlayersEdit->SetText("2");

	auto gameModeLabel = new TextLabel(this);
	gameModeLabel->SetText("Mode:");
	gameModeLabel->SetTextAlignment(TextLabelAlignment::Left);
	gameModeLabel->SetFrameGeometry(leftMargin + 150.0, y, 60.0, lineHeight);

	gameModeDropdown = new Dropdown(this);
	gameModeDropdown->SetFrameGeometry(leftMargin + 210.0, y, 150.0, lineHeight);
	gameModeDropdown->AddItem("Cooperative");
	gameModeDropdown->AddItem("Deathmatch");
	gameModeDropdown->AddItem("AltDeath");
	gameModeDropdown->SetSelectedItem(0);

	auto networkModeLabel = new TextLabel(this);
	networkModeLabel->SetText("Network:");
	networkModeLabel->SetTextAlignment(TextLabelAlignment::Left);
	networkModeLabel->SetFrameGeometry(leftMargin + 380.0, y, 70.0, lineHeight);

	networkModeDropdown = new Dropdown(this);
	networkModeDropdown->SetFrameGeometry(leftMargin + 450.0, y, 150.0, lineHeight);
	networkModeDropdown->AddItem("Peer-to-Peer");
	networkModeDropdown->AddItem("Packet Server");
	networkModeDropdown->SetSelectedItem(0);
	y += lineHeight + spacing;

	// Join options
	auto joinIPLabel = new TextLabel(this);
	joinIPLabel->SetText("Server IP:");
	joinIPLabel->SetTextAlignment(TextLabelAlignment::Left);
	joinIPLabel->SetFrameGeometry(leftMargin, y, 80.0, lineHeight);

	joinIPEdit = new LineEdit(this);
	joinIPEdit->SetFrameGeometry(leftMargin + 80.0, y, 150.0, lineHeight);
	joinIPEdit->SetText("127.0.0.1");

	auto portLabel = new TextLabel(this);
	portLabel->SetText("Port:");
	portLabel->SetTextAlignment(TextLabelAlignment::Left);
	portLabel->SetFrameGeometry(leftMargin + 250.0, y, 50.0, lineHeight);

	networkPortEdit = new LineEdit(this);
	networkPortEdit->SetFrameGeometry(leftMargin + 300.0, y, 80.0, lineHeight);
	networkPortEdit->SetText("5029");
	y += lineHeight + spacing * 2;

	// ===== Recent Configs Section =====
	recentLabel = new TextLabel(this);
	recentLabel->SetText("Recent Configurations:");
	recentLabel->SetTextAlignment(TextLabelAlignment::Left);
	recentLabel->SetFrameGeometry(leftMargin, y, 200.0, lineHeight);
	y += lineHeight + 5.0;

	recentConfigsList = new ListView(this);
	recentConfigsList->SetFrameGeometry(leftMargin, y, 670.0, 80.0);
	recentConfigsList->SetColumnWidths({ 670.0 });
	y += 90.0;

	// ===== Preset Section =====
	auto presetLabel = new TextLabel(this);
	presetLabel->SetText("Presets:");
	presetLabel->SetTextAlignment(TextLabelAlignment::Left);
	presetLabel->SetFrameGeometry(leftMargin, y, 100.0, lineHeight);

	presetDropdown = new Dropdown(this);
	presetDropdown->SetFrameGeometry(leftMargin + 100.0, y, 300.0, lineHeight);

	savePresetButton = new PushButton(this);
	savePresetButton->SetText("Save");
	savePresetButton->SetFrameGeometry(leftMargin + 410.0, y, 80.0, lineHeight);

	loadPresetButton = new PushButton(this);
	loadPresetButton->SetText("Load");
	loadPresetButton->SetFrameGeometry(leftMargin + 500.0, y, 80.0, lineHeight);

	deletePresetButton = new PushButton(this);
	deletePresetButton->SetText("Delete");
	deletePresetButton->SetFrameGeometry(leftMargin + 590.0, y, 100.0, lineHeight);
	y += lineHeight + spacing * 2;

	// ===== Command Preview =====
	auto commandLabel = new TextLabel(this);
	commandLabel->SetText("Command Preview:");
	commandLabel->SetTextAlignment(TextLabelAlignment::Left);
	commandLabel->SetFrameGeometry(leftMargin, y, 200.0, lineHeight);
	y += lineHeight + 5.0;

	commandPreview = new TextEdit(this);
	commandPreview->SetFrameGeometry(leftMargin, y, 670.0, 60.0);
	commandPreview->SetReadOnly(true);
	y += 70.0;

	// ===== Launch Button =====
	launchButton = new PushButton(this);
	launchButton->SetText("LAUNCH GZDOOM");
	launchButton->SetFrameGeometry(leftMargin, y, 670.0, 40.0);
	y += 50.0;

	// ===== Status Label =====
	statusLabel = new TextLabel(this);
	statusLabel->SetText("Ready - Use Auto-Detect buttons for quick setup");
	statusLabel->SetTextAlignment(TextLabelAlignment::Left);
	statusLabel->SetFrameGeometry(leftMargin, y, 670.0, lineHeight);
}

void GZDoomLauncher::SetupCallbacks()
{
	browseIWADButton->OnClick = [this]() { OnBrowseIWAD(); };
	autoDetectIWADButton->OnClick = [this]() { OnAutoDetectIWAD(); };
	browseGZDoomButton->OnClick = [this]() { OnBrowseGZDoom(); };
	autoDetectGZDoomButton->OnClick = [this]() { OnAutoDetectGZDoom(); };
	addPWADsButton->OnClick = [this]() { OnAddPWADs(); };
	removePWADButton->OnClick = [this]() { OnRemovePWAD(); };
	moveUpButton->OnClick = [this]() { OnMoveUp(); };
	moveDownButton->OnClick = [this]() { OnMoveDown(); };
	launchButton->OnClick = [this]() { OnLaunch(); };
	savePresetButton->OnClick = [this]() { OnSavePresetWithName(); };
	loadPresetButton->OnClick = [this]() { OnLoadPreset(); };
	deletePresetButton->OnClick = [this]() { OnDeletePreset(); };

	pwadListView->OnChanged = [this](int index) {
		selectedPWADIndex = index;
		UpdateCommandPreview();
	};

	// Drag-and-drop reordering callback
	pwadListView->OnReordered = [this](int fromIndex, int toIndex) {
		if (fromIndex >= 0 && fromIndex < static_cast<int>(pwadPaths.size()) &&
		    toIndex >= 0 && toIndex < static_cast<int>(pwadPaths.size()) &&
		    fromIndex != toIndex)
		{
			// Reorder the pwadPaths vector
			std::string movedItem = pwadPaths[fromIndex];
			pwadPaths.erase(pwadPaths.begin() + fromIndex);
			pwadPaths.insert(pwadPaths.begin() + toIndex, movedItem);

			// Update the UI to reflect the new order
			UpdatePWADList();
			UpdateCommandPreview();
			statusLabel->SetText("Reordered: " + GetFileName(movedItem));

			// Update selection to follow the moved item
			selectedPWADIndex = toIndex;
			pwadListView->SetSelectedItem(toIndex);
		}
	};

	presetDropdown->OnChanged = [this](int index) {
		OnPresetSelected(index);
	};

	// Update command preview when options change
	skillDropdown->OnChanged = [this](int) { UpdateCommandPreview(); };
	warpEdit->FuncAfterEditChanged = [this]() { UpdateCommandPreview(); };
	customParamsEdit->FuncAfterEditChanged = [this]() { UpdateCommandPreview(); };

	// Multiplayer mode callbacks
	singlePlayerCheck->FuncChanged = [this](bool checked) {
		if (checked) {
			currentMultiplayerMode = MultiplayerMode::SinglePlayer;
			hostGameCheck->SetChecked(false);
			joinGameCheck->SetChecked(false);
			OnMultiplayerModeChanged();
		}
	};

	hostGameCheck->FuncChanged = [this](bool checked) {
		if (checked) {
			currentMultiplayerMode = MultiplayerMode::Host;
			singlePlayerCheck->SetChecked(false);
			joinGameCheck->SetChecked(false);
			OnMultiplayerModeChanged();
		}
	};

	joinGameCheck->FuncChanged = [this](bool checked) {
		if (checked) {
			currentMultiplayerMode = MultiplayerMode::Join;
			singlePlayerCheck->SetChecked(false);
			hostGameCheck->SetChecked(false);
			OnMultiplayerModeChanged();
		}
	};

	// Multiplayer options callbacks for command preview update
	hostPlayersEdit->FuncAfterEditChanged = [this]() { UpdateCommandPreview(); };
	gameModeDropdown->OnChanged = [this](int) { UpdateCommandPreview(); };
	networkModeDropdown->OnChanged = [this](int) { UpdateCommandPreview(); };
	joinIPEdit->FuncAfterEditChanged = [this]() { UpdateCommandPreview(); };
	networkPortEdit->FuncAfterEditChanged = [this]() { UpdateCommandPreview(); };

	// Recent configs callback
	recentConfigsList->OnActivated = [this]() {
		int index = recentConfigsList->GetSelectedItem();
		if (index >= 0) {
			OnLoadRecentConfig(index);
		}
	};
}

void GZDoomLauncher::OnBrowseIWAD()
{
	auto dialog = OpenFileDialog::Create(this);
	dialog->SetTitle("Select IWAD File");
	dialog->AddFilter("WAD Files", "*.wad");
	dialog->AddFilter("All Files", "*.*");

	if (dialog->Show())
	{
		auto files = dialog->Filenames();
		if (!files.empty())
		{
			iwadPath = files[0];
			iwadPathEdit->SetText(iwadPath);
			OnIWADChanged();
		}
	}
}

void GZDoomLauncher::OnAutoDetectIWAD()
{
	statusLabel->SetText("Searching for IWADs...");
	auto iwads = WADParser::FindIWADs();

	if (iwads.empty())
	{
		statusLabel->SetText("No IWADs found. Please browse manually.");
		return;
	}

	// Use the first found IWAD
	iwadPath = iwads[0];
	iwadPathEdit->SetText(iwadPath);
	OnIWADChanged();

	statusLabel->SetText("Found " + std::to_string(iwads.size()) + " IWAD(s). Loaded: " + GetFileName(iwadPath));
}

void GZDoomLauncher::OnBrowseGZDoom()
{
	auto dialog = OpenFileDialog::Create(this);
	dialog->SetTitle("Select GZDoom Executable");
#ifdef _WIN32
	dialog->AddFilter("Executable Files", "*.exe");
#endif
	dialog->AddFilter("All Files", "*.*");

	if (dialog->Show())
	{
		auto files = dialog->Filenames();
		if (!files.empty())
		{
			gzdoomPath = files[0];
			gzdoomPathEdit->SetText(gzdoomPath);
			statusLabel->SetText("GZDoom executable selected: " + GetFileName(gzdoomPath));
			UpdateCommandPreview();
			SaveConfig();
		}
	}
}

void GZDoomLauncher::OnAutoDetectGZDoom()
{
	statusLabel->SetText("Searching for GZDoom...");
	auto executables = WADParser::FindGZDoom();

	if (executables.empty())
	{
		statusLabel->SetText("GZDoom not found. Please browse manually.");
		return;
	}

	// Use the first found executable
	gzdoomPath = executables[0];
	gzdoomPathEdit->SetText(gzdoomPath);
	statusLabel->SetText("Found GZDoom at: " + gzdoomPath);
	UpdateCommandPreview();
	SaveConfig();
}

void GZDoomLauncher::OnAddPWADs()
{
	auto dialog = OpenFileDialog::Create(this);
	dialog->SetTitle("Select PWAD/PK3 Files");
	dialog->AddFilter("Doom Mods", "*.wad;*.pk3;*.pk7;*.zip");
	dialog->AddFilter("WAD Files", "*.wad");
	dialog->AddFilter("PK3 Files", "*.pk3");
	dialog->AddFilter("All Files", "*.*");
	dialog->SetMultiSelect(true);

	if (dialog->Show())
	{
		auto files = dialog->Filenames();
		for (const auto& file : files)
		{
			pwadPaths.push_back(file);
		}
		UpdatePWADList();
		UpdateCommandPreview();
		statusLabel->SetText("Added " + std::to_string(files.size()) + " file(s)");
	}
}

void GZDoomLauncher::OnRemovePWAD()
{
	if (selectedPWADIndex >= 0 && selectedPWADIndex < static_cast<int>(pwadPaths.size()))
	{
		pwadPaths.erase(pwadPaths.begin() + selectedPWADIndex);
		UpdatePWADList();
		UpdateCommandPreview();
		statusLabel->SetText("File removed");
		selectedPWADIndex = -1;
	}
	else
	{
		statusLabel->SetText("No file selected to remove");
	}
}

void GZDoomLauncher::OnMoveUp()
{
	if (selectedPWADIndex > 0 && selectedPWADIndex < static_cast<int>(pwadPaths.size()))
	{
		std::swap(pwadPaths[selectedPWADIndex], pwadPaths[selectedPWADIndex - 1]);
		selectedPWADIndex--;
		UpdatePWADList();
		UpdateCommandPreview();
		pwadListView->SetSelectedItem(selectedPWADIndex);
		statusLabel->SetText("File moved up");
	}
}

void GZDoomLauncher::OnMoveDown()
{
	if (selectedPWADIndex >= 0 && selectedPWADIndex < static_cast<int>(pwadPaths.size()) - 1)
	{
		std::swap(pwadPaths[selectedPWADIndex], pwadPaths[selectedPWADIndex + 1]);
		selectedPWADIndex++;
		UpdatePWADList();
		UpdateCommandPreview();
		pwadListView->SetSelectedItem(selectedPWADIndex);
		statusLabel->SetText("File moved down");
	}
}

void GZDoomLauncher::OnLaunch()
{
	// Validate inputs
	if (gzdoomPath.empty())
	{
		statusLabel->SetText("Error: GZDoom executable not set");
		return;
	}

	if (iwadPath.empty())
	{
		statusLabel->SetText("Error: IWAD not selected");
		return;
	}

	std::string cmdLine = GenerateCommandLine();
	statusLabel->SetText("Launching: " + cmdLine);

	// Save to recent configs
	SaveRecentConfig();

#ifdef _WIN32
	// Windows: Use CreateProcess
	STARTUPINFOA si = {};
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi = {};

	std::string fullCmd = "\"" + gzdoomPath + "\" " + cmdLine;

	if (CreateProcessA(nullptr, const_cast<char*>(fullCmd.c_str()),
		nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi))
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		statusLabel->SetText("GZDoom launched successfully!");
	}
	else
	{
		statusLabel->SetText("Error: Failed to launch GZDoom");
	}
#else
	// Unix: Use fork/exec
	pid_t pid = fork();
	if (pid == 0)
	{
		// Child process
		std::vector<std::string> args;
		args.push_back(gzdoomPath);

		// Parse command line into arguments
		std::istringstream iss(cmdLine);
		std::string arg;
		while (iss >> arg)
		{
			args.push_back(arg);
		}

		// Convert to char* array
		std::vector<char*> argv;
		for (auto& a : args)
		{
			argv.push_back(const_cast<char*>(a.c_str()));
		}
		argv.push_back(nullptr);

		execv(gzdoomPath.c_str(), argv.data());
		_exit(1); // If exec fails
	}
	else if (pid > 0)
	{
		statusLabel->SetText("GZDoom launched successfully!");
	}
	else
	{
		statusLabel->SetText("Error: Failed to launch GZDoom");
	}
#endif
}

void GZDoomLauncher::OnSavePreset()
{
	// For now, create a simple preset with timestamp name
	LaunchPreset preset;
	preset.name = "Preset " + std::to_string(presets.size() + 1);
	preset.iwadPath = iwadPath;
	preset.pwadPaths = pwadPaths;
	preset.gzdoomPath = gzdoomPath;
	preset.skillLevel = skillDropdown->GetSelectedItem() + 1;
	preset.warpMap = warpEdit->GetText();
	preset.customParams = customParamsEdit->GetText();

	presets.push_back(preset);
	UpdatePresetList();
	SaveConfig();

	statusLabel->SetText("Preset saved: " + preset.name);
}

void GZDoomLauncher::OnLoadPreset()
{
	int index = presetDropdown->GetSelectedItem();
	if (index >= 0 && index < static_cast<int>(presets.size()))
	{
		const auto& preset = presets[index];

		iwadPath = preset.iwadPath;
		iwadPathEdit->SetText(iwadPath);

		pwadPaths = preset.pwadPaths;
		UpdatePWADList();

		gzdoomPath = preset.gzdoomPath;
		gzdoomPathEdit->SetText(gzdoomPath);

		skillDropdown->SetSelectedItem(preset.skillLevel - 1);
		warpEdit->SetText(preset.warpMap);
		customParamsEdit->SetText(preset.customParams);

		statusLabel->SetText("Preset loaded: " + preset.name);
	}
}

void GZDoomLauncher::OnDeletePreset()
{
	int index = presetDropdown->GetSelectedItem();
	if (index >= 0 && index < static_cast<int>(presets.size()))
	{
		std::string name = presets[index].name;
		presets.erase(presets.begin() + index);
		UpdatePresetList();
		SaveConfig();
		statusLabel->SetText("Preset deleted: " + name);
	}
}

void GZDoomLauncher::OnPresetSelected(int index)
{
	// Auto-load when selected for better UX
	if (index >= 0 && index < static_cast<int>(presets.size()))
	{
		OnLoadPreset();
	}
}

void GZDoomLauncher::UpdatePWADList()
{
	// Remove all items
	while (pwadListView->GetItemAmount() > 0)
	{
		pwadListView->RemoveItem(0);
	}

	// Add items back
	for (const auto& path : pwadPaths)
	{
		pwadListView->AddItem(GetFileName(path));
	}
}

void GZDoomLauncher::UpdatePresetList()
{
	presetDropdown->ClearItems();
	for (const auto& preset : presets)
	{
		presetDropdown->AddItem(preset.name);
	}
}

std::string GZDoomLauncher::GenerateCommandLine()
{
	std::ostringstream cmd;

	// Add multiplayer parameters first
	if (currentMultiplayerMode == MultiplayerMode::Host)
	{
		int players = std::stoi(hostPlayersEdit->GetText());
		cmd << "-host " << players;

		// Add game mode
		int modeIdx = gameModeDropdown->GetSelectedItem();
		if (modeIdx == 1) cmd << " -deathmatch";
		else if (modeIdx == 2) cmd << " -altdeath";

		// Add network mode
		int netModeIdx = networkModeDropdown->GetSelectedItem();
		if (netModeIdx == 1) cmd << " -netmode 1";

		// Add custom port if not default
		int port = std::stoi(networkPortEdit->GetText());
		if (port != 5029) cmd << " -port " << port;
	}
	else if (currentMultiplayerMode == MultiplayerMode::Join)
	{
		std::string ip = joinIPEdit->GetText();
		cmd << "-join " << ip;

		// Add custom port if not default
		int port = std::stoi(networkPortEdit->GetText());
		if (port != 5029) cmd << " -port " << port;
	}

	// Add IWAD
	if (currentMultiplayerMode != MultiplayerMode::SinglePlayer)
		cmd << " ";
	cmd << "-iwad \"" << iwadPath << "\"";

	// Add PWADs/PK3s
	if (!pwadPaths.empty())
	{
		cmd << " -file";
		for (const auto& path : pwadPaths)
		{
			cmd << " \"" << path << "\"";
		}
	}

	// Add skill level
	int skill = skillDropdown->GetSelectedItem() + 1;
	cmd << " -skill " << skill;

	// Add warp if specified
	std::string warp = warpEdit->GetText();
	if (!warp.empty())
	{
		cmd << " -warp " << warp;
	}

	// Add custom parameters
	std::string custom = customParamsEdit->GetText();
	if (!custom.empty())
	{
		cmd << " " << custom;
	}

	return cmd.str();
}

void GZDoomLauncher::LoadConfig()
{
	std::string configPath = GetConfigFilePath();
	std::ifstream file(configPath);
	if (!file.is_open())
		return;

	std::string line;

	// Read GZDoom path
	if (std::getline(file, line) && line.find("GZDOOM_PATH=") == 0)
	{
		gzdoomPath = line.substr(12);
		gzdoomPathEdit->SetText(gzdoomPath);
	}

	// Read recent configs
	int recentCount = 0;
	if (std::getline(file, line) && line.find("RECENT_COUNT=") == 0)
	{
		recentCount = std::stoi(line.substr(13));
	}

	for (int i = 0; i < recentCount; i++)
	{
		RecentConfig recent;

		if (std::getline(file, line) && line.find("RECENT_IWAD=") == 0)
			recent.iwadPath = line.substr(12);

		if (std::getline(file, line) && line.find("RECENT_SKILL=") == 0)
			recent.skillLevel = std::stoi(line.substr(13));

		if (std::getline(file, line) && line.find("RECENT_WARP=") == 0)
			recent.warpMap = line.substr(12);

		if (std::getline(file, line) && line.find("RECENT_TIMESTAMP=") == 0)
			recent.timestamp = line.substr(17);

		int pwadCount = 0;
		if (std::getline(file, line) && line.find("RECENT_PWAD_COUNT=") == 0)
			pwadCount = std::stoi(line.substr(18));

		for (int j = 0; j < pwadCount; j++)
		{
			if (std::getline(file, line) && line.find("RECENT_PWAD=") == 0)
			{
				recent.pwadPaths.push_back(line.substr(12));
			}
		}

		recentConfigs.push_back(recent);
	}

	// Read presets count
	int presetCount = 0;
	if (std::getline(file, line) && line.find("PRESET_COUNT=") == 0)
	{
		presetCount = std::stoi(line.substr(13));
	}

	// Read presets
	for (int i = 0; i < presetCount; i++)
	{
		LaunchPreset preset;

		if (std::getline(file, line) && line.find("PRESET_NAME=") == 0)
			preset.name = line.substr(12);

		if (std::getline(file, line) && line.find("PRESET_DESC=") == 0)
			preset.description = line.substr(12);

		if (std::getline(file, line) && line.find("PRESET_IWAD=") == 0)
			preset.iwadPath = line.substr(12);

		if (std::getline(file, line) && line.find("PRESET_GZDOOM=") == 0)
			preset.gzdoomPath = line.substr(14);

		if (std::getline(file, line) && line.find("PRESET_SKILL=") == 0)
			preset.skillLevel = std::stoi(line.substr(13));

		if (std::getline(file, line) && line.find("PRESET_WARP=") == 0)
			preset.warpMap = line.substr(12);

		if (std::getline(file, line) && line.find("PRESET_CUSTOM=") == 0)
			preset.customParams = line.substr(14);

		if (std::getline(file, line) && line.find("PRESET_MP_MODE=") == 0)
			preset.multiplayerMode = static_cast<MultiplayerMode>(std::stoi(line.substr(15)));

		if (std::getline(file, line) && line.find("PRESET_HOST_PLAYERS=") == 0)
			preset.hostPlayers = std::stoi(line.substr(20));

		if (std::getline(file, line) && line.find("PRESET_GAME_MODE=") == 0)
			preset.gameMode = static_cast<GameMode>(std::stoi(line.substr(17)));

		if (std::getline(file, line) && line.find("PRESET_NETWORK_MODE=") == 0)
			preset.networkMode = static_cast<NetworkMode>(std::stoi(line.substr(20)));

		if (std::getline(file, line) && line.find("PRESET_JOIN_IP=") == 0)
			preset.joinIP = line.substr(15);

		if (std::getline(file, line) && line.find("PRESET_PORT=") == 0)
			preset.networkPort = std::stoi(line.substr(12));

		int pwadCount = 0;
		if (std::getline(file, line) && line.find("PRESET_PWAD_COUNT=") == 0)
			pwadCount = std::stoi(line.substr(18));

		for (int j = 0; j < pwadCount; j++)
		{
			if (std::getline(file, line) && line.find("PRESET_PWAD=") == 0)
			{
				preset.pwadPaths.push_back(line.substr(12));
			}
		}

		presets.push_back(preset);
	}

	file.close();
}

void GZDoomLauncher::SaveConfig()
{
	std::string configPath = GetConfigFilePath();
	std::ofstream file(configPath);
	if (!file.is_open())
		return;

	// Save GZDoom path
	file << "GZDOOM_PATH=" << gzdoomPath << "\n";

	// Save recent configs
	file << "RECENT_COUNT=" << recentConfigs.size() << "\n";
	for (const auto& recent : recentConfigs)
	{
		file << "RECENT_IWAD=" << recent.iwadPath << "\n";
		file << "RECENT_SKILL=" << recent.skillLevel << "\n";
		file << "RECENT_WARP=" << recent.warpMap << "\n";
		file << "RECENT_TIMESTAMP=" << recent.timestamp << "\n";
		file << "RECENT_PWAD_COUNT=" << recent.pwadPaths.size() << "\n";
		for (const auto& pwad : recent.pwadPaths)
		{
			file << "RECENT_PWAD=" << pwad << "\n";
		}
	}

	// Save presets
	file << "PRESET_COUNT=" << presets.size() << "\n";

	for (const auto& preset : presets)
	{
		file << "PRESET_NAME=" << preset.name << "\n";
		file << "PRESET_DESC=" << preset.description << "\n";
		file << "PRESET_IWAD=" << preset.iwadPath << "\n";
		file << "PRESET_GZDOOM=" << preset.gzdoomPath << "\n";
		file << "PRESET_SKILL=" << preset.skillLevel << "\n";
		file << "PRESET_WARP=" << preset.warpMap << "\n";
		file << "PRESET_CUSTOM=" << preset.customParams << "\n";
		file << "PRESET_MP_MODE=" << static_cast<int>(preset.multiplayerMode) << "\n";
		file << "PRESET_HOST_PLAYERS=" << preset.hostPlayers << "\n";
		file << "PRESET_GAME_MODE=" << static_cast<int>(preset.gameMode) << "\n";
		file << "PRESET_NETWORK_MODE=" << static_cast<int>(preset.networkMode) << "\n";
		file << "PRESET_JOIN_IP=" << preset.joinIP << "\n";
		file << "PRESET_PORT=" << preset.networkPort << "\n";
		file << "PRESET_PWAD_COUNT=" << preset.pwadPaths.size() << "\n";

		for (const auto& pwad : preset.pwadPaths)
		{
			file << "PRESET_PWAD=" << pwad << "\n";
		}
	}

	file.close();
}

std::string GZDoomLauncher::GetFileName(const std::string& path)
{
	size_t pos = path.find_last_of("/\\");
	if (pos != std::string::npos)
		return path.substr(pos + 1);
	return path;
}

void GZDoomLauncher::OnIWADChanged()
{
	UpdateIWADInfo();
	UpdateCommandPreview();
	statusLabel->SetText("IWAD selected: " + GetFileName(iwadPath));
}

void GZDoomLauncher::UpdateIWADInfo()
{
	if (iwadPath.empty())
	{
		iwadInfoLabel->SetText("");
		currentIWADMetadata = WADMetadata();
		return;
	}

	// Parse WAD metadata
	currentIWADMetadata = WADParser::ParseWAD(iwadPath);

	if (!currentIWADMetadata.isValid)
	{
		iwadInfoLabel->SetText("Warning: Invalid WAD file");
		return;
	}

	// Build info string
	std::ostringstream info;
	info << currentIWADMetadata.wadType;

	if (!currentIWADMetadata.gameName.empty())
	{
		info << " - " << currentIWADMetadata.gameName;
	}

	if (currentIWADMetadata.HasMaps())
	{
		info << " - " << currentIWADMetadata.MapCount() << " map(s)";
	}

	info << " - " << currentIWADMetadata.numLumps << " lumps";

	iwadInfoLabel->SetText(info.str());
}

void GZDoomLauncher::UpdateCommandPreview()
{
	if (!commandPreview)
		return;

	if (gzdoomPath.empty() || iwadPath.empty())
	{
		commandPreview->SetText("Select GZDoom executable and IWAD to preview command");
		return;
	}

	std::string cmdLine = "\"" + gzdoomPath + "\" " + GenerateCommandLine();
	commandPreview->SetText(cmdLine);
}

void GZDoomLauncher::OnMultiplayerModeChanged()
{
	UpdateMultiplayerUI();
	UpdateCommandPreview();
}

void GZDoomLauncher::UpdateMultiplayerUI()
{
	// Enable/disable fields based on mode
	bool isHost = (currentMultiplayerMode == MultiplayerMode::Host);
	bool isJoin = (currentMultiplayerMode == MultiplayerMode::Join);

	// Host options
	if (hostPlayersEdit) hostPlayersEdit->SetEnabled(isHost);
	if (gameModeDropdown) gameModeDropdown->SetEnabled(isHost);
	if (networkModeDropdown) networkModeDropdown->SetEnabled(isHost || isJoin);

	// Join options
	if (joinIPEdit) joinIPEdit->SetEnabled(isJoin);

	// Port is used by both host and join
	if (networkPortEdit) networkPortEdit->SetEnabled(isHost || isJoin);
}

void GZDoomLauncher::OnLoadRecentConfig(int index)
{
	if (index < 0 || index >= static_cast<int>(recentConfigs.size()))
		return;

	const auto& recent = recentConfigs[index];

	// Load configuration
	iwadPath = recent.iwadPath;
	iwadPathEdit->SetText(iwadPath);
	OnIWADChanged();

	pwadPaths = recent.pwadPaths;
	UpdatePWADList();

	skillDropdown->SetSelectedItem(recent.skillLevel - 1);
	warpEdit->SetText(recent.warpMap);

	UpdateCommandPreview();
	statusLabel->SetText("Loaded recent config: " + recent.GetDisplayName());
}

void GZDoomLauncher::UpdateRecentConfigsList()
{
	if (!recentConfigsList)
		return;

	// Clear list
	while (recentConfigsList->GetItemAmount() > 0)
	{
		recentConfigsList->RemoveItem(0);
	}

	// Add recent configs
	for (const auto& recent : recentConfigs)
	{
		recentConfigsList->AddItem(recent.GetDisplayName());
	}
}

void GZDoomLauncher::SaveRecentConfig()
{
	if (iwadPath.empty())
		return;

	RecentConfig recent;
	recent.iwadPath = iwadPath;
	recent.pwadPaths = pwadPaths;
	recent.skillLevel = skillDropdown->GetSelectedItem() + 1;
	recent.warpMap = warpEdit->GetText();
	recent.timestamp = GetCurrentTimestamp();

	// Add to front of list
	recentConfigs.insert(recentConfigs.begin(), recent);

	// Keep only last 10
	if (recentConfigs.size() > 10)
	{
		recentConfigs.resize(10);
	}

	UpdateRecentConfigsList();
	SaveConfig();
}

void GZDoomLauncher::OnSavePresetWithName()
{
	// Show dialog to get preset name and description
	new PresetNameDialog(this, [this](const std::string& name, const std::string& description) {
		if (name.empty())
		{
			statusLabel->SetText("Preset name cannot be empty");
			return;
		}

		LaunchPreset preset;
		preset.name = name;
		preset.description = description;
		preset.iwadPath = iwadPath;
		preset.pwadPaths = pwadPaths;
		preset.gzdoomPath = gzdoomPath;
		preset.skillLevel = skillDropdown->GetSelectedItem() + 1;
		preset.warpMap = warpEdit->GetText();
		preset.customParams = customParamsEdit->GetText();

		// Save multiplayer settings
		preset.multiplayerMode = currentMultiplayerMode;
		preset.hostPlayers = std::stoi(hostPlayersEdit->GetText());
		preset.gameMode = static_cast<GameMode>(gameModeDropdown->GetSelectedItem());
		preset.networkMode = static_cast<NetworkMode>(networkModeDropdown->GetSelectedItem());
		preset.joinIP = joinIPEdit->GetText();
		preset.networkPort = std::stoi(networkPortEdit->GetText());

		presets.push_back(preset);
		UpdatePresetList();
		SaveConfig();
		statusLabel->SetText("Preset saved: " + name);
	});
}

std::string GZDoomLauncher::GetCurrentTimestamp()
{
	std::time_t now = std::time(nullptr);
	std::tm* tm = std::localtime(&now);
	std::ostringstream oss;
	oss << std::put_time(tm, "%Y-%m-%d %H:%M");
	return oss.str();
}
