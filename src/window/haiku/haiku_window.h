#pragma once

#include <Window.h>
#include <View.h>
#include <Application.h>
#include <GLView.h>

class HaikuDisplayWindow;

class ZWindow : public BWindow 
{
public:
	ZWindow(BRect frame, const char* title, HaikuDisplayWindow* parent, window_look look = B_TITLED_WINDOW_LOOK, window_feel feel = B_NORMAL_WINDOW_FEEL, uint32 flags = B_ASYNCHRONOUS_CONTROLS, bool use_opengl = false);

	void MessageReceived(BMessage* message) override;
	void FrameResized(float newWidth, float newHeight) override;
	bool QuitRequested() override;

    void CaptureMouse();
    void ReleaseMouseCapture();

    BGLView* gl_view = nullptr;

private:
	HaikuDisplayWindow* parent;
	BView* view;
};
