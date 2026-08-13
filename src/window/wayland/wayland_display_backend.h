#pragma once

#include "window/window.h"
#include "window/ztimer/ztimer.h"
#include "wayland_dynamic.h"

#include <linux/input.h>
#include <poll.h>
#include <map>
#include <set>
#include <xkbcommon/xkbcommon.h>

// Forward declarations for Wayland C types (proxies)
struct wl_display;
struct wl_registry;
struct wl_compositor;
struct wl_shm;
struct wl_seat;
struct wl_output;
struct wl_data_device_manager;
struct wl_data_device;
struct wl_data_offer;
struct xdg_wm_base;
struct zwp_pointer_constraints_v1;
struct xdg_activation_v1;
struct zxdg_decoration_manager_v1;
struct wp_fractional_scale_manager_v1;
struct zxdg_output_manager_v1;
struct zxdg_output_v1;
struct zxdg_exporter_v2;
struct wl_keyboard;
struct wl_pointer;
struct zwp_relative_pointer_manager_v1;
struct zwp_relative_pointer_v1;
struct xdg_toplevel_icon_manager_v1;
struct wp_cursor_shape_manager_v1;
struct wp_cursor_shape_device_v1;
struct wl_surface;
struct wl_buffer;
struct wl_cursor_theme;
struct wl_cursor;
struct wl_cursor_image;

static short poll_single(int fd, short events, int timeout)
{
	pollfd pfd { .fd = fd, .events = events, .revents = 0 };
	if (0 > poll(&pfd, 1, timeout))
	{
		throw std::runtime_error("poll() failed");
	}

	return pfd.revents;
}

enum pointer_event_mask
{
	   POINTER_EVENT_ENTER = 1 << 0,
	   POINTER_EVENT_LEAVE = 1 << 1,
	   POINTER_EVENT_MOTION = 1 << 2,
	   POINTER_EVENT_BUTTON = 1 << 3,
	   POINTER_EVENT_AXIS = 1 << 4,
	   POINTER_EVENT_AXIS_SOURCE = 1 << 5,
	   POINTER_EVENT_AXIS_STOP = 1 << 6,
	   POINTER_EVENT_AXIS_DISCRETE = 1 << 7,
	   POINTER_EVENT_AXIS_120 = 1 << 8,
	   POINTER_EVENT_RELATIVE_MOTION = 1 << 9,
};

struct WaylandPointerEvent
{
	uint32_t event_mask;
	double surfaceX, surfaceY;
	double dx, dy, dx_unaccel, dy_unaccel;
	uint32_t button;
	uint32_t state; // wl_pointer_button_state
	uint32_t time;
	uint32_t serial;
	struct {
		bool valid;
		double value;
		int32_t discrete;
		int32_t value120;
	} axes[2];
	uint32_t axis_source; // wl_pointer_axis_source
};

class WaylandTimer
{
public:
	WaylandTimer(int timeoutMilliseconds, std::function<void()> onTimer, int64_t nextTime) : timeoutMilliseconds(timeoutMilliseconds), onTimer(onTimer), nextTime(nextTime) {}

	int timeoutMilliseconds = 0;
	std::function<void()> onTimer;
	int64_t nextTime = 0;
};

class WaylandDisplayWindow;

class WaylandDisplayBackend : public DisplayBackend
{
public:
	WaylandDisplayBackend();
	~WaylandDisplayBackend();

	std::unique_ptr<DisplayWindow> Create(DisplayWindowHost* windowHost, WidgetType type, DisplayWindow* owner, RenderAPI renderAPI) override;
	void ProcessEvents() override;
	void RunLoop() override;
	void ExitLoop() override;

	void* StartTimer(int timeoutMilliseconds, std::function<void()> onTimer) override;
	void StopTimer(void* timerID) override;

	Size GetScreenSize() override;

	bool IsWayland() override { return true; }

	void OnWindowCreated(WaylandDisplayWindow* window);
	void OnWindowDestroyed(WaylandDisplayWindow* window);

	void SetCursor(StandardCursor cursor);
	void ShowCursor(bool enable);
	bool GetKeyState(InputKey key);

	std::string GetClipboardText();
	void SetClipboardText(const std::string& text);
	void EnsureDataDevice();

	struct wl_data_device* GetDataDevice() { return m_DataDevice; }
	uint32_t GetKeyboardSerial() const { return m_KeyboardSerial; }

#ifdef USE_DBUS
	std::unique_ptr<OpenFileDialog> CreateOpenFileDialog(DisplayWindow* owner) override;
	std::unique_ptr<SaveFileDialog> CreateSaveFileDialog(DisplayWindow* owner) override;
	std::unique_ptr<OpenFolderDialog> CreateOpenFolderDialog(DisplayWindow* owner) override;
#endif

	bool exitRunLoop = false;
	Size s_ScreenSize = Size(0, 0);
	double s_DpiScale = 1.0;
	struct wl_display* s_waylandDisplay = nullptr;
	struct wl_registry* s_waylandRegistry = nullptr;
	std::vector<WaylandDisplayWindow*> s_Windows;
	WaylandDisplayWindow* m_FocusWindow = nullptr;
	WaylandDisplayWindow* m_MouseFocusWindow = nullptr; // Mouse focus should be tracked separately.
	WaylandDisplayWindow* m_ActiveWindow = nullptr;
	WaylandDisplayWindow* m_HoverWindow = nullptr;

	struct wl_compositor* m_waylandCompositor = nullptr;
	struct wl_shm* m_waylandSHM = nullptr;
	struct wl_seat* m_waylandSeat = nullptr;
	struct wl_output* m_waylandOutput = nullptr;
	struct wl_data_device_manager* m_DataDeviceManager = nullptr;
	struct xdg_wm_base* m_XDGWMBase = nullptr;
	struct zwp_pointer_constraints_v1* m_PointerConstraints = nullptr;
	struct xdg_activation_v1* m_XDGActivation = nullptr;
	struct zxdg_decoration_manager_v1* m_XDGDecorationManager = nullptr;
	struct wp_fractional_scale_manager_v1* m_FractionalScaleManager = nullptr;
	struct zxdg_output_manager_v1* m_XDGOutputManager = nullptr;
	struct zxdg_output_v1* m_XDGOutput = nullptr;
	struct zxdg_exporter_v2* m_XDGExporter = nullptr;

	struct wl_keyboard* m_waylandKeyboard = nullptr;
	struct wl_pointer* m_waylandPointer = nullptr;

	struct zwp_relative_pointer_manager_v1* m_RelativePointerManager = nullptr;
	struct zwp_relative_pointer_v1* m_RelativePointer = nullptr;

	struct xdg_toplevel_icon_manager_v1* m_XDGToplevelIconManager = nullptr;

	struct xdg_wm_dialog_v1* m_XDGWMDialog = nullptr;
	struct wp_cursor_shape_manager_v1* m_CursorShapeManager = nullptr;
	struct wp_cursor_shape_device_v1* m_CursorShapeDevice = nullptr;

	struct wl_cursor_theme* m_cursorTheme = nullptr;
	struct wl_cursor* m_obtainedCursor = nullptr;
	struct wl_cursor_image* m_cursorImage = nullptr;
	struct wl_surface* m_cursorSurface = nullptr;
	struct wl_buffer* m_cursorBuffer = nullptr;
	bool m_CursorVisible = true;

	std::map<InputKey, bool> inputKeyStates; // True when the key is pressed, false when isn't

	// Scancode -> InputKey latched at key-press time.
	//
	// The keysym for a physical key depends on the active modifier and layout
	// state, so resolving it again at release time can yield a different
	// InputKey than the press did (press '1' -> XKB_KEY_1, hold Shift, release
	// -> XKB_KEY_exclam). That delivers the key-up for the wrong key and leaves
	// the original stuck down. Latching the press-time mapping and replaying it
	// on release keeps press/release symmetric across modifier and layout
	// changes. It also gives us the set of currently-held keys, which is what
	// keyboard enter/leave need in order to stay in sync with the compositor.
	std::map<uint32_t, InputKey> m_PressedScancodes;

	// Client-side key repeat.
	//
	// wl_keyboard only tells us the rate and delay via repeat_info; generating
	// the repeats is the client's job. (wl_keyboard v10 can send a REPEATED key
	// state, but few compositors do, so we only defer to it if we actually see
	// one.) Without this, held keys never repeat in menus or the console.
	//
	// The repeat is driven from ProcessEvents() rather than a WaylandTimer,
	// because an embedding application may pump ProcessEvents() itself instead
	// of calling RunLoop(), in which case UpdateTimers() never runs.
	int32_t m_RepeatRate = 25;      // repeats per second; 0 disables repeat
	int32_t m_RepeatDelay = 600;    // milliseconds before the first repeat
	uint32_t m_RepeatScancode = 0;  // 0 when no key is repeating
	InputKey m_RepeatKey = InputKey::None;
	int64_t m_RepeatNextMs = 0;
	bool m_CompositorSendsRepeat = false;

	void UpdateKeyRepeat();
	void StartKeyRepeat(uint32_t scancode, InputKey key);
	void StopKeyRepeat() { m_RepeatScancode = 0; m_RepeatKey = InputKey::None; }

	bool IsMouseLocked() { return hasMouseLock; }
	void SetMouseLocked(bool val) { hasMouseLock = val; }

	void OnKeyboardKeyEvent(xkb_keysym_t xkbKeySym, uint32_t state);
	void OnKeyboardCharEvent(const char* ch, uint32_t state);
	void OnKeyboardDelayEnd();
	void OnKeyboardRepeat();
	void OnMouseEnterEvent(uint32_t serial);
	void OnMouseLeaveEvent();
	void OnMousePressEvent(InputKey button);
	void OnMouseReleaseEvent(InputKey button);
	void OnMouseMoveEvent(Point surfacePos);
	void OnMouseMoveRawEvent(int surfaceX, int surfaceY);
	void OnMouseWheelEvent(InputKey button);

	InputKey XKBKeySymToInputKey(xkb_keysym_t keySym);
	InputKey LinuxInputEventCodeToInputKey(uint32_t inputCode);

	uint32_t GetWaylandCursorShape(StandardCursor cursor);
	std::string GetWaylandCursorName(StandardCursor cursor);

	bool hasKeyboard = false;
	bool hasPointer = false;
	bool hasMouseLock = false;

	uint32_t m_KeyboardSerial = 0;
	uint32_t m_MouseSerial = 0;
	uint32_t m_PointerSerial = 0;

	xkb_context* m_KeymapContext = nullptr;
	xkb_keymap* m_Keymap = nullptr;
	xkb_state* m_KeyboardState = nullptr;

	WaylandPointerEvent currentPointerEvent = {0};

	struct wl_data_device* m_DataDevice = nullptr;
	std::unordered_map<struct wl_data_offer*, std::set<std::string>> m_DataOfferMimeTypes;

	struct wl_data_source* m_DataSource = nullptr;
	std::string m_ClipboardText;
	std::string m_ClipboardContents;
	std::string m_ClipboardMimeType;

	std::vector<std::shared_ptr<WaylandTimer>> m_timers;

	ZTimer::TimePoint m_previousTime;
	ZTimer::TimePoint m_currentTime;

	ZTimer m_keyboardDelayTimer;
	ZTimer m_keyboardRepeatTimer;

	InputKey previousKey = {};
	std::string previousChars;

private:
	void CheckNeedsUpdate();
	void UpdateTimers();
	void WaitForEvents(int timeout);
	int GetTimerTimeout();
	void ConnectDeviceEvents();
};
