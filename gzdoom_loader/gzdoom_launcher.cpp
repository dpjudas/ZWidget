#include "gzdoom_launcher.h"
#include "zwidget/widgets/textlabel/textlabel.h"
#include "zwidget/widgets/lineedit/lineedit.h"
#include "zwidget/widgets/pushbutton/pushbutton.h"
#include "zwidget/widgets/listview/listview.h"
#include "zwidget/widgets/dropdown/dropdown.h"
#include "zwidget/widgets/textedit/textedit.h"
#include "zwidget/window/window.h"
#include "zwidget/systemdialogs/open_file_dialog.h"
#include "zwidget/core/span_layout.h"
#include <sstream>
#include <fstream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

GZDoomLauncher::GZDoomLauncher() : Widget(nullptr, WidgetType::Window)
{
	SetWindowTitle("GZDoom Loader - Enhanced");
	SetWindowBackground(Colorf::fromRgba8(32, 32, 32, 255));

	CreateUI();
	SetupCallbacks();
	LoadConfig();
	UpdatePWADList();
	UpdatePresetList();
	UpdateCommandPreview();
}

void GZDoomLauncher::CreateUI()
{
	double y = 20.0;
	double leftMargin = 20.0;
	double lineHeight = 30.0;
	double spacing = 10.0;

	// Title
	auto titleLabel = new TextLabel(this);
	titleLabel->SetText("GZDoom Launcher - Enhanced Edition");
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
	pwadListView = new ListView(this);
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
	savePresetButton->OnClick = [this]() { OnSavePreset(); };
	loadPresetButton->OnClick = [this]() { OnLoadPreset(); };
	deletePresetButton->OnClick = [this]() { OnDeletePreset(); };

	pwadListView->OnChanged = [this](int index) {
		selectedPWADIndex = index;
		UpdateCommandPreview();
	};

	presetDropdown->OnChanged = [this](int index) {
		OnPresetSelected(index);
	};

	// Update command preview when options change
	skillDropdown->OnChanged = [this](int) { UpdateCommandPreview(); };
	warpEdit->FuncAfterEditChanged = [this]() { UpdateCommandPreview(); };
	customParamsEdit->FuncAfterEditChanged = [this]() { UpdateCommandPreview(); };
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

	// Add IWAD
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
	std::ifstream file("gzdoom_launcher_config.txt");
	if (!file.is_open())
		return;

	std::string line;

	// Read GZDoom path
	if (std::getline(file, line) && line.find("GZDOOM_PATH=") == 0)
	{
		gzdoomPath = line.substr(12);
		gzdoomPathEdit->SetText(gzdoomPath);
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
	std::ofstream file("gzdoom_launcher_config.txt");
	if (!file.is_open())
		return;

	// Save GZDoom path
	file << "GZDOOM_PATH=" << gzdoomPath << "\n";

	// Save presets
	file << "PRESET_COUNT=" << presets.size() << "\n";

	for (const auto& preset : presets)
	{
		file << "PRESET_NAME=" << preset.name << "\n";
		file << "PRESET_IWAD=" << preset.iwadPath << "\n";
		file << "PRESET_GZDOOM=" << preset.gzdoomPath << "\n";
		file << "PRESET_SKILL=" << preset.skillLevel << "\n";
		file << "PRESET_WARP=" << preset.warpMap << "\n";
		file << "PRESET_CUSTOM=" << preset.customParams << "\n";
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
