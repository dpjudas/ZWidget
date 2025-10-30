#include "cocoa_display_window.h"
#include <stdio.h>
#include <vector> // Required for std::vector
#include <map>
#include <dlfcn.h>
#include <cmath>
#ifdef HAVE_VULKAN
#define VK_USE_PLATFORM_METAL_EXT
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_metal.h>
#endif
#include <zwidget/core/image.h>
#include "zwidget/window/cocoanativehandle.h"
#ifdef HAVE_METAL
#endif

#import "AppKitWrapper.h"
#import <Cocoa/Cocoa.h>

#ifdef HAVE_METAL
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#endif

#ifdef HAVE_OPENGL
#import <OpenGL/gl.h>
#import <OpenGL/glu.h>

#define GL_SILENCE_DEPRECATION
#endif

// Forward declarations
@class ZWidgetView;

@class ZWidgetWindowDelegate;

InputKey keycode_to_inputkey(unsigned short keycode)
{
    static const std::map<unsigned short, InputKey> keymap =
    {
        {0, InputKey::A},
        {1, InputKey::S},
        {2, InputKey::D},
        {3, InputKey::F},
        {4, InputKey::H},
        {5, InputKey::G},
        {6, InputKey::Z},
        {7, InputKey::X},
        {8, InputKey::C},
        {9, InputKey::V},
        {11, InputKey::B},
        {12, InputKey::Q},
        {13, InputKey::W},
        {14, InputKey::E},
        {15, InputKey::R},
        {16, InputKey::Y},
        {17, InputKey::T},
        {36, InputKey::Enter},
        {48, InputKey::Tab},
        {49, InputKey::Space},
        {51, InputKey::Backspace},
        {53, InputKey::Escape},
        {55, InputKey::LCommand}, // Command key
        {56, InputKey::LShift},
        {57, InputKey::CapsLock},
        {58, InputKey::LAlt}, // Left Alt (Option)
        {59, InputKey::LControl},
        {60, InputKey::RShift},
        {61, InputKey::RAlt}, // Right Alt (Option)
        {62, InputKey::RControl},
        {115, InputKey::Home},
        {116, InputKey::PageUp},
        {117, InputKey::Delete},
        {119, InputKey::End},
        {121, InputKey::PageDown},
        {122, InputKey::F1},
        {120, InputKey::F2},
        {99, InputKey::F3},
        {118, InputKey::F4},
        {96, InputKey::F5},
        {97, InputKey::F6},
        {98, InputKey::F7},
        {100, InputKey::F8},
        {101, InputKey::F9},
        {109, InputKey::F10},
        {103, InputKey::F11},
        {111, InputKey::F12},
        {123, InputKey::Left},
        {124, InputKey::Right},
        {125, InputKey::Down},
        {126, InputKey::Up}
    };

    auto it = keymap.find(keycode);
    if (it != keymap.end())
    {
        return it->second;
    }
    return InputKey::None;
}

struct CocoaDisplayWindowImpl
{
public:
    DisplayWindowHost* windowHost = nullptr;
    NSWindow* window = nil;
    ZWidgetWindowDelegate* delegate = nil;
    NSBitmapImageRep* bitmapRep = nil;
    CGImageRef cgImage = nullptr;
    std::map<InputKey, bool> keyState;
    bool mouseCaptured = false;
    RenderAPI renderAPI = RenderAPI::Unspecified;

    CVDisplayLinkRef displayLink = nullptr;

    CGColorSpaceRef colorSpace = nullptr;
    CGContextRef bitmapContext = nullptr;
    std::vector<uint32_t> pixelBuffer;
    int bitmapWidth = 0;
    int bitmapHeight = 0;

#ifdef HAVE_METAL
    id<MTLDevice> metalDevice = nil;
    CAMetalLayer* metalLayer = nil;
#endif

#ifdef HAVE_OPENGL
    NSOpenGLContext* openglContext = nil;
#endif

    // Declare methods, but implement them outside the struct
    void initOpenGL(ZWidgetView* view);
#ifdef HAVE_METAL
    void updateDrawableSize(CGSize size);
#endif

    CVReturn displayLinkOutputCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* inNow, const CVTimeStamp* inOutputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext);
    void startDisplayLink();
    void stopDisplayLink();

    ~CocoaDisplayWindowImpl()
    {
        if (bitmapContext) CGContextRelease(bitmapContext);
        if (colorSpace) CGColorSpaceRelease(colorSpace);
        if (cgImage) CGImageRelease(cgImage);
    }
};

static CVReturn DisplayLinkOutputCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* inNow, const CVTimeStamp* inOutputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
    CocoaDisplayWindowImpl* impl = (CocoaDisplayWindowImpl*)displayLinkContext;
    return impl->displayLinkOutputCallback(displayLink, inNow, inOutputTime, flagsIn, flagsOut, displayLinkContext);
}

void CocoaDisplayWindowImpl::startDisplayLink()
{
    if (displayLink) return;

    CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
    CVDisplayLinkSetOutputCallback(displayLink, &DisplayLinkOutputCallback, this);
    CVDisplayLinkStart(displayLink);
}

void CocoaDisplayWindowImpl::stopDisplayLink()
{
    if (!displayLink) return;

    CVDisplayLinkStop(displayLink);
    CVDisplayLinkRelease(displayLink);
    displayLink = nullptr;
}

@interface ZWidgetView : NSView
{
    CocoaDisplayWindowImpl* impl;
}
- (id)initWithImpl:(CocoaDisplayWindowImpl*)impl;
@end

@implementation ZWidgetView

+ (Class)layerClass
{
    return [CAMetalLayer class];
}

- (id)initWithImpl:(CocoaDisplayWindowImpl*)d
{
    self = [super init];
    if (self)
    {
        impl = d;
        self.wantsLayer = YES;
        self.layer.contentsScale = [NSScreen mainScreen].backingScaleFactor;
        [self addTrackingArea:[[NSTrackingArea alloc] initWithRect:self.bounds options:NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingActiveInKeyWindow owner:self userInfo:nil]];
    }
    return self;
}

- (BOOL)isOpaque
{
    return YES;
}

- (CALayer *)makeBackingLayer
{
    return [[self.class layerClass] layer];
}
- (BOOL)canBecomeKeyView
{
    return YES;
}

- (void)setFrame:(NSRect)frame
{
    [super setFrame:frame];
    impl->updateDrawableSize(frame.size);
}
- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (void)drawRect:(NSRect)dirtyRect
{
    NSLog(@"ZWidgetView: drawRect called");
    if (!impl) return;

    if (impl->renderAPI == RenderAPI::Bitmap)
    {
        if (impl->cgImage)
        {
            NSLog(@"drawRect: impl->cgImage is NOT null. Drawing image.");
            CGContextRef context = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
            CGContextSaveGState(context);
            CGContextTranslateCTM(context, 0, self.bounds.size.height);
            CGContextScaleCTM(context, 1.0, -1.0);
            CGContextDrawImage(context, NSRectToCGRect(self.bounds), impl->cgImage);
            CGContextRestoreGState(context);
        }
        else
        {
            NSLog(@"drawRect: impl->cgImage IS null. Not drawing image.");
        }
    }
    else if (impl->renderAPI == RenderAPI::OpenGL)
    {
#ifdef HAVE_OPENGL
        if (impl->openglContext)
        {
            [impl->openglContext makeCurrentContext];
            [impl->openglContext flushBuffer];
        }
#endif
    }
    else if (impl->renderAPI == RenderAPI::Metal)
    {
#ifdef HAVE_METAL
        // Metal rendering is handled by the CVDisplayLink callback
        // No need to do anything here directly
#endif
    }
}

- (void)viewDidMoveToWindow
{
    if ([self window])
    {
        NSLog(@"ZWidgetView: viewDidMoveToWindow with windowHost: %p", (void*)impl->windowHost);
        impl->windowHost->OnWindowPaint();
        impl->startDisplayLink();
    }
}

- (void)viewWillMoveToWindow:(NSWindow *)newWindow
{
    if (!newWindow)
    {
        impl->stopDisplayLink();
    }
}

- (void)mouseEntered:(NSEvent *)theEvent
{
    // Not used yet
}

- (void)mouseExited:(NSEvent *)theEvent
{
    if (impl && impl->windowHost)
        impl->windowHost->OnWindowMouseLeave();
}

- (void)mouseMoved:(NSEvent *)theEvent
{
    if (impl && impl->windowHost)
    {
        if (impl->mouseCaptured)
        {
            impl->windowHost->OnWindowRawMouseMove([theEvent deltaX], [theEvent deltaY]);
        }
        else
        {
            NSPoint p = [theEvent locationInWindow];
            impl->windowHost->OnWindowMouseMove(Point(p.x, [self frame].size.height - p.y));
        }
    }
}

- (void)mouseDown:(NSEvent *)theEvent
{
    if (impl && impl->windowHost)
    {
        NSPoint p = [theEvent locationInWindow];
        InputKey mouseKey = InputKey::None;
        if ([theEvent buttonNumber] == 0) mouseKey = InputKey::LeftMouse;
        else if ([theEvent buttonNumber] == 1) mouseKey = InputKey::RightMouse;
        else if ([theEvent buttonNumber] == 2) mouseKey = InputKey::MiddleMouse;

        if (mouseKey != InputKey::None)
        {
            impl->windowHost->OnWindowMouseDown(Point(p.x, [self frame].size.height - p.y), mouseKey);
        }
    }
}

- (void)mouseUp:(NSEvent *)theEvent
{
    if (impl && impl->windowHost)
    {
        NSPoint p = [theEvent locationInWindow];
        InputKey mouseKey = InputKey::None;
        if ([theEvent buttonNumber] == 0) mouseKey = InputKey::LeftMouse;
        else if ([theEvent buttonNumber] == 1) mouseKey = InputKey::RightMouse;
        else if ([theEvent buttonNumber] == 2) mouseKey = InputKey::MiddleMouse;

        if (mouseKey != InputKey::None)
        {
            impl->windowHost->OnWindowMouseUp(Point(p.x, [self frame].size.height - p.y), mouseKey);
        }
    }
}

- (void)rightMouseDown:(NSEvent *)theEvent
{
    if (impl && impl->windowHost)
    {
        NSPoint p = [theEvent locationInWindow];
        impl->windowHost->OnWindowMouseDown(Point(p.x, [self frame].size.height - p.y), InputKey::RightMouse);
    }
}

- (void)rightMouseUp:(NSEvent *)theEvent
{
    if (impl && impl->windowHost)
    {
        NSPoint p = [theEvent locationInWindow];
        impl->windowHost->OnWindowMouseUp(Point(p.x, [self frame].size.height - p.y), InputKey::RightMouse);
    }
}

- (void)mouseDragged:(NSEvent *)theEvent
{
    if (impl && impl->windowHost)
    {
        NSPoint p = [theEvent locationInWindow];
        impl->windowHost->OnWindowMouseMove(Point(p.x, [self frame].size.height - p.y));
    }

}

- (void)rightMouseDragged:(NSEvent *)theEvent
{
    if (impl && impl->windowHost)
    {
        NSPoint p = [theEvent locationInWindow];
        impl->windowHost->OnWindowMouseMove(Point(p.x, [self frame].size.height - p.y));
    }

}

- (void)scrollWheel:(NSEvent *)theEvent
{
    if (impl && impl->windowHost)
    {
        NSPoint p = [theEvent locationInWindow];
        if ([theEvent deltaY] > 0)
            impl->windowHost->OnWindowMouseWheel(Point(p.x, [self frame].size.height - p.y), InputKey::MouseWheelUp);
        else if ([theEvent deltaY] < 0)
            impl->windowHost->OnWindowMouseWheel(Point(p.x, [self frame].size.height - p.y), InputKey::MouseWheelDown);
    }

}

- (void)keyDown:(NSEvent *)theEvent
{
    if (impl && impl->windowHost)
    {
        InputKey key = keycode_to_inputkey([theEvent keyCode]);
        impl->keyState[key] = true;
        impl->windowHost->OnWindowKeyDown(key); // Removed isARepeat as it's not in the ZWidget API
        
        NSString* characters = [theEvent characters];
        if ([characters length] > 0)
        {
            impl->windowHost->OnWindowKeyChar([characters UTF8String]);
        }
    }

}

- (void)keyUp:(NSEvent *)theEvent
{
    if (impl && impl->windowHost)
    {
        InputKey key = keycode_to_inputkey([theEvent keyCode]);
        impl->keyState[key] = false;
        impl->windowHost->OnWindowKeyUp(key);
    }

}

- (void)flagsChanged:(NSEvent *)theEvent
{
    if (impl && impl->windowHost)
    {
        bool shiftPressed = ([theEvent modifierFlags] & NSEventModifierFlagShift) != 0;
        bool ctrlPressed = ([theEvent modifierFlags] & NSEventModifierFlagControl) != 0;
        bool altPressed = ([theEvent modifierFlags] & NSEventModifierFlagOption) != 0;
        bool commandPressed = ([theEvent modifierFlags] & NSEventModifierFlagCommand) != 0;

        // Update Shift keys
        if (impl->keyState[InputKey::LShift] != shiftPressed)
        {
            impl->keyState[InputKey::LShift] = shiftPressed;
            if (shiftPressed) impl->windowHost->OnWindowKeyDown(InputKey::LShift);
            else impl->windowHost->OnWindowKeyUp(InputKey::LShift);
        }
        if (impl->keyState[InputKey::RShift] != shiftPressed) // Also update RShift
        {
            impl->keyState[InputKey::RShift] = shiftPressed;
            if (shiftPressed) impl->windowHost->OnWindowKeyDown(InputKey::RShift);
            else impl->windowHost->OnWindowKeyUp(InputKey::RShift);
        }

        // Update Control keys
        if (impl->keyState[InputKey::LControl] != ctrlPressed)
        {
            impl->keyState[InputKey::LControl] = ctrlPressed;
            if (ctrlPressed) impl->windowHost->OnWindowKeyDown(InputKey::LControl);
            else impl->windowHost->OnWindowKeyUp(InputKey::LControl);
        }
        if (impl->keyState[InputKey::RControl] != ctrlPressed) // Also update RControl
        {
            impl->keyState[InputKey::RControl] = ctrlPressed;
            if (ctrlPressed) impl->windowHost->OnWindowKeyDown(InputKey::RControl);
            else impl->windowHost->OnWindowKeyUp(InputKey::RControl);
        }

        // Update Alt keys
        if (impl->keyState[InputKey::LAlt] != altPressed)
        {
            impl->keyState[InputKey::LAlt] = altPressed;
            if (altPressed) impl->windowHost->OnWindowKeyDown(InputKey::LAlt);
            else impl->windowHost->OnWindowKeyUp(InputKey::LAlt);
        }
        if (impl->keyState[InputKey::RAlt] != altPressed) // Also update RAlt
        {
            impl->keyState[InputKey::RAlt] = altPressed;
            if (altPressed) impl->windowHost->OnWindowKeyDown(InputKey::RAlt);
            else impl->keyState[InputKey::RAlt] = false;
        }

        // Update Command key (mapped to LCommand)
        if (impl->keyState[InputKey::LCommand] != commandPressed)
        {
            impl->keyState[InputKey::LCommand] = commandPressed;
            if (commandPressed) impl->windowHost->OnWindowKeyDown(InputKey::LCommand);
            else impl->windowHost->OnWindowKeyUp(InputKey::LCommand);
        }
    }

}
@end

// ZWidgetWindowDelegate interface and implementation
@interface ZWidgetWindowDelegate : NSObject <NSWindowDelegate>
{
    CocoaDisplayWindowImpl* impl;
}
- (id)initWithImpl:(CocoaDisplayWindowImpl*)impl;
@end

@implementation ZWidgetWindowDelegate
- (id)initWithImpl:(CocoaDisplayWindowImpl*)d
{
    self = [super init];
    if (self)
    {
        impl = d;
    }
    return self;
}
@end

void CocoaDisplayWindowImpl::initOpenGL(ZWidgetView* view)
{
#ifdef HAVE_OPENGL
    NSOpenGLPixelFormatAttribute attrs[] = { NSOpenGLPFAAccelerated, NSOpenGLPFANoRecovery, NSOpenGLPFADoubleBuffer, NSOpenGLPFAColorSize, 24, NSOpenGLPFAAlphaSize, 8, NSOpenGLPFADepthSize, 24, 0 };
    NSOpenGLPixelFormat* pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    openglContext = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
    if (openglContext)
    {
        renderAPI = RenderAPI::OpenGL;
        [openglContext setView:(NSView*)view];
    }
    else
    {
        renderAPI = RenderAPI::Bitmap; // Fallback if OpenGL not available
    }
#else
    renderAPI = RenderAPI::Bitmap; // Fallback if OpenGL not available
#endif
}

#ifdef HAVE_METAL
void CocoaDisplayWindowImpl::updateDrawableSize(CGSize size)
{
    if (metalLayer)
    {
        CGFloat scale = [NSScreen mainScreen].backingScaleFactor;
        metalLayer.drawableSize = CGSizeMake(size.width * scale, size.height * scale);
    }
}
#endif

CVReturn CocoaDisplayWindowImpl::displayLinkOutputCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* inNow, const CVTimeStamp* inOutputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
    NSLog(@"displayLinkOutputCallback called");
    if (renderAPI == RenderAPI::Metal)
    {
#ifdef HAVE_METAL
        if (windowHost)
        {
            windowHost->OnWindowPaint();
        }
#endif
    }
    else if (renderAPI == RenderAPI::OpenGL)
    {
#ifdef HAVE_OPENGL
        if (openglContext && windowHost)
        {
            [openglContext makeCurrentContext];
            windowHost->OnWindowPaint();
            [openglContext flushBuffer];
        }
#endif
    }
    else if (renderAPI == RenderAPI::Bitmap)
    {
        NSLog(@"displayLinkOutputCallback: RenderAPI::Bitmap branch executed. Dispatching OnWindowPaint() to main thread.");
        dispatch_async(dispatch_get_main_queue(), ^{
            NSLog(@"dispatch_async block executed. Calling OnWindowPaint().");
            if (windowHost) windowHost->OnWindowPaint();
        });
    }
    return kCVReturnSuccess;
}

CocoaDisplayWindow::CocoaDisplayWindow(DisplayWindowHost* windowHost, bool popupWindow, DisplayWindow* owner, RenderAPI renderAPI)
{
	impl = std::make_unique<CocoaDisplayWindowImpl>();
	impl->windowHost = windowHost;
	impl->renderAPI = renderAPI;

	NSRect contentRect = NSMakeRect(0, 0, 640, 480);
	NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;
	if (popupWindow)
		style = NSWindowStyleMaskBorderless;

	impl->window = [[NSWindow alloc] initWithContentRect:contentRect styleMask:style backing:NSBackingStoreBuffered defer:NO];
	impl->delegate = [[ZWidgetWindowDelegate alloc] initWithImpl:impl.get()];
	[impl->window setDelegate:impl->delegate];
	[impl->window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
	[impl->window setAcceptsMouseMovedEvents:YES];
	[impl->window setRestorable:NO];
	[impl->window setReleasedWhenClosed:NO];

	ZWidgetView* view = [[ZWidgetView alloc] initWithImpl:impl.get()];
	[impl->window setContentView:view];

	if (owner)
	{
		CocoaDisplayWindow* cocoaOwner = static_cast<CocoaDisplayWindow*>(owner);
		[impl->window setParentWindow:cocoaOwner->impl->window];
	}

#ifdef HAVE_METAL
	// Create Metal device and layer for application rendering (not ZWidget rendering)
	// Applications can access these via GetMetalDevice() and GetMetalLayer()
	impl->metalDevice = MTLCreateSystemDefaultDevice();
	if (impl->metalDevice)
	{
		impl->metalLayer = [CAMetalLayer layer];
		impl->metalLayer.device = impl->metalDevice;
		impl->metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
		impl->metalLayer.framebufferOnly = NO;  // Allow reading for screenshots, etc.
		impl->metalLayer.presentsWithTransaction = NO;
		impl->metalLayer.displaySyncEnabled = YES;
		impl->metalLayer.maximumDrawableCount = 3;
		impl->metalLayer.autoresizingMask = kCALayerWidthSizable | kCALayerHeightSizable;
		impl->metalLayer.contentsScale = [[NSScreen mainScreen] backingScaleFactor];
		impl->metalLayer.frame = view.layer.frame;
		[view.layer addSublayer:impl->metalLayer];
	}
#endif
#ifdef HAVE_OPENGL
	if (renderAPI == RenderAPI::OpenGL)
	{
		impl->initOpenGL(view);
	}
	else
#endif
	{
		impl->renderAPI = RenderAPI::Bitmap;
	}
}

CocoaDisplayWindow::~CocoaDisplayWindow()
{
    NSLog(@"CocoaDisplayWindow: Destructor entered, this = %p", (void*)this);
}

void CocoaDisplayWindow::SetWindowTitle(const std::string& text)
{
    if (impl->window)
    {
        [impl->window setTitle:[NSString stringWithUTF8String:text.c_str()]];
    }
}

void CocoaDisplayWindow::SetWindowIcon(const std::vector<std::shared_ptr<Image>>& images)
{
    if (impl->window && !images.empty())
    {
	// For simplicity, use the first image as the icon.
        // A more robust implementation might choose an appropriate size.
        std::shared_ptr<Image> iconImage = images[0];

        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGContextRef context = CGBitmapContextCreate(
            iconImage->GetData(), iconImage->GetWidth(), iconImage->GetHeight(), 8, iconImage->GetWidth() * 4, colorSpace,
            kCGImageAlphaPremultipliedLast | (CGBitmapInfo)kCGBitmapByteOrder32Big);

        if (context)
        {
            CGImageRef cgImage = CGBitmapContextCreateImage(context);
            if (cgImage)
            {
                NSImage* nsImage = [[NSImage alloc] initWithCGImage:cgImage size:NSZeroSize];
                [NSApp setApplicationIconImage:nsImage];
                CGImageRelease(cgImage);
            }
            CGContextRelease(context);
        }
        CGColorSpaceRelease(colorSpace);
    }
}

void CocoaDisplayWindow::SetWindowFrame(const Rect& box)
{
    if (impl->window)
    {
        NSRect frame = NSMakeRect(box.x, [[NSScreen mainScreen] frame].size.height - box.y - box.height, box.width, box.height);
        [impl->window setFrame:frame display:YES animate:NO];
    }
}

void CocoaDisplayWindow::SetClientFrame(const Rect& box)
{
    if (impl->window)
    {
        NSRect contentRect = NSMakeRect(box.x, [[NSScreen mainScreen] frame].size.height - box.y - box.height, box.width, box.height);
        [impl->window setContentSize:contentRect.size];
        [impl->window setFrameOrigin:contentRect.origin];
    }
}

void CocoaDisplayWindow::Show()
{
    if (impl->window)
    {
        [impl->window makeKeyAndOrderFront:nil];
		Update();
    }
}

void CocoaDisplayWindow::ShowFullscreen()
{
    if (impl->window)
    {
        [impl->window toggleFullScreen:nil];
    }
}

void CocoaDisplayWindow::ShowMaximized()
{
    if (impl->window)
    {
        [impl->window zoom:nil];
    }
}

void CocoaDisplayWindow::ShowMinimized()
{
    if (impl->window)
    {
        [impl->window miniaturize:nil];
    }
}

void CocoaDisplayWindow::ShowNormal()
{
    if (impl->window)
    {
        if ([impl->window isMiniaturized])
        {
            [impl->window deminiaturize:nil];
        }
        if ([impl->window isZoomed])
        {
            [impl->window zoom:nil];
        }
    }
}

bool CocoaDisplayWindow::IsWindowFullscreen()
{
    if (impl->window)
    {
        return ([impl->window styleMask] & NSWindowStyleMaskFullScreen) == NSWindowStyleMaskFullScreen;
    }
    return false;
}

void CocoaDisplayWindow::Hide()
{
    if (impl->window)
    {
        [impl->window orderOut:nil];
    }
}

void CocoaDisplayWindow::Activate()
{
    if (impl->window)
    {
        [impl->window makeKeyAndOrderFront:nil];
        [NSApp activateIgnoringOtherApps:YES];
    }
}

void CocoaDisplayWindow::ShowCursor(bool enable)
{
    if (enable)
    {
        [NSCursor unhide];
    }
    else
    {
        [NSCursor hide];
    }
}

// No-ops as MacOS doesn't allow this
void CocoaDisplayWindow::LockKeyboard() {}

void CocoaDisplayWindow::UnlockKeyboard() {}

void CocoaDisplayWindow::LockCursor()
{
    if (impl->window)
    {
        CGAssociateMouseAndMouseCursorPosition(false);
        // Hide cursor when locked
        [NSCursor hide];
    }
}

void CocoaDisplayWindow::UnlockCursor()
{
    if (impl->window)
    {
        CGAssociateMouseAndMouseCursorPosition(true);
        // Show cursor when unlocked
        [NSCursor unhide];
    }
}

void CocoaDisplayWindow::CaptureMouse()
{
    impl->mouseCaptured = true;
}

void CocoaDisplayWindow::ReleaseMouseCapture()
{
    impl->mouseCaptured = false;
}

void CocoaDisplayWindow::Update()
{
    if (impl->window)
    {
        [[impl->window contentView] setNeedsDisplay:YES];
    }
}

bool CocoaDisplayWindow::GetKeyState(InputKey key)
{
    auto it = impl->keyState.find(key);
    if (it != impl->keyState.end())
    {
        return it->second;
    }
    return false;
}

void CocoaDisplayWindow::SetCursor(StandardCursor cursor, std::shared_ptr<CustomCursor> custom)
{
    NSCursor* nsCursor = nil;

    if (custom && !custom->GetFrames().empty())
    {
        // Create custom cursor from image
        const auto& frame = custom->GetFrames()[0];  // For now, use first frame (TODO: animated cursors)
        auto image = frame.FrameImage;
        Point hotspot = custom->GetHotspot();

        // Convert Image to NSImage
        int width = image->GetWidth();
        int height = image->GetHeight();
        const uint32_t* pixels = (const uint32_t*)image->GetData();

        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGContextRef context = CGBitmapContextCreate(
            (void*)pixels, width, height, 8, width * 4, colorSpace,
            kCGImageAlphaPremultipliedLast | (CGBitmapInfo)kCGBitmapByteOrder32Big);

        if (context)
        {
            CGImageRef cgImage = CGBitmapContextCreateImage(context);
            if (cgImage)
            {
                NSImage* nsImage = [[NSImage alloc] initWithCGImage:cgImage size:NSMakeSize(width, height)];
                NSPoint nsHotspot = NSMakePoint(hotspot.x, hotspot.y);
                nsCursor = [[NSCursor alloc] initWithImage:nsImage hotSpot:nsHotspot];
                CGImageRelease(cgImage);
            }
            CGContextRelease(context);
        }
        CGColorSpaceRelease(colorSpace);
    }
    else
    {
        // Standard cursors
        switch (cursor)
        {
            case StandardCursor::arrow: nsCursor = [NSCursor arrowCursor]; break;
            case StandardCursor::ibeam: nsCursor = [NSCursor IBeamCursor]; break;
            case StandardCursor::wait: nsCursor = [NSCursor operationNotAllowedCursor]; break;
            case StandardCursor::cross: nsCursor = [NSCursor crosshairCursor]; break;
            case StandardCursor::size_nwse: nsCursor = [NSCursor resizeUpCursor]; break;
            case StandardCursor::size_nesw: nsCursor = [NSCursor resizeDownCursor]; break;
            case StandardCursor::size_we: nsCursor = [NSCursor resizeLeftRightCursor]; break;
            case StandardCursor::size_ns: nsCursor = [NSCursor resizeUpDownCursor]; break;
            case StandardCursor::size_all: nsCursor = [NSCursor openHandCursor]; break;
            case StandardCursor::no: nsCursor = [NSCursor operationNotAllowedCursor]; break;
            case StandardCursor::hand: nsCursor = [NSCursor pointingHandCursor]; break;
            default: nsCursor = [NSCursor arrowCursor]; break;
        }
    }

    if (nsCursor)
    {
        [nsCursor set];
    }
}

Rect CocoaDisplayWindow::GetWindowFrame() const
{
    if (impl->window)
    {
        NSRect frame = [impl->window frame];
        return Rect(frame.origin.x, [[NSScreen mainScreen] frame].size.height - frame.origin.y - frame.size.height, frame.size.width, frame.size.height);
    }
    return {};
}

Size CocoaDisplayWindow::GetClientSize() const
{
    if (impl->window)
    {
        NSRect contentRect = [[impl->window contentView] frame];
        return Size(contentRect.size.width, contentRect.size.height);
    }
    return {};
}

int CocoaDisplayWindow::GetPixelWidth() const
{
    if (impl->window)
    {
        NSRect contentRect = [[impl->window contentView] frame];
        CGFloat scale = [impl->window backingScaleFactor];
        if (scale <= 0.0) scale = 1.0;
        return (int)std::round(contentRect.size.width * scale);
    }
    return 0;
}

int CocoaDisplayWindow::GetPixelHeight() const
{
    if (impl->window)
    {
        NSRect contentRect = [[impl->window contentView] frame];
        CGFloat scale = [impl->window backingScaleFactor];
        if (scale <= 0.0) scale = 1.0;
        return (int)std::round(contentRect.size.height * scale);
    }
    return 0;
}

double CocoaDisplayWindow::GetDpiScale() const
{
    if (impl->window)
    {
        CGFloat scale = [impl->window backingScaleFactor];
        if (scale <= 0.0) scale = 1.0;
        return scale;
    }
    return 1.0;
}

Point CocoaDisplayWindow::MapFromGlobal(const Point& pos) const
{
    if (impl->window)
    {
        NSPoint globalPoint = NSMakePoint(pos.x, [[NSScreen mainScreen] frame].size.height - pos.y);
        NSPoint windowPoint = [impl->window convertPointFromScreen:globalPoint];
        NSPoint viewPoint = [[impl->window contentView] convertPoint:windowPoint fromView:nil];
        return Point(viewPoint.x, [[impl->window contentView] frame].size.height - viewPoint.y);
    }
    return {};
}

Point CocoaDisplayWindow::MapToGlobal(const Point& pos) const
{
    if (impl->window)
    {
        NSPoint viewPoint = NSMakePoint(pos.x, [[impl->window contentView] frame].size.height - pos.y);
        NSPoint windowPoint = [[impl->window contentView] convertPoint:viewPoint toView:nil];
        NSPoint globalPoint = [impl->window convertPointToScreen:windowPoint];
        return Point(globalPoint.x, [[NSScreen mainScreen] frame].size.height - globalPoint.y);
    }
    return {};
}

void CocoaDisplayWindow::SetBorderColor(uint32_t bgra8) {}
void CocoaDisplayWindow::SetCaptionColor(uint32_t bgra8) {}
void CocoaDisplayWindow::SetCaptionTextColor(uint32_t bgra8) {}

void CocoaDisplayWindow::PresentBitmap(int width, int height, const uint32_t* pixels)
{
    // ZWidget uses simple bitmap blitting for launcher UI
    // Metal layer is available for applications via GetMetalLayer()
    if (impl->renderAPI == RenderAPI::OpenGL)
    {
#ifdef HAVE_OPENGL
        if (impl->openglContext)
        {
            [impl->openglContext makeCurrentContext];
            glViewport(0, 0, width, height);
            glRasterPos2i(-1, -1);
            glDrawPixels(width, height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, pixels);
            // No flushBuffer here, it will be done by the display link callback
        }
#endif
    }
    else if (impl->renderAPI == RenderAPI::Bitmap)
    {
        NSLog(@"PresentBitmap: RenderAPI::Bitmap path executed.");
        if (impl->cgImage) CGImageRelease(impl->cgImage);
        impl->cgImage = nullptr;

        // Copy pixel data to persistent buffer since the input pointer is temporary
        size_t dataSize = width * height * 4;
        impl->pixelBuffer.resize(width * height);
        memcpy(impl->pixelBuffer.data(), pixels, dataSize);

        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGContextRef context = CGBitmapContextCreate(
            impl->pixelBuffer.data(), width, height, 8, width * 4, colorSpace,
            kCGImageAlphaPremultipliedFirst | (CGBitmapInfo)kCGBitmapByteOrder32Little);

        if (context)
        {
            impl->cgImage = CGBitmapContextCreateImage(context);
            if (impl->cgImage)
            {
                NSLog(@"PresentBitmap: CGImageRef created successfully.");
            }
            else
            {
                NSLog(@"PresentBitmap: Failed to create CGImageRef.");
            }
            CGContextRelease(context);
        }
        else
        {
            NSLog(@"PresentBitmap: Failed to create CGBitmapContext.");
        }
        CGColorSpaceRelease(colorSpace);

        if (impl->window)
        {
            dispatch_async(dispatch_get_main_queue(), ^{
                [[impl->window contentView] setNeedsDisplay:YES];
            });
        }
    }
}

std::string CocoaDisplayWindow::GetClipboardText()
{
    NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
    NSString* text = [pasteboard stringForType:NSPasteboardTypeString];
    if (text)
    {
        return [text UTF8String];
    }
    return {};
}

void CocoaDisplayWindow::SetClipboardText(const std::string& text)
{
    NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
    [pasteboard clearContents];
    [pasteboard setString:[NSString stringWithUTF8String:text.c_str()] forType:NSPasteboardTypeString];
}

void* CocoaDisplayWindow::GetNativeHandle()
{
    // Create and return a CocoaNativeHandle for this window
    CocoaNativeHandle* handle = new CocoaNativeHandle();
    if (impl->window)
    {
        handle->nsWindow = impl->window;
        handle->nsView = [impl->window contentView];
#ifdef HAVE_METAL
        handle->metalLayer = impl->metalLayer;
#endif
    }
    return handle;
}


std::vector<std::string> CocoaDisplayWindow::GetVulkanInstanceExtensions()
{
    std::vector<std::string> extensions;
#ifdef HAVE_VULKAN
    extensions.push_back("VK_KHR_surface");
    extensions.push_back("VK_EXT_metal_surface");
#endif
    return extensions;
}
VkSurfaceKHR CocoaDisplayWindow::CreateVulkanSurface(VkInstance instance)
{
    VkSurfaceKHR surface = nullptr;
#ifdef HAVE_VULKAN
    if (impl->window && impl->metalLayer)
    {
        // Dynamically load vkCreateMetalSurfaceEXT
        static PFN_vkCreateMetalSurfaceEXT vkCreateMetalSurfaceEXT = nullptr;
        if (!vkCreateMetalSurfaceEXT)
        {
            void* vulkanLib = dlopen("libvulkan.dylib", RTLD_NOW);
            if (vulkanLib)
            {
                vkCreateMetalSurfaceEXT = (PFN_vkCreateMetalSurfaceEXT)dlsym(vulkanLib, "vkCreateMetalSurfaceEXT");
            }
        }

        if (vkCreateMetalSurfaceEXT)
        {
            VkMetalSurfaceCreateInfoEXT surfaceInfo = {};
            surfaceInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
            surfaceInfo.pLayer = impl->metalLayer;

            VkResult err = vkCreateMetalSurfaceEXT(instance, &surfaceInfo, nullptr, &surface);
            if (err != VK_SUCCESS)
            {
                // Handle error
                fprintf(stderr, "Failed to create Vulkan Metal surface: %d\n", err);
            }
        }
    }
#endif
    return surface;
}

void* CocoaDisplayWindow::GetMetalDevice()
{
#ifdef HAVE_METAL
    // Return the Metal device for application rendering
    return (__bridge void*)impl->metalDevice;
#else
    return nullptr;
#endif
}

void* CocoaDisplayWindow::GetMetalLayer()
{
#ifdef HAVE_METAL
    // Return the CAMetalLayer for application rendering
    return (__bridge void*)impl->metalLayer;
#else
    return nullptr;
#endif
}
