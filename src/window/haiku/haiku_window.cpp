#include "haiku_window.h"
#include "haiku_display_window.h"
#include <Message.h>
#include <PopUpMenu.h>
#include <InterfaceDefs.h>
#include <WindowScreen.h>
#include <string>

static RawKeycode toRawKeycode(int32_t key) {
    switch (key) {
        case 0x01: return RawKeycode::Escape;
        case 0x02: return RawKeycode::F1;
        case 0x03: return RawKeycode::F2;
        case 0x04: return RawKeycode::F3;
        case 0x05: return RawKeycode::F4;
        case 0x06: return RawKeycode::F5;
        case 0x07: return RawKeycode::F6;
        case 0x08: return RawKeycode::F7;
        case 0x09: return RawKeycode::F8;
        case 0x0a: return RawKeycode::F9;
        case 0x0b: return RawKeycode::F10;
        case 0x0c: return RawKeycode::F11;
        case 0x0d: return RawKeycode::F12;
        case 0x0e: return RawKeycode::SysRq;
        case 0x0f: return RawKeycode::Scroll;
        case 0x10: return RawKeycode::Pause;

        case 0x11: return RawKeycode::Grave;
        case 0x12: return RawKeycode::_1;
        case 0x13: return RawKeycode::_2;
        case 0x14: return RawKeycode::_3;
        case 0x15: return RawKeycode::_4;
        case 0x16: return RawKeycode::_5;
        case 0x17: return RawKeycode::_6;
        case 0x18: return RawKeycode::_7;
        case 0x19: return RawKeycode::_8;
        case 0x1a: return RawKeycode::_9;
        case 0x1b: return RawKeycode::_0;
        case 0x1c: return RawKeycode::Minus;
        case 0x1d: return RawKeycode::Equals;
        case 0x1e: return RawKeycode::Backspace;

        case 0x1f: return RawKeycode::Insert;
        case 0x20: return RawKeycode::Home;
        case 0x21: return RawKeycode::PageUp;

        case 0x22: return RawKeycode::Numlock;
        case 0x23: return RawKeycode::NumpadDivide;
        case 0x24: return RawKeycode::NumpadMultiply;
        case 0x25: return RawKeycode::NumpadSubstract;

        case 0x26: return RawKeycode::Tab;
        case 0x27: return RawKeycode::Q;
        case 0x28: return RawKeycode::W;
        case 0x29: return RawKeycode::E;
        case 0x2a: return RawKeycode::R;
        case 0x2b: return RawKeycode::T;
        case 0x2c: return RawKeycode::Y;
        case 0x2d: return RawKeycode::U;
        case 0x2e: return RawKeycode::I;
        case 0x2f: return RawKeycode::O;
        case 0x30: return RawKeycode::P;
        case 0x31: return RawKeycode::LBracket;
        case 0x32: return RawKeycode::RBracket;
        case 0x33: return RawKeycode::Backslash;

        case 0x34: return RawKeycode::Delete;
        case 0x35: return RawKeycode::End;
        case 0x36: return RawKeycode::PageDown;

        case 0x37: return RawKeycode::Numpad7;
        case 0x38: return RawKeycode::Numpad8;
        case 0x39: return RawKeycode::Numpad9;
        case 0x3a: return RawKeycode::NumpadAdd;

        case 0x3b: return RawKeycode::CapsLock;
        case 0x3c: return RawKeycode::A;
        case 0x3d: return RawKeycode::S;
        case 0x3e: return RawKeycode::D;
        case 0x3f: return RawKeycode::F;
        case 0x40: return RawKeycode::G;
        case 0x41: return RawKeycode::H;
        case 0x42: return RawKeycode::J;
        case 0x43: return RawKeycode::K;
        case 0x44: return RawKeycode::L;
        case 0x45: return RawKeycode::Semicolon;
        case 0x46: return RawKeycode::Apostrophe;
        case 0x47: return RawKeycode::Return;

        case 0x48: return RawKeycode::Numpad4;
        case 0x49: return RawKeycode::Numpad5;
        case 0x4a: return RawKeycode::Numpad6;

        case 0x4b: return RawKeycode::LShift;
        case 0x4c: return RawKeycode::Z;
        case 0x4d: return RawKeycode::X;
        case 0x4e: return RawKeycode::C;
        case 0x4f: return RawKeycode::V;
        case 0x50: return RawKeycode::B;
        case 0x51: return RawKeycode::N;
        case 0x52: return RawKeycode::M;
        case 0x53: return RawKeycode::Comma;
        case 0x54: return RawKeycode::Period;
        case 0x55: return RawKeycode::Slash;
        case 0x56: return RawKeycode::RShift;

        case 0x57: return RawKeycode::Up;

        case 0x58: return RawKeycode::Numpad1;
        case 0x59: return RawKeycode::Numpad2;
        case 0x5a: return RawKeycode::Numpad3;
        case 0x5b: return RawKeycode::NumpadEnter;

        case 0x5c: return RawKeycode::LControl;
        case 0x5d: return RawKeycode::LCmd;
        case 0x5e: return RawKeycode::LAlt;
        case 0x5f: return RawKeycode::Space;
        case 0x60: return RawKeycode::RAlt;
        case 0x61: return RawKeycode::Left;
        case 0x62: return RawKeycode::Down;
        case 0x63: return RawKeycode::Right;

        case 0x64: return RawKeycode::Numpad0;
        case 0x65: return RawKeycode::NumpadDecimal;

        case 0x66: return RawKeycode::RCmd;
        case 0x67: return RawKeycode::RControl;
        case 0x68: return RawKeycode::Apps;
    }

    return RawKeycode::None;
}

static InputKey toInputKey(int32_t key, int32_t raw_char) {
    // 1. Scan codes for function keys and modifiers (most reliable for these)
    switch (key) {
        case 0x02: return InputKey::F1;
        case 0x03: return InputKey::F2;
        case 0x04: return InputKey::F3;
        case 0x05: return InputKey::F4;
        case 0x06: return InputKey::F5;
        case 0x07: return InputKey::F6;
        case 0x08: return InputKey::F7;
        case 0x09: return InputKey::F8;
        case 0x0a: return InputKey::F9;
        case 0x0b: return InputKey::F10;
        case 0x0c: return InputKey::F11;
        case 0x0d: return InputKey::F12;
        case 0x4b: return InputKey::LShift;
        case 0x56: return InputKey::RShift;
        case 0x5c: return InputKey::LControl;
        case 0x67: return InputKey::RControl;
        case 0x5e: return InputKey::Alt;
        case 0x60: return InputKey::Alt;
        case 0x5d: return InputKey::LCommand;
        case 0x66: return InputKey::RCommand;
        case 0x3b: return InputKey::CapsLock;
    }

    // 2. Map everything else by raw_char (Priority)
    if (raw_char != 0) {
        if (raw_char >= 'a' && raw_char <= 'z') return (InputKey)((uint32_t)InputKey::A + (raw_char - 'a'));
        if (raw_char >= 'A' && raw_char <= 'Z') return (InputKey)((uint32_t)InputKey::A + (raw_char - 'A'));
        if (raw_char >= '0' && raw_char <= '9') return (InputKey)((uint32_t)InputKey::_0 + (raw_char - '0'));

        switch (raw_char) {
            case B_BACKSPACE: return InputKey::Backspace;
            case B_TAB: return InputKey::Tab;
            case B_ENTER: return InputKey::Enter;
            case B_ESCAPE: return InputKey::Escape;
            case B_SPACE: return InputKey::Space;
            case B_LEFT_ARROW: return InputKey::Left;
            case B_RIGHT_ARROW: return InputKey::Right;
            case B_UP_ARROW: return InputKey::Up;
            case B_DOWN_ARROW: return InputKey::Down;
            case B_INSERT: return InputKey::Insert;
            case B_DELETE: return InputKey::Delete;
            case B_HOME: return InputKey::Home;
            case B_END: return InputKey::End;
            case B_PAGE_UP: return InputKey::PageUp;
            case B_PAGE_DOWN: return InputKey::PageDown;
            case '-': return InputKey::Minus;
            case '=': return InputKey::Equals;
            case '[': return InputKey::LeftBracket;
            case ']': return InputKey::RightBracket;
            case '\\': return InputKey::Backslash;
            case ';': return InputKey::Semicolon;
            case '\'': return InputKey::SingleQuote;
            case ',': return InputKey::Comma;
            case '.': return InputKey::Period;
            case '/': return InputKey::Slash;
            case '`': return InputKey::Tilde;
        }
    }

    // 3. Fallback to scan code
    switch (key) {
        case 0x61: return InputKey::Left;
        case 0x63: return InputKey::Right;
        case 0x57: return InputKey::Up;
        case 0x62: return InputKey::Down;
        case 0x1f: return InputKey::Insert;
        case 0x34: return InputKey::Delete;
        case 0x20: return InputKey::Home;
        case 0x35: return InputKey::End;
        case 0x21: return InputKey::PageUp;
        case 0x36: return InputKey::PageDown;
        case 0x1c: return InputKey::Minus;
        case 0x1d: return InputKey::Equals;
        case 0x31: return InputKey::LeftBracket;
        case 0x32: return InputKey::RightBracket;
        case 0x33: return InputKey::Backslash;
        case 0x45: return InputKey::Semicolon;
        case 0x46: return InputKey::SingleQuote;
        case 0x53: return InputKey::Comma;
        case 0x54: return InputKey::Period;
        case 0x55: return InputKey::Slash;
        case 0x11: return InputKey::Tilde;
        case 0x5f: return InputKey::Space;
        case 0x1e: return InputKey::Backspace;
        case 0x26: return InputKey::Tab;
        case 0x47: return InputKey::Enter;
        case 0x01: return InputKey::Escape;

        case 0x22: return InputKey::NumLock;
        case 0x0f: return InputKey::ScrollLock;
        case 0x10: return InputKey::Pause;
        case 0x0e: return InputKey::Print;
        
        case 0x37: return InputKey::NumPad7;
        case 0x38: return InputKey::NumPad8;
        case 0x39: return InputKey::NumPad9;
        case 0x3a: return InputKey::GreyPlus;
        case 0x48: return InputKey::NumPad4;
        case 0x49: return InputKey::NumPad5;
        case 0x4a: return InputKey::NumPad6;
        case 0x58: return InputKey::NumPad1;
        case 0x59: return InputKey::NumPad2;
        case 0x5a: return InputKey::NumPad3;
        case 0x5b: return InputKey::Enter;
        case 0x64: return InputKey::NumPad0;
        case 0x65: return InputKey::NumPadPeriod;
        case 0x23: return InputKey::GreySlash;
        case 0x24: return InputKey::GreyStar;
        case 0x25: return InputKey::GreyMinus;
    }

    return InputKey::None;
}

class ZView : public BView {
public:
    ZView(BRect frame, HaikuDisplayWindow* parent) 
        : BView(frame, "ZView", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS), parent(parent) {}
    
    void Draw(BRect updateRect) override {
        if (!parent->in_draw) {
            parent->in_draw = true;
            parent->windowHost->OnWindowPaint();
            parent->in_draw = false;
        }
        if (parent->bitmap) {
            DrawBitmap(parent->bitmap, updateRect, updateRect);
        }
    }

    void MouseMoved(BPoint point, uint32 transit, const BMessage* message) override {
        if (parent->cursor_locked) {
            BRect frame = Window()->Frame();
            BPoint center(frame.Width() / 2, frame.Height() / 2);
            if (point.x != center.x || point.y != center.y) {
                parent->windowHost->OnWindowRawMouseMove((int)(point.x - center.x), (int)(point.y - center.y));
                BPoint screenCenter = Window()->ConvertToScreen(center);
                set_mouse_position((int32)screenCenter.x, (int32)screenCenter.y);
            }
        } else {
            parent->windowHost->OnWindowMouseMove({(double)point.x, (double)point.y});
        }
    }

    void MouseDown(BPoint point) override {
        parent->windowHost->OnWindowMouseDown({(double)point.x, (double)point.y}, InputKey::LeftMouse);
    }

    void MouseUp(BPoint point) override {
        parent->windowHost->OnWindowMouseUp({(double)point.x, (double)point.y}, InputKey::LeftMouse);
    }

    void MessageReceived(BMessage* message) override {
        if (message->what == B_MOUSE_WHEEL_CHANGED) {
            float deltaY = 0;
            if (message->FindFloat("be:wheel_delta_y", &deltaY) == B_OK) {
                if (deltaY != 0) {
                    parent->windowHost->OnWindowMouseWheel({0, 0}, deltaY < 0 ? InputKey::MouseWheelDown : InputKey::MouseWheelUp);
                }
            }
        }
        BView::MessageReceived(message);
    }

    void KeyDown(const char* bytes, int32 numBytes) override {
        BMessage* message = Window()->CurrentMessage();
        int32 key = 0;
        int32 raw_char = 0;
        if (message) {
            message->FindInt32("key", &key);
            message->FindInt32("raw_char", &raw_char);
        }

        parent->windowHost->OnWindowKeyDown(toInputKey(key, raw_char));
        parent->windowHost->OnWindowRawKey(toRawKeycode(key), true);
        
        if (numBytes > 0 && (unsigned char)bytes[0] >= 32) {
            std::string chars(bytes, numBytes);
            parent->windowHost->OnWindowKeyChar(chars);
        }
    }

    void KeyUp(const char* bytes, int32 numBytes) override {
        BMessage* message = Window()->CurrentMessage();
        int32 key = 0;
        int32 raw_char = 0;
        if (message) {
            message->FindInt32("key", &key);
            message->FindInt32("raw_char", &raw_char);
        }

        parent->windowHost->OnWindowKeyUp(toInputKey(key, raw_char));
        parent->windowHost->OnWindowRawKey(toRawKeycode(key), false);
    }

private:
    HaikuDisplayWindow* parent;
};

ZWindow::ZWindow(BRect frame, const char* title, HaikuDisplayWindow* parent, window_look look, window_feel feel, uint32 flags, bool use_opengl)
	: BWindow(frame, title, look, feel, flags), parent(parent)
{
    if (use_opengl)
    {
        gl_view = new BGLView(Bounds(), "ZGLView", B_FOLLOW_ALL, B_WILL_DRAW, BGL_RGB | BGL_DOUBLE | BGL_DEPTH);
        AddChild(gl_view);
        view = gl_view;
    }
    else
    {
	    view = new ZView(Bounds(), parent);
	    AddChild(view);
    }
    view->MakeFocus(true);
}

void ZWindow::FrameResized(float newWidth, float newHeight)
{
	parent->windowHost->OnWindowGeometryChanged();
}

bool ZWindow::QuitRequested()
{
	parent->windowHost->OnWindowClose();
	return true;
}

void ZWindow::CaptureMouse()
{
    if (view && Lock())
    {
        view->SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
        Unlock();
    }
}

void ZWindow::ReleaseMouseCapture()
{
    // In Haiku, mouse capture is usually released automatically or by passing 0 to SetMouseEventMask?
    // Actually, passing 0 or just letting it go.
}

void ZWindow::MessageReceived(BMessage* message)
{
	BWindow::MessageReceived(message);
}
