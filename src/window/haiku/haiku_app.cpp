#include "haiku_app.h"
#include <iostream>

ZApplication::ZApplication() : BApplication("application/x-vnd.zwidget-example") {}

void ZApplication::ReadyToRun()
{
}

void ZApplication::MessageReceived(BMessage* message) {
    if (message->what == ZWIDGET_TIMER) {
        void* id;
        if (message->FindPointer("id", &id) == B_OK) {
            auto it = timers.find(id);
            if (it != timers.end()) {
                it->second();
            }
        }
    } else {
        BApplication::MessageReceived(message);
    }
}

void* ZApplication::StartTimer(int timeoutMilliseconds, std::function<void()> onTimer)
{
    void* id = (void*)(uintptr_t)rand(); // Simple unique ID generation
    timers[id] = onTimer;

    BMessageRunner* runner = new BMessageRunner(BMessenger(this), new BMessage(ZWIDGET_TIMER), timeoutMilliseconds * 1000, -1);

    messageRunners[id] = runner;
    return id;
}

void ZApplication::StopTimer(void* timerID)
{
    auto itRunner = messageRunners.find(timerID);
    if (itRunner != messageRunners.end())
    {
        delete itRunner->second;
        messageRunners.erase(itRunner);
    }

    auto itTimer = timers.find(timerID);
    if (itTimer != timers.end())
    {
        timers.erase(itTimer);
    }
}
