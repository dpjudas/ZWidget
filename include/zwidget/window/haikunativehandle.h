#pragma once

class BWindow;
class BView;

struct HaikuNativeHandle
{
	BWindow* window = nullptr;
	BView* view = nullptr;
};
