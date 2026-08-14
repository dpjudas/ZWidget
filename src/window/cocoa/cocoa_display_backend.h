#pragma once

#include "zwidget/window/window.h"

class CocoaDisplayBackend : public DisplayBackend
{
public:
    CocoaDisplayBackend();
    ~CocoaDisplayBackend();

    std::unique_ptr<DisplayWindow> Create(DisplayWindowHost* windowHost, WidgetType type, DisplayWindow* owner, RenderAPI renderAPI) override;

    void ProcessEvents() override;
    void RunLoop() override;
    void RunModalLoop(DisplayWindow* modal) override;
    void ExitLoop() override;
    
    bool IsCocoa() override { return true; }

    void* StartTimer(int timeoutMilliseconds, std::function<void()> onTimer) override;
    void StopTimer(void* timerID) override;

    Size GetScreenSize() override;

    std::unique_ptr<OpenFileDialog> CreateOpenFileDialog(DisplayWindow* owner) override;
    std::unique_ptr<SaveFileDialog> CreateSaveFileDialog(DisplayWindow* owner) override;
    std::unique_ptr<OpenFolderDialog> CreateOpenFolderDialog(DisplayWindow* owner) override;

    static std::unique_ptr<DisplayBackend> TryCreateCocoa();

private:
    // RunLoop() has to cope with being called while the host application is
    // already inside [NSApp run] -- see the comment on RunLoop(). ExitRunLoop
    // ends the manual pump used in that case; StartedRunLoop records whether
    // this backend owns the run loop, so ExitLoop() only stops a loop it
    // started and never tears down the host's.
    bool ExitRunLoop = false;
    bool StartedRunLoop = false;
    // Set while inside -[NSApplication runModalForWindow:]. ExitLoop has three
    // cases now and they need different terminations: a modal session ends with
    // stopModal, a loop this backend started ends with stop:, and a manual pump
    // ends by flipping ExitRunLoop.
    bool InModalSession = false;
};