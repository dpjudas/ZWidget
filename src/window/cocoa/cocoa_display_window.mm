#
#include "cocoa_display_window.h"
#include <stdio.h>
#include <vector> // Required for std::vector
#include <map>
#include <dlfcn.h>
#include <zwidget/core/image.h>
#import "AppKitWrapper.h"

#ifdef HAVE_METAL
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#endif

#ifdef HAVE_OPENGL
#import <OpenGL/gl.h>
#import <OpenGL/glu.h>
#endif


@class ZWidgetWindowDelegate;

struct CocoaDisplayWindowImpl
{
public:
    DisplayWindowHost* windowHost = nullptr;
    NSWindow* window = nil;
    ZWidgetWindowDelegate* delegate = nil;
    NSBitmapImageRep* bitmapRep = nil;
    std::map<InputKey, bool> keyState;
    bool mouseCaptured = false;
    RenderAPI renderAPI = RenderAPI::Unspecified;

#ifdef HAVE_METAL
    id<MTLDevice> metalDevice = nil;
    id<MTLCommandQueue> commandQueue = nil;
    CAMetalLayer* metalLayer = nil;
#endif

#ifdef HAVE_OPENGL
    NSOpenGLContext* openglContext = nil;
#endif
};

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

- (BOOL)windowShouldClose:(id)sender
{
    if (impl && impl->windowHost)
    {
        impl->windowHost->OnWindowClose();
    }
    return NO;
}
@end

@interface ZWidgetView : NSView
{
    CocoaDisplayWindowImpl* impl;
}
- (id)initWithImpl:(CocoaDisplayWindowImpl*)impl;
@end

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
        {55, InputKey::LShift}, // Command
        {56, InputKey::LShift},
        {57, InputKey::CapsLock},
        {58, InputKey::Alt},
        {59, InputKey::LControl},
        {60, InputKey::RShift},
        {61, InputKey::Alt}, // Right Alt
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

@implementation ZWidgetView

#ifdef HAVE_METAL
+ (Class)layerClass
{
    return [CAMetalLayer class];
}
#endif

- (id)initWithImpl:(CocoaDisplayWindowImpl*)d
{
    self = [super init];
    if (self)
    {
        impl = d;
        [self addTrackingArea:[[NSTrackingArea alloc] initWithRect:self.bounds options:NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingActiveInKeyWindow owner:self userInfo:nil]];
    }
    return self;
}
- (BOOL)isOpaque
{
    return YES;
}
- (BOOL)canBecomeKeyView
{
    return YES;
}
- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (void)drawRect:(NSRect)dirtyRect
{
    if (impl && impl->bitmapRep)
    {
        [impl->bitmapRep drawInRect:self.bounds];
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
        impl->windowHost->OnWindowMouseDown(Point(p.x, [self frame].size.height - p.y), InputKey::LeftMouse);
    }
}

- (void)mouseUp:(NSEvent *)theEvent
{
    if (impl && impl->windowHost)
    {
        NSPoint p = [theEvent locationInWindow];
        impl->windowHost->OnWindowMouseUp(Point(p.x, [self frame].size.height - p.y), InputKey::LeftMouse);
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

        if (impl->keyState[InputKey::Shift] != shiftPressed)
        {
            impl->keyState[InputKey::Shift] = shiftPressed;
            if (shiftPressed)
                impl->windowHost->OnWindowKeyDown(InputKey::Shift);
            else
                impl->windowHost->OnWindowKeyUp(InputKey::Shift);
        }

        if (impl->keyState[InputKey::Ctrl] != ctrlPressed)
        {
            impl->keyState[InputKey::Ctrl] = ctrlPressed;
            if (ctrlPressed)
                impl->windowHost->OnWindowKeyDown(InputKey::Ctrl);
            else
                impl->windowHost->OnWindowKeyUp(InputKey::Ctrl);
        }

        if (impl->keyState[InputKey::Alt] != altPressed)
        {
            impl->keyState[InputKey::Alt] = altPressed;
            if (altPressed)
                impl->windowHost->OnWindowKeyDown(InputKey::Alt);
            else
                impl->windowHost->OnWindowKeyUp(InputKey::Alt);
        }
    }
}

@end

CocoaDisplayWindow::CocoaDisplayWindow(DisplayWindowHost* windowHost, bool popupWindow, DisplayWindow* owner, RenderAPI renderAPI) : impl(std::make_unique<CocoaDisplayWindowImpl>())
{
    impl->windowHost = windowHost;
    impl->renderAPI = renderAPI;

    NSRect frame = NSMakeRect(0, 0, 800, 600);
    NSUInteger styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
    impl->window = [[NSWindow alloc] initWithContentRect:frame styleMask:styleMask backing:NSBackingStoreBuffered defer:NO];
    [impl->window setTitle:@"ZWidget Window"];
    [impl->window setContentMinSize:NSMakeSize(200, 100)];

    ZWidgetView* view = [[ZWidgetView alloc] initWithImpl:impl.get()];
    [impl->window setContentView:view];
    // [view release]; // No release needed with ARC

    impl->delegate = [[ZWidgetWindowDelegate alloc] initWithImpl:impl.get()];
    [impl->window setDelegate:impl->delegate];

    [impl->window center];

    // Conditional rendering API initialization
    if (renderAPI == RenderAPI::Unspecified)
    {
#ifdef HAVE_METAL
        // Try to initialize Metal
        impl->metalDevice = MTLCreateSystemDefaultDevice();
        if (impl->metalDevice)
        {
            impl->renderAPI = RenderAPI::Metal;
            impl->commandQueue = [impl->metalDevice newCommandQueue];
            impl->metalLayer = (CAMetalLayer*)[view layer];
            impl->metalLayer.device = impl->metalDevice;
            impl->metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
            impl->metalLayer.framebufferOnly = YES;
        }
#endif
        if (impl->renderAPI == RenderAPI::Unspecified)
        {
#ifdef HAVE_OPENGL
                            // Try to initialize OpenGL
                            NSOpenGLPixelFormatAttribute attrs[] = { NSOpenGLPFAAccelerated, NSOpenGLPFANoRecovery, NSOpenGLPFADoubleBuffer, NSOpenGLPFAColorSize, 24, NSOpenGLPFAAlphaSize, 8, NSOpenGLPFADepthSize, 24, 0 };
                            NSOpenGLPixelFormat* pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
                            impl->openglContext = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
                            if (impl->openglContext)
                            {
                                impl->renderAPI = RenderAPI::OpenGL;
                                [impl->openglContext setView:view];
                            }
                            // [pixelFormat release]; // No release needed with ARC
#endif
        }
        if (impl->renderAPI == RenderAPI::Unspecified)
        {
            impl->renderAPI = RenderAPI::Bitmap; // Fallback to Bitmap
        }
    }
    else if (renderAPI == RenderAPI::Metal)
    {
#ifdef HAVE_METAL
        // Initialize Metal
        impl->metalDevice = MTLCreateSystemDefaultDevice();
        if (impl->metalDevice)
        {
            impl->commandQueue = [impl->metalDevice newCommandQueue];
            impl->metalLayer = (CAMetalLayer*)[view layer];
            impl->metalLayer.device = impl->metalDevice;
            impl->metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
            impl->metalLayer.framebufferOnly = YES;
        }
        else
        {
            impl->renderAPI = RenderAPI::Bitmap; // Fallback if Metal not available
        }
#else
        impl->renderAPI = RenderAPI::Bitmap; // Fallback if Metal not available
#endif
    }
    else if (renderAPI == RenderAPI::OpenGL)
    {
#ifdef HAVE_OPENGL
        // Initialize OpenGL
        NSOpenGLPixelFormatAttribute attrs[] = { NSOpenGLPFAAccelerated, NSOpenGLPFANoRecovery, NSOpenGLPFADoubleBuffer, NSOpenGLPFAColorSize, 24, NSOpenGLPFAAlphaSize, 8, NSOpenGLPFADepthSize, 24, 0 };
        NSOpenGLPixelFormat* pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
        impl->openglContext = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
        if (impl->openglContext)
        {
            [impl->openglContext setView:view];
        }
        else
        {
            impl->renderAPI = RenderAPI::Bitmap; // Fallback if OpenGL not available
        }
        // [pixelFormat release]; // No release needed with ARC
#else
        impl->renderAPI = RenderAPI::Bitmap; // Fallback if OpenGL not available
#endif
    }
    else if (renderAPI == RenderAPI::Vulkan)
    {
#ifndef HAVE_VULKAN
        impl->renderAPI = RenderAPI::Bitmap; // Fallback if Vulkan not available
#endif
    }

    printf("CocoaDisplayWindow created with NSWindow (RenderAPI: %d)\n", (int)impl->renderAPI);
}

CocoaDisplayWindow::~CocoaDisplayWindow()
{
    // [impl->delegate release]; // No release needed with ARC
    printf("CocoaDisplayWindow destroyed\n");
}

void CocoaDisplayWindow::SetWindowTitle(const std::string& text)
{
    [impl->window setTitle:[NSString stringWithUTF8String:text.c_str()]];
}

void CocoaDisplayWindow::SetWindowIcon(const std::vector<std::shared_ptr<Image>>& images)
{
    if (images.empty())
    {
        [NSApp setApplicationIconImage:nil];
        return;
    }

    // Find the largest image to use as the icon
    std::shared_ptr<Image> bestImage = images[0];
    for (const auto& img : images)
    {
        if (img->GetWidth() > bestImage->GetWidth())
        {
            bestImage = img;
        }
    }

    // Convert Image to NSImage
    NSBitmapImageRep* bitmapRep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
                                                                      pixelsWide:bestImage->GetWidth()
                                                                      pixelsHigh:bestImage->GetHeight()
                                                                   bitsPerSample:8
                                                                 samplesPerPixel:4
                                                                        hasAlpha:YES
                                                                        isPlanar:NO
                                                                  colorSpaceName:NSCalibratedRGBColorSpace
                                                                     bytesPerRow:bestImage->GetWidth() * 4
                                                                    bitsPerPixel:32];
    if (bitmapRep)
    {
        memcpy([bitmapRep bitmapData], bestImage->GetData(), bestImage->GetWidth() * bestImage->GetHeight() * 4);
        NSImage* icon = [[NSImage alloc] initWithSize:NSMakeSize(bestImage->GetWidth(), bestImage->GetHeight())];
        [icon addRepresentation:bitmapRep];
        [NSApp setApplicationIconImage:icon];
        // [icon release]; // No release needed with ARC
        // [bitmapRep release]; // No release needed with ARC
    }
}

void CocoaDisplayWindow::SetWindowFrame(const Rect& box)
{
    NSRect frame = NSMakeRect(box.x, box.y, box.width, box.height);
    [impl->window setFrame:frame display:YES];
}

void CocoaDisplayWindow::SetClientFrame(const Rect& box)
{
    NSRect contentRect = NSMakeRect(box.x, box.y, box.width, box.height);
    [impl->window setContentSize:contentRect.size];
    [impl->window setFrameOrigin:contentRect.origin];
}

void CocoaDisplayWindow::Show()
{
    [impl->window makeKeyAndOrderFront:nil];
}

void CocoaDisplayWindow::ShowFullscreen()
{
    if (!([impl->window styleMask] & NSWindowStyleMaskFullScreen))
    {
        [impl->window toggleFullScreen:nil];
    }
}

void CocoaDisplayWindow::ShowMaximized()
{
    if (![impl->window isZoomed])
    {
        [impl->window zoom:nil];
    }
}

void CocoaDisplayWindow::ShowMinimized()
{
    if (![impl->window isMiniaturized])
    {
        [impl->window miniaturize:nil];
    }
}

void CocoaDisplayWindow::ShowNormal()
{
    if ([impl->window isMiniaturized])
    {
        [impl->window deminiaturize:nil];
    }
    if ([impl->window isZoomed])
    {
        [impl->window zoom:nil];
    }
    if (([impl->window styleMask] & NSWindowStyleMaskFullScreen))
    {
        [impl->window toggleFullScreen:nil];
    }
}

bool CocoaDisplayWindow::IsWindowFullscreen()
{
    return ([impl->window styleMask] & NSWindowStyleMaskFullScreen);
}

void CocoaDisplayWindow::Hide()
{
    [impl->window orderOut:nil];
}

void CocoaDisplayWindow::Activate()
{
    [impl->window makeKeyWindow];
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

void CocoaDisplayWindow::LockKeyboard()
{
    // No direct equivalent for global keyboard lock on macOS
}

void CocoaDisplayWindow::UnlockKeyboard()
{
    // No direct equivalent for global keyboard unlock on macOS
}

void CocoaDisplayWindow::LockCursor()
{
    CGAssociateMouseAndMouseCursorPosition(false);
}

void CocoaDisplayWindow::UnlockCursor()
{
    CGAssociateMouseAndMouseCursorPosition(true);
}

void CocoaDisplayWindow::CaptureMouse()
{
    impl->mouseCaptured = true;
    LockCursor();
    ShowCursor(false);
    [impl->window setAcceptsMouseMovedEvents:YES];
}

void CocoaDisplayWindow::ReleaseMouseCapture()
{
    impl->mouseCaptured = false;
    UnlockCursor();
    ShowCursor(true);
    [impl->window setAcceptsMouseMovedEvents:NO];
}

void CocoaDisplayWindow::Update()
{
    [[impl->window contentView] setNeedsDisplay:YES];
}

bool CocoaDisplayWindow::GetKeyState(InputKey key)
{
    auto it = impl->keyState.find(key);
    if (it != impl->keyState.end())
        return it->second;
    return false;
}

void CocoaDisplayWindow::SetCursor(StandardCursor cursor)
{
    NSCursor* nsCursor = nil;
    switch (cursor)
    {
        case StandardCursor::arrow:
            nsCursor = [NSCursor arrowCursor];
            break;
        case StandardCursor::ibeam:
            nsCursor = [NSCursor IBeamCursor];
            break;
        case StandardCursor::cross:
            nsCursor = [NSCursor crosshairCursor];
            break;
        case StandardCursor::hand:
            nsCursor = [NSCursor pointingHandCursor];
            break;
        case StandardCursor::size_we:
            nsCursor = [NSCursor resizeLeftRightCursor];
            break;
        case StandardCursor::size_ns:
            nsCursor = [NSCursor resizeUpDownCursor];
            break;
        case StandardCursor::size_nesw:
            nsCursor = [NSCursor resizeLeftRightCursor];
            break;
        case StandardCursor::size_nwse:
            nsCursor = [NSCursor resizeUpDownCursor];
            break;
        case StandardCursor::size_all:
            nsCursor = [NSCursor openHandCursor]; // Closest equivalent, or a custom cursor
            break;
        case StandardCursor::no:
            nsCursor = [NSCursor operationNotAllowedCursor];
            break;
        case StandardCursor::wait:
        case StandardCursor::appstarting:
            nsCursor = [NSCursor arrowCursor]; // Or a custom spinning cursor
            break;
        case StandardCursor::uparrow:
            nsCursor = [NSCursor arrowCursor]; // No direct equivalent, use arrow
            break;
        default:
            nsCursor = [NSCursor arrowCursor];
            break;
    }
    [nsCursor set];
}

Rect CocoaDisplayWindow::GetWindowFrame() const
{
    NSRect frame = [impl->window frame];
    return Rect(frame.origin.x, frame.origin.y, frame.size.width, frame.size.height);
}

Size CocoaDisplayWindow::GetClientSize() const
{
    NSRect contentRect = [[impl->window contentView] frame];
    return Size(contentRect.size.width, contentRect.size.height);
}

int CocoaDisplayWindow::GetPixelWidth() const
{
    NSRect backingBounds = [[impl->window contentView] convertRectToBacking:[[impl->window contentView] bounds]];
    return (int)backingBounds.size.width;
}

int CocoaDisplayWindow::GetPixelHeight() const
{
    NSRect backingBounds = [[impl->window contentView] convertRectToBacking:[[impl->window contentView] bounds]];
    return (int)backingBounds.size.height;
}

double CocoaDisplayWindow::GetDpiScale() const
{
    if (impl->window && [impl->window screen])
    {
        return [[impl->window screen] backingScaleFactor];
    }
    return 1.0;
}

Point CocoaDisplayWindow::MapFromGlobal(const Point& pos) const
{
    if (impl->window)
    {
        NSPoint globalPoint = NSMakePoint(pos.x, pos.y);
        NSPoint windowPoint = [impl->window convertPointFromScreen:globalPoint];
        return Point(windowPoint.x, windowPoint.y);
    }
    return Point();
}

Point CocoaDisplayWindow::MapToGlobal(const Point& pos) const
{
    if (impl->window)
    {
        NSPoint windowPoint = NSMakePoint(pos.x, pos.y);
        NSPoint globalPoint = [impl->window convertPointToScreen:windowPoint];
        return Point(globalPoint.x, globalPoint.y);
    }
    return Point();
}

void CocoaDisplayWindow::SetBorderColor(uint32_t bgra8)
{
    // Setting border color directly is not supported by NSWindow without custom drawing or a custom NSWindow subclass.
}

void CocoaDisplayWindow::SetCaptionColor(uint32_t bgra8)
{
    CGFloat r = ((bgra8 >> 16) & 0xFF) / 255.0f;
    CGFloat g = ((bgra8 >> 8) & 0xFF) / 255.0f;
    CGFloat b = (bgra8 & 0xFF) / 255.0f;
    CGFloat a = ((bgra8 >> 24) & 0xFF) / 255.0f;
    impl->window.backgroundColor = [NSColor colorWithRed:r green:g blue:b alpha:a];
    impl->window.titlebarAppearsTransparent = YES;
    impl->window.titleVisibility = NSWindowTitleHidden;
}

void CocoaDisplayWindow::SetCaptionTextColor(uint32_t bgra8)
{
    // Setting caption text color directly is not supported by NSWindow without custom drawing.
    // This would require creating a custom title bar view.
}

void CocoaDisplayWindow::PresentBitmap(int width, int height, const uint32_t* pixels)
{
#ifdef HAVE_METAL
    if (impl->renderAPI == RenderAPI::Metal)
    {
        if (!impl->metalLayer || !impl->metalDevice || !impl->commandQueue)
        {
            // Metal not properly initialized, fallback to bitmap
            impl->renderAPI = RenderAPI::Bitmap;
            PresentBitmap(width, height, pixels); // Recurse with bitmap rendering
            return;
        }

        // Update metal layer drawable size
        CGSize drawableSize = { (CGFloat)width, (CGFloat)height };
        if (!CGSizeEqualToSize(impl->metalLayer.drawableSize, drawableSize))
        {
            impl->metalLayer.drawableSize = drawableSize;
        }

        id<CAMetalDrawable> drawable = [impl->metalLayer nextDrawable];
        if (!drawable)
        {
            return; // No drawable available
        }

        MTLTextureDescriptor* textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                                                    width:width
                                                                                                   height:height
                                                                                                mipmapped:NO];
        textureDescriptor.usage = MTLTextureUsageShaderRead;
        id<MTLTexture> texture = [impl->metalDevice newTextureWithDescriptor:textureDescriptor];

        MTLRegion region = MTLRegionMake2D(0, 0, width, height);
        [texture replaceRegion:region mipmapLevel:0 withBytes:pixels bytesPerRow:width * 4];

        id<MTLCommandBuffer> commandBuffer = [impl->commandQueue commandBuffer];
        id<MTLBlitCommandEncoder> blitEncoder = [commandBuffer blitCommandEncoder];

        [blitEncoder copyFromTexture:texture
                         sourceSlice:0
                         sourceLevel:0
                        sourceOrigin:MTLOriginMake(0, 0, 0)
                          sourceSize:MTLSizeMake(width, height, 1)
                           toTexture:drawable.texture
                     destinationSlice:0
                     destinationLevel:0
                    destinationOrigin:MTLOriginMake(0, 0, 0)];

        [blitEncoder endEncoding];
        [commandBuffer presentDrawable:drawable];
        [commandBuffer commit];
        return;
    }
#endif // HAVE_METAL
#ifdef HAVE_OPENGL
    else if (impl->renderAPI == RenderAPI::OpenGL)
    {
        if (!impl->openglContext)
        {
            // OpenGL not properly initialized, fallback to bitmap
            impl->renderAPI = RenderAPI::Bitmap;
            PresentBitmap(width, height, pixels); // Recurse with bitmap rendering
            return;
        }

        [impl->openglContext makeCurrentContext];

        glViewport(0, 0, width, height);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, width, height, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glEnable(GL_TEXTURE_2D);
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, pixels);

        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(0, 0);
        glTexCoord2f(1, 0); glVertex2f(width, 0);
        glTexCoord2f(1, 1); glVertex2f(width, height);
        glTexCoord2f(0, 1); glVertex2f(0, height);
        glEnd();

        glDeleteTextures(1, &textureID);
        glDisable(GL_TEXTURE_2D);

        [impl->openglContext flushBuffer];
    }
#endif // HAVE_OPENGL
    else // RenderAPI::Bitmap
    {
        // Fallback to software rendering (existing code)
        if (!impl->bitmapRep || [impl->bitmapRep pixelsWide] != width || [impl->bitmapRep pixelsHigh] != height)
        {
            if (impl->bitmapRep)
                // [impl->bitmapRep release]; // No release needed with ARC
            
            impl->bitmapRep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
                                                                      pixelsWide:width
                                                                      pixelsHigh:height
                                                                     bitsPerSample:8
                                                                 samplesPerPixel:4
                                                                        hasAlpha:YES
                                                                        isPlanar:NO
                                                                  colorSpaceName:NSCalibratedRGBColorSpace
                                                                     bytesPerRow:width * 4
                                                                    bitsPerPixel:32];
        }

        memcpy([impl->bitmapRep bitmapData], pixels, width * height * 4);

        [[impl->window contentView] setNeedsDisplay:YES];
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
    return "";
}

void CocoaDisplayWindow::SetClipboardText(const std::string& text)
{
    NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
    [pasteboard clearContents];
    [pasteboard setString:[NSString stringWithUTF8String:text.c_str()] forType:NSPasteboardTypeString];
}

void* CocoaDisplayWindow::GetNativeHandle()
{
    return (__bridge void*)impl->window;
}

#ifdef HAVE_VULKAN
// This is to avoid needing all the Vulkan headers and the volk binding library just for this:
#ifndef VK_VERSION_1_0

#define VKAPI_CALL
#define VKAPI_PTR VKAPI_CALL

typedef uint32_t VkFlags;
typedef struct VkInstance_T* VkInstance;
typedef enum VkStructureType {
    VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK = 1000124000,
    VK_OBJECT_TYPE_MAX_ENUM = 0x7FFFFFFF
} VkStructureType;

typedef enum VkResult {
    VK_SUCCESS = 0,
    VK_RESULT_MAX_ENUM = 0x7FFFFFFF
} VkResult;

typedef struct VkAllocationCallbacks VkAllocationCallbacks;

typedef void (VKAPI_PTR* PFN_vkVoidFunction)(void);
typedef PFN_vkVoidFunction(VKAPI_PTR* PFN_vkGetInstanceProcAddr)(VkInstance instance, const char* pName);

#ifndef VK_MVK_macos_surface
#define VK_MVK_macos_surface 1

typedef VkFlags VkMacOSSurfaceCreateFlagsMVK;

typedef struct VkMacOSSurfaceCreateInfoMVK {
    VkStructureType                 sType;
    const void*                     pNext;
    VkMacOSSurfaceCreateFlagsMVK    flags;
    const void*                     pView;
} VkMacOSSurfaceCreateInfoMVK;

typedef VkResult (VKAPI_PTR *PFN_vkCreateMacOSSurfaceMVK)(VkInstance instance, const VkMacOSSurfaceCreateInfoMVK* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);

#endif // VK_MVK_macos_surface
#endif // VK_VERSION_1_0

class ZWidgetCocoaVulkanLoader
{
public:
	ZWidgetCocoaVulkanLoader()
	{
		module = dlopen("libvulkan.dylib", RTLD_NOW | RTLD_LOCAL);
		if (!module)
			module = dlopen("libvulkan.1.dylib", RTLD_NOW | RTLD_LOCAL);
		if (!module)
			module = dlopen("libMoltenVK.dylib", RTLD_NOW | RTLD_LOCAL);

		if (!module)
			throw std::runtime_error("Could not load vulkan");

		vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(module, "vkGetInstanceProcAddr");
		if (!vkGetInstanceProcAddr)
		{
			dlclose(module);
			throw std::runtime_error("vkGetInstanceProcAddr not found");
		}
	}

	~ZWidgetCocoaVulkanLoader()
	{
		dlclose(module);
	}

	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
	void* module = nullptr;
};

std::vector<std::string> CocoaDisplayWindow::GetVulkanInstanceExtensions()
{
    return { "VK_KHR_surface", "VK_MVK_macos_surface" };
}

VkSurfaceKHR CocoaDisplayWindow::CreateVulkanSurface(VkInstance instance)
{
    static ZWidgetCocoaVulkanLoader loader;

    auto vkCreateMacOSSurfaceMVK = (PFN_vkCreateMacOSSurfaceMVK)loader.vkGetInstanceProcAddr(instance, "vkCreateMacOSSurfaceMVK");
    if (!vkCreateMacOSSurfaceMVK)
        throw std::runtime_error("Could not create vulkan surface");

    VkMacOSSurfaceCreateInfoMVK createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.pView = (__bridge const void*)[impl->window contentView];

    VkSurfaceKHR surface = {};
    VkResult result = vkCreateMacOSSurfaceMVK(instance, &createInfo, nullptr, &surface);
    if (result != VK_SUCCESS)
    {
        return {};
    }
    return surface;
}
#endif

#ifdef HAVE_METAL
// Metal-specific implementation would go here.
// For example, creating a CAMetalLayer or handling Metal rendering.
#endif

#ifdef HAVE_OPENGL
// OpenGL-specific implementation would go here.
// For example, creating an NSOpenGLContext or handling OpenGL rendering.
#endif
