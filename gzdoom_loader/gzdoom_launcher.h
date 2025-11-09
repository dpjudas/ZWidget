#pragma once

#include "zwidget/core/widget.h"
#include "wad_parser.h"
#include <string>
#include <vector>

class TextLabel;
class LineEdit;
class PushButton;
class ListView;
class Dropdown;
class TextEdit;
class CheckboxLabel;

// Multiplayer mode enumeration
enum class MultiplayerMode
{
	SinglePlayer,
	Host,
	Join
};

// Game mode enumeration
enum class GameMode
{
	Cooperative,
	Deathmatch,
	AltDeath
};

// Network mode enumeration
enum class NetworkMode
{
	PeerToPeer,    // -netmode 0 (default, faster)
	PacketServer   // -netmode 1 (firewall friendly)
};

// Structure to hold a preset configuration
struct LaunchPreset
{
	std::string name;
	std::string description;
	std::string iwadPath;
	std::vector<std::string> pwadPaths;
	std::string gzdoomPath;
	int skillLevel = 3;
	std::string warpMap;
	std::string customParams;

	// Multiplayer settings
	MultiplayerMode multiplayerMode = MultiplayerMode::SinglePlayer;
	int hostPlayers = 2;
	GameMode gameMode = GameMode::Cooperative;
	NetworkMode networkMode = NetworkMode::PeerToPeer;
	std::string joinIP = "127.0.0.1";
	int networkPort = 5029;
};

// Structure to hold recent configuration
struct RecentConfig
{
	std::string iwadPath;
	std::vector<std::string> pwadPaths;
	int skillLevel = 3;
	std::string warpMap;
	std::string timestamp;

	std::string GetDisplayName() const;
};

// Simple dialog for entering preset name and description
class PresetNameDialog : public Widget
{
public:
	PresetNameDialog(Widget* parent, std::function<void(const std::string&, const std::string&)> onAccept);

private:
	void OnOK();
	void OnCancel();

	LineEdit* nameEdit = nullptr;
	TextEdit* descriptionEdit = nullptr;
	PushButton* okButton = nullptr;
	PushButton* cancelButton = nullptr;
	std::function<void(const std::string&, const std::string&)> onAcceptCallback;
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
	void OnAutoDetectIWAD();
	void OnBrowseGZDoom();
	void OnAutoDetectGZDoom();
	void OnAddPWADs();
	void OnRemovePWAD();
	void OnMoveUp();
	void OnMoveDown();
	void OnLaunch();
	void OnSavePreset();
	void OnSavePresetWithName();
	void OnLoadPreset();
	void OnDeletePreset();
	void OnPresetSelected(int index);
	void OnIWADChanged();
	void OnMultiplayerModeChanged();
	void OnLoadRecentConfig(int index);

	// Helper functions
	void UpdatePWADList();
	void UpdatePresetList();
	void UpdateRecentConfigsList();
	void UpdateCommandPreview();
	void UpdateIWADInfo();
	void UpdateMultiplayerUI();
	void SaveRecentConfig();
	std::string GenerateCommandLine();
	void LoadConfig();
	void SaveConfig();
	std::string GetFileName(const std::string& path);
	std::string GetCurrentTimestamp();

	// UI Components
	LineEdit* iwadPathEdit = nullptr;
	LineEdit* gzdoomPathEdit = nullptr;
	ListView* pwadListView = nullptr;
	Dropdown* skillDropdown = nullptr;
	LineEdit* warpEdit = nullptr;
	LineEdit* customParamsEdit = nullptr;
	Dropdown* presetDropdown = nullptr;
	TextLabel* statusLabel = nullptr;
	TextLabel* iwadInfoLabel = nullptr;
	TextEdit* commandPreview = nullptr;

	PushButton* browseIWADButton = nullptr;
	PushButton* autoDetectIWADButton = nullptr;
	PushButton* browseGZDoomButton = nullptr;
	PushButton* autoDetectGZDoomButton = nullptr;
	PushButton* addPWADsButton = nullptr;
	PushButton* removePWADButton = nullptr;
	PushButton* moveUpButton = nullptr;
	PushButton* moveDownButton = nullptr;
	PushButton* launchButton = nullptr;
	PushButton* savePresetButton = nullptr;
	PushButton* loadPresetButton = nullptr;
	PushButton* deletePresetButton = nullptr;

	// Multiplayer UI components
	CheckboxLabel* singlePlayerCheck = nullptr;
	CheckboxLabel* hostGameCheck = nullptr;
	CheckboxLabel* joinGameCheck = nullptr;
	LineEdit* hostPlayersEdit = nullptr;
	Dropdown* gameModeDropdown = nullptr;
	Dropdown* networkModeDropdown = nullptr;
	LineEdit* joinIPEdit = nullptr;
	LineEdit* networkPortEdit = nullptr;
	TextLabel* multiplayerLabel = nullptr;

	// Recent configs UI components
	ListView* recentConfigsList = nullptr;
	TextLabel* recentLabel = nullptr;

	// Data
	std::string iwadPath;
	std::string gzdoomPath;
	std::vector<std::string> pwadPaths;
	std::vector<LaunchPreset> presets;
	std::vector<RecentConfig> recentConfigs;
	WADMetadata currentIWADMetadata;
	int selectedPWADIndex = -1;

	// Multiplayer data
	MultiplayerMode currentMultiplayerMode = MultiplayerMode::SinglePlayer;
	int hostPlayers = 2;
	GameMode currentGameMode = GameMode::Cooperative;
	NetworkMode currentNetworkMode = NetworkMode::PeerToPeer;
	std::string joinIP = "127.0.0.1";
	int networkPort = 5029;
};
