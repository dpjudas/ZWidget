#pragma once

#include <Application.h>
#include <Handler.h>
#include <MessageRunner.h>
#include <Looper.h>
#include <iostream>
#include <map>
#include <functional>

const uint32_t ZWIDGET_TIMER = 'zwgt';

class ZApplication : public BApplication {
public:
    ZApplication();
    void ReadyToRun() override;

    void MessageReceived(BMessage* message) override;

    void* StartTimer(int timeoutMilliseconds, std::function<void()> onTimer);
    void StopTimer(void* timerID);

private:
    std::map<void*, std::function<void()>> timers;
    std::map<void*, BMessageRunner*> messageRunners;
};
