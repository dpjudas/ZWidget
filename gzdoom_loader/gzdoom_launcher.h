#pragma once

#include "zwidget/core/widget.h"
#include <string>
#include <vector>

class TextLabel;
class LineEdit;
class PushButton;
class ListView;
class Dropdown;

// Structure to hold a preset configuration
struct LaunchPreset
{
	std::string name;
	std::string iwadPath;
	std::vector<std::string> pwadPaths;
	std::string gzdoomPath;
	int skillLevel = 3;
	std::string warpMap;
	std::string customParams;
};

class GZDoomLauncher : public Widget
{
public:
	GZDoomLauncher();

private:
	void CreateUI();
	void SetupCallbacks();

	// UI Event handlers
	void OnBrowseIWAD();
	void OnBrowseGZDoom();
	void OnAddPWADs();
	void OnRemovePWAD();
	void OnMoveUp();
	void OnMoveDown();
	void OnLaunch();
	void OnSavePreset();
	void OnLoadPreset();
	void OnDeletePreset();
	void OnPresetSelected(int index);

	// Helper functions
	void UpdatePWADList();
	void UpdatePresetList();
	std::string GenerateCommandLine();
	void LoadConfig();
	void SaveConfig();
	std::string GetFileName(const std::string& path);

	// UI Components
	LineEdit* iwadPathEdit = nullptr;
	LineEdit* gzdoomPathEdit = nullptr;
	ListView* pwadListView = nullptr;
	Dropdown* skillDropdown = nullptr;
	LineEdit* warpEdit = nullptr;
	LineEdit* customParamsEdit = nullptr;
	Dropdown* presetDropdown = nullptr;
	TextLabel* statusLabel = nullptr;

	PushButton* browseIWADButton = nullptr;
	PushButton* browseGZDoomButton = nullptr;
	PushButton* addPWADsButton = nullptr;
	PushButton* removePWADButton = nullptr;
	PushButton* moveUpButton = nullptr;
	PushButton* moveDownButton = nullptr;
	PushButton* launchButton = nullptr;
	PushButton* savePresetButton = nullptr;
	PushButton* loadPresetButton = nullptr;
	PushButton* deletePresetButton = nullptr;

	// Data
	std::string iwadPath;
	std::string gzdoomPath;
	std::vector<std::string> pwadPaths;
	std::vector<LaunchPreset> presets;
	int selectedPWADIndex = -1;
};
