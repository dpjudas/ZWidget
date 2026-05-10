#pragma once

#include "window/window.h"

class HaikuDisplayBackend : public DisplayBackend
{
public:
	HaikuDisplayBackend();
	std::unique_ptr<DisplayWindow> Create(DisplayWindowHost* windowHost, WidgetType type, DisplayWindow* owner, RenderAPI renderAPI) override;
	void ProcessEvents() override;
	void RunLoop() override;
	void ExitLoop() override;

	void* StartTimer(int timeoutMilliseconds, std::function<void()> onTimer) override;
	void StopTimer(void* timerID) override;

	Size GetScreenSize() override;

	std::unique_ptr<OpenFileDialog> CreateOpenFileDialog(DisplayWindow* owner) override;
	std::unique_ptr<SaveFileDialog> CreateSaveFileDialog(DisplayWindow* owner) override;
	std::unique_ptr<OpenFolderDialog> CreateOpenFolderDialog(DisplayWindow* owner) override;

	bool IsHaiku() override { return true; };
};
