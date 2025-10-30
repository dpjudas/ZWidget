#pragma once

#ifdef __OBJC__
@class NSWindow;
@class NSView;
@class CAMetalLayer;
#else
class NSWindow;
class NSView;
class CAMetalLayer;
#endif


struct CocoaNativeHandle
{
	NSWindow* nsWindow = nullptr;
	NSView* nsView = nullptr;

	// For application-side rendering. Not used by ZWidget itself.
	CAMetalLayer* metalLayer = nullptr;
};