#include "haiku_display_backend.h"
#include "haiku_display_window.h"
#include "haiku_app.h"
#include "haiku_open_file_dialog.h"
#include "haiku_save_file_dialog.h"
#include "haiku_open_folder_dialog.h"
#include <iostream>
#include "haiku_display_backend.h"
#include "haiku_display_window.h"
#include "haiku_app.h"
#include "haiku_open_file_dialog.h"
#include "haiku_save_file_dialog.h"
#include "haiku_open_folder_dialog.h"
#include <iostream>
#include <Screen.h>

HaikuDisplayBackend::HaikuDisplayBackend()
{
	if (!be_app) {
		new ZApplication();
	}
}

std::unique_ptr<DisplayWindow> HaikuDisplayBackend::Create(DisplayWindowHost* windowHost, WidgetType type, DisplayWindow* owner, RenderAPI renderAPI)
{
	if (!be_app) {
		new ZApplication();
	}
	return std::make_unique<HaikuDisplayWindow>(windowHost, type, static_cast<HaikuDisplayWindow*>(owner), renderAPI);
}

void HaikuDisplayBackend::ProcessEvents()
{
	snooze(10000); // 10ms to avoid 100% CPU during modal loops
}

void HaikuDisplayBackend::RunLoop()
{
	if (!be_app) {
		new ZApplication();
	}
	be_app->Run();
}

void HaikuDisplayBackend::ExitLoop()
{
	if (be_app) {
		if (be_app->Lock()) {
		be_app->PostMessage(B_QUIT_REQUESTED);
			be_app->Unlock();
		}
	}
}

void* HaikuDisplayBackend::StartTimer(int timeoutMilliseconds, std::function<void()> onTimer)
{
	ZApplication* zapp = dynamic_cast<ZApplication*>(be_app);
	if (zapp) {
		return zapp->StartTimer(timeoutMilliseconds, std::move(onTimer));
	}
	return nullptr; 
}

void HaikuDisplayBackend::StopTimer(void* timerID)
{
	ZApplication* zapp = dynamic_cast<ZApplication*>(be_app);
	if (zapp) {
		zapp->StopTimer(timerID);
	}
}

Size HaikuDisplayBackend::GetScreenSize()
{
	BScreen screen;
	BRect bounds = screen.Frame();
	return Size(bounds.Width() + 1, bounds.Height() + 1);
}

std::unique_ptr<OpenFileDialog> HaikuDisplayBackend::CreateOpenFileDialog(DisplayWindow* owner)
{
	return std::make_unique<HaikuOpenFileDialog>(owner);
}

std::unique_ptr<SaveFileDialog> HaikuDisplayBackend::CreateSaveFileDialog(DisplayWindow* owner)
{
	return std::make_unique<HaikuSaveFileDialog>(owner);
}

std::unique_ptr<OpenFolderDialog> HaikuDisplayBackend::CreateOpenFolderDialog(DisplayWindow* owner)
{
	return std::make_unique<HaikuOpenFolderDialog>(owner);
}
