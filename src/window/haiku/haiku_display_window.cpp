#include "haiku_display_window.h"
#include <zwidget/core/image.h>
#include <Screen.h>
#include <iostream>
#include <Clipboard.h>
#include <Cursor.h>
#include <Font.h>
#include <InterfaceDefs.h>
#include <WindowScreen.h>

HaikuDisplayWindow::HaikuDisplayWindow(DisplayWindowHost* windowHost, WidgetType type, HaikuDisplayWindow* owner, RenderAPI renderAPI)
	: windowHost(windowHost)
{
	BRect frame(100, 100, 715, 768); // Default initial frame
	
    window_look look = B_TITLED_WINDOW_LOOK;
    window_feel feel = B_NORMAL_WINDOW_FEEL;
    uint32 flags = B_ASYNCHRONOUS_CONTROLS;

    if (type == WidgetType::Popup)
    {
        look = B_NO_BORDER_WINDOW_LOOK;
        feel = B_FLOATING_ALL_WINDOW_FEEL;
    }
    else if (type == WidgetType::Dialog)
    {
        look = B_TITLED_WINDOW_LOOK;
        feel = B_MODAL_APP_WINDOW_FEEL;
    }

	window = new ZWindow(frame, "ZWidget", this, look, feel, flags, renderAPI == RenderAPI::OpenGL);
	
    if (renderAPI == RenderAPI::Bitmap || renderAPI == RenderAPI::Unspecified)
        bitmap = new BBitmap(window->Bounds(), B_RGB32);
}

HaikuDisplayWindow::~HaikuDisplayWindow()
{
	if (bitmap) delete bitmap;
	if (window)
	{
		if (window->Lock())
			window->Quit();
	}
}

void HaikuDisplayWindow::SetWindowTitle(const std::string& text) 
{
	if (window) window->SetTitle(text.c_str());
}

void HaikuDisplayWindow::SetWindowIcon(const std::vector<std::shared_ptr<Image>>& images) 
{
    // BWindow doesn't have a direct SetIcon method. 
    // Icons are usually set via the application or file attributes.
}
void HaikuDisplayWindow::SetClientFrame(const Rect& box) 
{
	if (window)
	{
		window->MoveTo(box.x, box.y);
		window->ResizeTo(box.width - 1, box.height - 1);
	}
}

void HaikuDisplayWindow::Show() 
{
    if (window) {
        window->Show();
    }
}

void HaikuDisplayWindow::ShowFullscreen() {
    if (window) {
        window->SetLook(B_BORDERED_WINDOW_LOOK);
        window->SetFeel(B_NORMAL_WINDOW_FEEL);
    }
}
void HaikuDisplayWindow::ShowMaximized() {
    if (window) {
        window->Zoom();
    }
}
void HaikuDisplayWindow::ShowMinimized() {
    if (window) {
        window->Minimize(true);
    }
}
void HaikuDisplayWindow::ShowNormal() {
    if (window) {
        window->Minimize(false);
        window->SetLook(B_TITLED_WINDOW_LOOK);
    }
}
bool HaikuDisplayWindow::IsWindowFullscreen() { 
    return false;
}
void HaikuDisplayWindow::Hide() { if (window) window->Hide(); }
void HaikuDisplayWindow::Activate() { if (window) window->Activate(); }
void HaikuDisplayWindow::LockKeyboard() {}
void HaikuDisplayWindow::UnlockKeyboard() {}
void HaikuDisplayWindow::LockCursor() 
{
    if (window && window->Lock())
    {
        be_app->HideCursor();
        cursor_locked = true;
        
        // Center the cursor
        BRect frame = window->Frame();
        BPoint center(frame.left + frame.Width() / 2, frame.top + frame.Height() / 2);
        set_mouse_position((int32)center.x, (int32)center.y);
        
        window->Unlock();
    }
}

void HaikuDisplayWindow::UnlockCursor() 
{
    if (window && window->Lock())
    {
        be_app->ShowCursor();
        cursor_locked = false;
        window->Unlock();
    }
}
void HaikuDisplayWindow::CaptureMouse() 
{
    if (window) window->CaptureMouse();
}
void HaikuDisplayWindow::ReleaseMouseCapture() 
{
    if (window) window->ReleaseMouseCapture();
}
void HaikuDisplayWindow::Update() 
{ 
	if (window && window->Lock())
	{
		BView* view = window->ChildAt(0);
		if (view)
		{
			view->Invalidate();
		}
		window->Unlock();
	}
}

Rect HaikuDisplayWindow::GetClientFrame() const 
{ 
	if (window)
	{
		BRect frame = window->Frame();
		return Rect::xywh(frame.left, frame.top, frame.Width() + 1, frame.Height() + 1);
	}
	return {0, 0, 0, 0}; 
}
Size HaikuDisplayWindow::GetClientSize() const 
{ 
	if (window)
	{
		BRect bounds = window->Bounds();
		return Size(bounds.Width() + 1, bounds.Height() + 1);
	}
	return {0, 0}; 
}
int HaikuDisplayWindow::GetPixelWidth() const { return (int)GetClientSize().width; }
int HaikuDisplayWindow::GetPixelHeight() const { return (int)GetClientSize().height; }
double HaikuDisplayWindow::GetDpiScale() const 
{ 
    return be_plain_font ? be_plain_font->Size() / 12.0 : 1.0; 
}

void HaikuDisplayWindow::PresentBitmap(int width, int height, const uint32_t* pixels) 
{
	if (window == nullptr) return;

	if (bitmap == nullptr || bitmap->Bounds().IntegerWidth() != width - 1 || bitmap->Bounds().IntegerHeight() != height - 1)
	{
		if (bitmap) delete bitmap;
		bitmap = new BBitmap(BRect(0, 0, width - 1, height - 1), B_RGB32);
	}

	bitmap->ImportBits(pixels, width * height * 4, width * 4, 0, B_RGB32);

	if (!in_draw && window->Lock())
	{
		BView* view = window->ChildAt(0);
		if (view)
		{
			view->Invalidate();
		}
		window->Unlock();
	}
}

void HaikuDisplayWindow::SetBorderColor(uint32_t bgra8) {}
void HaikuDisplayWindow::SetCaptionColor(uint32_t bgra8) {}
void HaikuDisplayWindow::SetCaptionTextColor(uint32_t bgra8) {}

std::string HaikuDisplayWindow::GetClipboardText() 
{
    BClipboard clipboard("system");
    if (clipboard.Lock())
    {
        BMessage* data = clipboard.Data();
        const char* text;
        if (data->FindString("text/plain", &text) == B_OK)
        {
            std::string result(text);
            clipboard.Unlock();
            return result;
        }
        clipboard.Unlock();
    }
    return "";
}

void HaikuDisplayWindow::SetClipboardText(const std::string& text) 
{
    BClipboard clipboard("system");
    if (clipboard.Lock())
    {
        clipboard.Clear();
        BMessage* data = clipboard.Data();
        data->AddString("text/plain", text.c_str());
        clipboard.Commit();
        clipboard.Unlock();
    }
}

Point HaikuDisplayWindow::MapFromGlobal(const Point& pos) const 
{ 
	if (window)
	{
		BPoint p(pos.x, pos.y);
		BPoint lp = window->ConvertFromScreen(p);
		return Point(lp.x, lp.y);
	}
	return pos; 
}
Point HaikuDisplayWindow::MapToGlobal(const Point& pos) const 
{ 
	if (window)
	{
		BPoint p(pos.x, pos.y);
		BPoint sp = window->ConvertToScreen(p);
		return Point(sp.x, sp.y);
	}
	return pos; 
}

std::vector<std::string> HaikuDisplayWindow::GetVulkanInstanceExtensions() 
{ 
    return {"VK_KHR_surface", "VK_KHR_haiku_surface"}; 
}

VkSurfaceKHR HaikuDisplayWindow::CreateVulkanSurface(VkInstance instance) 
{ 
    return nullptr; 
}

void HaikuDisplayWindow::ShowCursor(bool enable)
{
    if (window && window->Lock())
    {
        if (enable)
            be_app->ShowCursor();
        else
            be_app->HideCursor();
        window->Unlock();
    }
}

bool HaikuDisplayWindow::GetKeyState(InputKey key)
{
    key_info info;
    get_key_info(&info);

    auto is_pressed = [&](uint32_t scancode) {
        return (info.key_states[scancode >> 3] & (1 << (7 - (scancode % 8)))) != 0;
    };

    switch (key)
    {
        case InputKey::A: return is_pressed(0x3c);
        case InputKey::B: return is_pressed(0x50);
        case InputKey::C: return is_pressed(0x4e);
        case InputKey::D: return is_pressed(0x3e);
        case InputKey::E: return is_pressed(0x29);
        case InputKey::F: return is_pressed(0x3f);
        case InputKey::G: return is_pressed(0x40);
        case InputKey::H: return is_pressed(0x41);
        case InputKey::I: return is_pressed(0x2e);
        case InputKey::J: return is_pressed(0x42);
        case InputKey::K: return is_pressed(0x43);
        case InputKey::L: return is_pressed(0x44);
        case InputKey::M: return is_pressed(0x52);
        case InputKey::N: return is_pressed(0x51);
        case InputKey::O: return is_pressed(0x2f);
        case InputKey::P: return is_pressed(0x30);
        case InputKey::Q: return is_pressed(0x27);
        case InputKey::R: return is_pressed(0x2a);
        case InputKey::S: return is_pressed(0x3d);
        case InputKey::T: return is_pressed(0x2b);
        case InputKey::U: return is_pressed(0x2d);
        case InputKey::V: return is_pressed(0x4f);
        case InputKey::W: return is_pressed(0x28);
        case InputKey::X: return is_pressed(0x4d);
        case InputKey::Y: return is_pressed(0x2c);
        case InputKey::Z: return is_pressed(0x4c);
        case InputKey::_0: return is_pressed(0x1b);
        case InputKey::_1: return is_pressed(0x12);
        case InputKey::_2: return is_pressed(0x13);
        case InputKey::_3: return is_pressed(0x14);
        case InputKey::_4: return is_pressed(0x15);
        case InputKey::_5: return is_pressed(0x16);
        case InputKey::_6: return is_pressed(0x17);
        case InputKey::_7: return is_pressed(0x18);
        case InputKey::_8: return is_pressed(0x19);
        case InputKey::_9: return is_pressed(0x1a);
        case InputKey::F1: return is_pressed(0x02);
        case InputKey::F2: return is_pressed(0x03);
        case InputKey::F3: return is_pressed(0x04);
        case InputKey::F4: return is_pressed(0x05);
        case InputKey::F5: return is_pressed(0x06);
        case InputKey::F6: return is_pressed(0x07);
        case InputKey::F7: return is_pressed(0x08);
        case InputKey::F8: return is_pressed(0x09);
        case InputKey::F9: return is_pressed(0x0a);
        case InputKey::F10: return is_pressed(0x0b);
        case InputKey::F11: return is_pressed(0x0c);
        case InputKey::F12: return is_pressed(0x0d);
        case InputKey::LShift: return is_pressed(0x4b);
        case InputKey::RShift: return is_pressed(0x56);
        case InputKey::LControl: return is_pressed(0x5c);
        case InputKey::RControl: return is_pressed(0x67);
        case InputKey::Alt: return is_pressed(0x5e) || is_pressed(0x60);
        case InputKey::LCommand: return is_pressed(0x5d);
        case InputKey::RCommand: return is_pressed(0x66);
        case InputKey::CapsLock: return is_pressed(0x3b);
        case InputKey::Escape: return is_pressed(0x01);
        case InputKey::Space: return is_pressed(0x5f);
        case InputKey::Enter: return is_pressed(0x47) || is_pressed(0x5b);
        case InputKey::Backspace: return is_pressed(0x1e);
        case InputKey::Tab: return is_pressed(0x26);
        case InputKey::Left: return is_pressed(0x61);
        case InputKey::Right: return is_pressed(0x63);
        case InputKey::Up: return is_pressed(0x57);
        case InputKey::Down: return is_pressed(0x62);
        default: return false;
    }
}

void HaikuDisplayWindow::SetCursor(StandardCursor cursor, std::shared_ptr<CustomCursor> custom) 
{
    if (window && window->Lock())
    {
        BView* view = window->ChildAt(0);
        if (view)
        {
            switch (cursor)
            {
                case StandardCursor::arrow: view->SetViewCursor(B_CURSOR_SYSTEM_DEFAULT); break;
                case StandardCursor::ibeam: view->SetViewCursor(B_CURSOR_I_BEAM); break;
                case StandardCursor::hand: { static BCursor c(B_CURSOR_ID_FOLLOW_LINK); view->SetViewCursor(&c); break; }
                case StandardCursor::cross: { static BCursor c(B_CURSOR_ID_CROSS_HAIR); view->SetViewCursor(&c); break; }
                case StandardCursor::wait: { static BCursor c(B_CURSOR_ID_PROGRESS); view->SetViewCursor(&c); break; }
                case StandardCursor::no: { static BCursor c(B_CURSOR_ID_NOT_ALLOWED); view->SetViewCursor(&c); break; }
                case StandardCursor::size_all: { static BCursor c(B_CURSOR_ID_MOVE); view->SetViewCursor(&c); break; }
                case StandardCursor::size_we: { static BCursor c(B_CURSOR_ID_RESIZE_EAST_WEST); view->SetViewCursor(&c); break; }
                case StandardCursor::size_ns: { static BCursor c(B_CURSOR_ID_RESIZE_NORTH_SOUTH); view->SetViewCursor(&c); break; }
                case StandardCursor::size_nesw: { static BCursor c(B_CURSOR_ID_RESIZE_NORTH_EAST_SOUTH_WEST); view->SetViewCursor(&c); break; }
                case StandardCursor::size_nwse: { static BCursor c(B_CURSOR_ID_RESIZE_NORTH_WEST_SOUTH_EAST); view->SetViewCursor(&c); break; }
                default: break;
            }
        }
        window->Unlock();
    }
}
