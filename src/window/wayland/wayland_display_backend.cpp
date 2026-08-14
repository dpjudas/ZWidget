#include "wayland_display_backend.h"
#include "wayland_display_window.h"
#include "wayland_dynamic.h"
#include "xdg-shell-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"
#include "xdg-foreign-unstable-v2-client-protocol.h"
#include "pointer-constraints-unstable-v1-client-protocol.h"
#include "xdg-activation-v1-client-protocol.h"
#include "xdg-decoration-unstable-v1-client-protocol.h"
#include "fractional-scale-v1-client-protocol.h"
#include "relative-pointer-unstable-v1-client-protocol.h"
#include "xdg-toplevel-icon-v1-client-protocol.h"
#include "cursor-shape-v1-client-protocol.h"
#include "xdg-dialog-v1-client-protocol.h"
#include <chrono>
#include <sys/mman.h>
#include <unistd.h>
#include <poll.h>
#define wl_fixed_to_int_hack(f) ((f) >> 8)
#include <cstring>
#include <stdarg.h>

#ifdef USE_DBUS
#include "window/dbus/dbus_open_file_dialog.h"
#include "window/dbus/dbus_save_file_dialog.h"
#include "window/dbus/dbus_open_folder_dialog.h"
#endif

#define WAYLAND WaylandDynamic::Get()

// Remap core interfaces to the dynamically loaded ones via getter functions
#define wl_registry_interface (*WAYLAND->GetRegistryInterface())
#define wl_compositor_interface (*WAYLAND->GetCompositorInterface())
#define wl_shm_interface (*WAYLAND->GetShmInterface())
#define wl_seat_interface (*WAYLAND->GetSeatInterface())
#define wl_output_interface (*WAYLAND->GetOutputInterface())
#define wl_data_device_manager_interface (*WAYLAND->GetDataDeviceManagerInterface())

// Remap XKB as well
#undef xkb_context_new
#define xkb_context_new WAYLAND->p_xkb_context_new
#undef xkb_context_unref
#define xkb_context_unref WAYLAND->p_xkb_context_unref
#undef xkb_keymap_new_from_string
#define xkb_keymap_new_from_string WAYLAND->p_xkb_keymap_new_from_string
#undef xkb_keymap_unref
#define xkb_keymap_unref WAYLAND->p_xkb_keymap_unref
#undef xkb_state_new
#define xkb_state_new WAYLAND->p_xkb_state_new
#undef xkb_state_unref
#define xkb_state_unref WAYLAND->p_xkb_state_unref
#undef xkb_state_update_mask
#define xkb_state_update_mask WAYLAND->p_xkb_state_update_mask
#undef xkb_state_key_get_one_sym
#define xkb_state_key_get_one_sym WAYLAND->p_xkb_state_key_get_one_sym
#undef xkb_state_key_get_utf8
#define xkb_state_key_get_utf8 WAYLAND->p_xkb_state_key_get_utf8
#define WAYLAND_wl_fixed_to_double(f) ((double)(f) / 256.0)

// Listeners
static void registry_handle_global(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version);
static void registry_handle_global_remove(void* data, struct wl_registry* registry, uint32_t name) {}
static const struct wl_registry_listener registry_listener = { registry_handle_global, registry_handle_global_remove };

static void seat_handle_capabilities(void* data, struct wl_seat* seat, uint32_t caps);
static void seat_handle_name(void* data, struct wl_seat* seat, const char* name) {}
static const struct wl_seat_listener seat_listener = { seat_handle_capabilities, seat_handle_name };

static void xdg_wm_base_handle_ping(void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial)
{
	WAYLAND->p_proxy_marshal_flags((struct wl_proxy*)xdg_wm_base, XDG_WM_BASE_PONG, NULL, wl_proxy_get_version((struct wl_proxy*)xdg_wm_base), 0, serial);
}
static const struct xdg_wm_base_listener xdg_wm_base_listener = { xdg_wm_base_handle_ping };

static void output_handle_geometry(void* data, struct wl_output* output, int32_t x, int32_t y, int32_t physical_width, int32_t physical_height, int32_t subpixel, const char* make, const char* model, int32_t transform) {}
static void output_handle_mode(void* data, struct wl_output* output, uint32_t flags, int32_t width, int32_t height, int32_t refresh);
static void output_handle_done(void* data, struct wl_output* output) {}
static void output_handle_scale(void* data, struct wl_output* output, int32_t factor);
static const struct wl_output_listener output_listener = { output_handle_geometry, output_handle_mode, output_handle_done, output_handle_scale };

static void xdg_output_handle_logical_position(void* data, struct zxdg_output_v1* zxdg_output_v1, int32_t x, int32_t y) {}
static void xdg_output_handle_logical_size(void* data, struct zxdg_output_v1* zxdg_output_v1, int32_t width, int32_t height);
static void xdg_output_handle_done(void* data, struct zxdg_output_v1* zxdg_output_v1) {}
static void xdg_output_handle_name(void* data, struct zxdg_output_v1* zxdg_output_v1, const char* name) {}
static void xdg_output_handle_description(void* data, struct zxdg_output_v1* zxdg_output_v1, const char* description) {}
static const struct zxdg_output_v1_listener xdg_output_listener = { xdg_output_handle_logical_position, xdg_output_handle_logical_size, xdg_output_handle_done, xdg_output_handle_name, xdg_output_handle_description };

static void data_offer_handle_offer(void* data, struct wl_data_offer* wl_data_offer, const char* mime_type);
static void data_offer_handle_source_actions(void* data, struct wl_data_offer* wl_data_offer, uint32_t source_actions) {}
static void data_offer_handle_action(void* data, struct wl_data_offer* wl_data_offer, uint32_t dnd_action) {}
static const struct wl_data_offer_listener data_offer_listener = { data_offer_handle_offer, data_offer_handle_source_actions, data_offer_handle_action };

static void data_device_handle_data_offer(void* data, struct wl_data_device* wl_data_device, struct wl_data_offer* id);
static void data_device_handle_enter(void* data, struct wl_data_device* wl_data_device, uint32_t serial, struct wl_surface* surface, wl_fixed_t x, wl_fixed_t y, struct wl_data_offer* id) {}
static void data_device_handle_leave(void* data, struct wl_data_device* wl_data_device) {}
static void data_device_handle_motion(void* data, struct wl_data_device* wl_data_device, uint32_t time, wl_fixed_t x, wl_fixed_t y) {}
static void data_device_handle_drop(void* data, struct wl_data_device* wl_data_device) {}
static void data_device_handle_selection(void* data, struct wl_data_device* wl_data_device, struct wl_data_offer* id);
static const struct wl_data_device_listener data_device_listener = { data_device_handle_data_offer, data_device_handle_enter, data_device_handle_leave, data_device_handle_motion, data_device_handle_drop, data_device_handle_selection };

static void keyboard_handle_keymap(void* data, struct wl_keyboard* wl_keyboard, uint32_t format, int32_t fd, uint32_t size);
static void keyboard_handle_enter(void* data, struct wl_keyboard* wl_keyboard, uint32_t serial, struct wl_surface* surface, struct wl_array* keys);
static void keyboard_handle_leave(void* data, struct wl_keyboard* wl_keyboard, uint32_t serial, struct wl_surface* surface);
static void keyboard_handle_key(void* data, struct wl_keyboard* wl_keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
static void keyboard_handle_modifiers(void* data, struct wl_keyboard* wl_keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group);
static void keyboard_handle_repeat_info(void* data, struct wl_keyboard* wl_keyboard, int32_t rate, int32_t delay);
static const struct wl_keyboard_listener keyboard_listener = { keyboard_handle_keymap, keyboard_handle_enter, keyboard_handle_leave, keyboard_handle_key, keyboard_handle_modifiers, keyboard_handle_repeat_info };

static void pointer_handle_enter(void* data, struct wl_pointer* wl_pointer, uint32_t serial, struct wl_surface* surface, wl_fixed_t surface_x, wl_fixed_t surface_y);
static void pointer_handle_leave(void* data, struct wl_pointer* wl_pointer, uint32_t serial, struct wl_surface* surface);
static void pointer_handle_motion(void* data, struct wl_pointer* wl_pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y);
static void pointer_handle_button(void* data, struct wl_pointer* wl_pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
static void pointer_handle_axis(void* data, struct wl_pointer* wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value);
static void pointer_handle_frame(void* data, struct wl_pointer* wl_pointer);
static void pointer_handle_axis_source(void* data, struct wl_pointer* wl_pointer, uint32_t axis_source) {}
static void pointer_handle_axis_stop(void* data, struct wl_pointer* wl_pointer, uint32_t time, uint32_t axis) {}
static void pointer_handle_axis_discrete(void* data, struct wl_pointer* wl_pointer, uint32_t axis, int32_t discrete) {}
static void pointer_handle_axis_value120(void* data, struct wl_pointer* wl_pointer, uint32_t axis, int32_t value120) {}
#ifdef WL_POINTER_AXIS_RELATIVE_DIRECTION_SINCE_VERSION
static void pointer_handle_axis_relative_direction(void* data, struct wl_pointer* wl_pointer, uint32_t axis, uint32_t direction) {}
#endif
// wl_pointer_listener grew over time: axis_value120 arrived in wayland 1.21 and
// axis_relative_direction in 1.22. This compiles against the system libwayland
// headers, which on Ubuntu 22.04 are 1.20 and declare neither, so listing the
// newer handlers unconditionally overflows the struct. Key off the SINCE_VERSION
// macros the header defines for each request, so we supply exactly the handlers
// the local wayland version knows about.
static const struct wl_pointer_listener pointer_listener = {
	pointer_handle_enter,
	pointer_handle_leave,
	pointer_handle_motion,
	pointer_handle_button,
	pointer_handle_axis,
	pointer_handle_frame,
	pointer_handle_axis_source,
	pointer_handle_axis_stop,
	pointer_handle_axis_discrete,
#ifdef WL_POINTER_AXIS_VALUE120_SINCE_VERSION
	pointer_handle_axis_value120,
#endif
#ifdef WL_POINTER_AXIS_RELATIVE_DIRECTION_SINCE_VERSION
	pointer_handle_axis_relative_direction,
#endif
};

static void relative_pointer_handle_relative_motion(void* data, struct zwp_relative_pointer_v1* zwp_relative_pointer_v1, uint32_t utime_hi, uint32_t utime_lo, wl_fixed_t dx, wl_fixed_t dy, wl_fixed_t dx_unaccel, wl_fixed_t dy_unaccel);
static const struct zwp_relative_pointer_v1_listener relative_pointer_listener = { relative_pointer_handle_relative_motion };

static void data_source_handle_target(void* data, struct wl_data_source* wl_data_source, const char* mime_type) {}
static void data_source_handle_send(void* data, struct wl_data_source* wl_data_source, const char* mime_type, int32_t fd);
static void data_source_handle_cancelled(void* data, struct wl_data_source* wl_data_source);
static void data_source_handle_dnd_drop_performed(void* data, struct wl_data_source* wl_data_source) {}
static void data_source_handle_dnd_finished(void* data, struct wl_data_source* wl_data_source) {}
static void data_source_handle_action(void* data, struct wl_data_source* wl_data_source, uint32_t dnd_action) {}
static const struct wl_data_source_listener data_source_listener = { data_source_handle_target, data_source_handle_send, data_source_handle_cancelled, data_source_handle_dnd_drop_performed, data_source_handle_dnd_finished, data_source_handle_action };

WaylandDisplayBackend::WaylandDisplayBackend()
{
	fprintf(stderr, "WaylandDisplayBackend: Connecting to display...\n");
	s_waylandDisplay = wl_display_connect(NULL);
	if (!s_waylandDisplay) throw std::runtime_error("Could not connect to Wayland display");
	fprintf(stderr, "WaylandDisplayBackend: Connected (display=%p).\n", s_waylandDisplay);

	fprintf(stderr, "WaylandDisplayBackend: Getting registry...\n");
	s_waylandRegistry = wl_display_get_registry(s_waylandDisplay);
	fprintf(stderr, "WaylandDisplayBackend: Registry pointer: %p\n", s_waylandRegistry);
	
	fprintf(stderr, "WaylandDisplayBackend: Adding registry listener...\n");
	wl_registry_add_listener(s_waylandRegistry, &registry_listener, this);
	fprintf(stderr, "WaylandDisplayBackend: Listener added.\n");

	fprintf(stderr, "WaylandDisplayBackend: Dispatching roundtrip 1...\n");
	wl_display_roundtrip(s_waylandDisplay);
	fprintf(stderr, "WaylandDisplayBackend: Roundtrip 1 done.\n");
	
	fprintf(stderr, "WaylandDisplayBackend: Dispatching roundtrip 2...\n");
	wl_display_roundtrip(s_waylandDisplay);
	fprintf(stderr, "WaylandDisplayBackend: Roundtrip 2 done.\n");

	fprintf(stderr, "WaylandDisplayBackend: Creating XKB context...\n");
	m_KeymapContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	fprintf(stderr, "WaylandDisplayBackend: XKB context created (%p).\n", m_KeymapContext);
}

WaylandDisplayBackend::~WaylandDisplayBackend()
{
	if (m_cursorBuffer) wl_buffer_destroy(m_cursorBuffer);
	if (m_cursorSurface) wl_surface_destroy(m_cursorSurface);
	if (m_cursorTheme) wl_cursor_theme_destroy(m_cursorTheme);
	if (m_DataSource) wl_data_source_destroy(m_DataSource);
	if (m_DataDevice) wl_data_device_destroy(m_DataDevice);
	if (m_DataDeviceManager) wl_data_device_manager_destroy(m_DataDeviceManager);
	if (m_XDGOutputManager) zxdg_output_manager_v1_destroy(m_XDGOutputManager);
	if (m_XDGDecorationManager) zxdg_decoration_manager_v1_destroy(m_XDGDecorationManager);
	if (m_FractionalScaleManager) wp_fractional_scale_manager_v1_destroy(m_FractionalScaleManager);
	if (m_PointerConstraints) zwp_pointer_constraints_v1_destroy(m_PointerConstraints);
	if (m_RelativePointerManager) zwp_relative_pointer_manager_v1_destroy(m_RelativePointerManager);
	if (m_XDGWMBase) xdg_wm_base_destroy(m_XDGWMBase);
	if (m_waylandSHM) wl_shm_destroy(m_waylandSHM);
	if (m_waylandCompositor) wl_compositor_destroy(m_waylandCompositor);
	if (m_waylandSeat) wl_seat_destroy(m_waylandSeat);
	if (s_waylandRegistry) wl_registry_destroy(s_waylandRegistry);
	if (s_waylandDisplay) wl_display_disconnect(s_waylandDisplay);

	if (m_KeyboardState) WAYLAND->p_xkb_state_unref(m_KeyboardState);
	if (m_Keymap) WAYLAND->p_xkb_keymap_unref(m_Keymap);
	if (m_KeymapContext) WAYLAND->p_xkb_context_unref(m_KeymapContext);
}


#ifdef USE_DBUS
// Portal dialogs identify their parent window by an exported handle. On Wayland
// that comes from xdg-foreign, which is why the backend exports the surface.
std::unique_ptr<OpenFileDialog> WaylandDisplayBackend::CreateOpenFileDialog(DisplayWindow* owner)
{
	std::string ownerHandle;
	if (owner)
		ownerHandle = "wayland:" + static_cast<WaylandDisplayWindow*>(owner)->GetWaylandWindowID();
	return std::make_unique<DBusOpenFileDialog>(ownerHandle);
}

std::unique_ptr<SaveFileDialog> WaylandDisplayBackend::CreateSaveFileDialog(DisplayWindow* owner)
{
	std::string ownerHandle;
	if (owner)
		ownerHandle = "wayland:" + static_cast<WaylandDisplayWindow*>(owner)->GetWaylandWindowID();
	return std::make_unique<DBusSaveFileDialog>(ownerHandle);
}

std::unique_ptr<OpenFolderDialog> WaylandDisplayBackend::CreateOpenFolderDialog(DisplayWindow* owner)
{
	std::string ownerHandle;
	if (owner)
		ownerHandle = "wayland:" + static_cast<WaylandDisplayWindow*>(owner)->GetWaylandWindowID();
	return std::make_unique<DBusOpenFolderDialog>(ownerHandle);
}
#endif

std::unique_ptr<DisplayWindow> WaylandDisplayBackend::Create(DisplayWindowHost* windowHost, WidgetType type, DisplayWindow* owner, RenderAPI renderAPI)
{
	return std::make_unique<WaylandDisplayWindow>(this, windowHost, type, (WaylandDisplayWindow*)owner, renderAPI);
}

// Modifiers and lock keys must never repeat. xkb_keymap_key_repeats() would
// answer this per-key from the keymap, but it is not among the symbols resolved
// in wayland_dynamic.h, so exclude the known non-repeating keys directly.
static bool IsRepeatableKey(InputKey key)
{
	switch (key)
	{
	case InputKey::Shift: case InputKey::LShift: case InputKey::RShift:
	case InputKey::Ctrl: case InputKey::LControl: case InputKey::RControl:
	case InputKey::Alt:
	case InputKey::CapsLock: case InputKey::NumLock: case InputKey::ScrollLock:
	case InputKey::None:
		return false;
	default:
		return true;
	}
}

static int64_t NowMilliseconds()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();
}

void WaylandDisplayBackend::StartKeyRepeat(uint32_t scancode, InputKey key)
{
	if (m_CompositorSendsRepeat || m_RepeatRate <= 0 || !IsRepeatableKey(key))
	{
		StopKeyRepeat();
		return;
	}
	m_RepeatScancode = scancode;
	m_RepeatKey = key;
	m_RepeatNextMs = NowMilliseconds() + m_RepeatDelay;
}

void WaylandDisplayBackend::UpdateKeyRepeat()
{
	if (m_RepeatScancode == 0 || !m_ActiveWindow || m_RepeatRate <= 0)
		return;

	const int64_t now = NowMilliseconds();
	if (now < m_RepeatNextMs)
		return;

	const int64_t interval = 1000 / m_RepeatRate;

	// Emit at most a handful per pump. If the process was stalled (a level
	// load, say) we must not replay the whole backlog as a burst of keys.
	int emitted = 0;
	while (now >= m_RepeatNextMs && emitted < 4)
	{
		m_ActiveWindow->windowHost->OnWindowKeyDown(m_RepeatKey);

		char buffer[32];
		if (WAYLAND->p_xkb_state_key_get_utf8(m_KeyboardState, m_RepeatScancode, buffer, sizeof(buffer)) > 0)
		{
			// The window may have been destroyed by the key handler above.
			if (!m_ActiveWindow)
				return;
			m_ActiveWindow->windowHost->OnWindowKeyChar(buffer);
		}

		m_RepeatNextMs += interval > 0 ? interval : 40;
		emitted++;

		if (!m_ActiveWindow || m_RepeatScancode == 0)
			return;
	}

	if (now > m_RepeatNextMs)
		m_RepeatNextMs = now + (interval > 0 ? interval : 40);
}

void WaylandDisplayBackend::ProcessEvents()
{
	UpdateKeyRepeat();

	while (wl_display_prepare_read(s_waylandDisplay) != 0)
	{
		wl_display_dispatch_pending(s_waylandDisplay);
	}

	if (wl_display_flush(s_waylandDisplay) < 0)
	{
		wl_display_cancel_read(s_waylandDisplay);
		return;
	}

	if (poll_single(wl_display_get_fd(s_waylandDisplay), POLLIN, 0) & POLLIN)
	{
		wl_display_read_events(s_waylandDisplay);
		wl_display_dispatch_pending(s_waylandDisplay);
	}
	else
	{
		wl_display_cancel_read(s_waylandDisplay);
	}
}

void WaylandDisplayBackend::RunLoop()
{
	while (!exitRunLoop)
	{
		UpdateTimers();
		ProcessEvents();
		CheckNeedsUpdate();
		if (!exitRunLoop)
		{
			int timeout = GetTimerTimeout();
			WaitForEvents(timeout);
		}
	}
}

void WaylandDisplayBackend::ExitLoop()
{
	exitRunLoop = true;
}

void WaylandDisplayBackend::WaitForEvents(int timeout)
{
	struct pollfd pfd = { wl_display_get_fd(s_waylandDisplay), POLLIN, 0 };
	poll(&pfd, 1, timeout);
}

void WaylandDisplayBackend::CheckNeedsUpdate()
{
	for (auto window : s_Windows)
	{
		if (window->m_NeedsUpdate)
		{
			window->m_NeedsUpdate = false;
			window->windowHost->OnWindowPaint();
		}
	}
}

void WaylandDisplayBackend::UpdateTimers()
{
	auto now = std::chrono::steady_clock::now();
	auto it = m_timers.begin();
	while (it != m_timers.end())
	{
		if (now >= std::chrono::steady_clock::time_point(std::chrono::milliseconds((*it)->nextTime)))
		{
			(*it)->onTimer();
			(*it)->nextTime = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() + (*it)->timeoutMilliseconds;
		}
		it++;
	}
}

int WaylandDisplayBackend::GetTimerTimeout()
{
	if (m_timers.empty()) return -1;
	auto now = std::chrono::steady_clock::now();
	int64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
	int64_t minWait = 1000000;
	for (auto timer : m_timers)
	{
		int64_t wait = timer->nextTime - nowMs;
		if (wait < minWait) minWait = wait;
	}
	if (minWait < 0) return 0;
	return (int)minWait;
}

void* WaylandDisplayBackend::StartTimer(int timeoutMilliseconds, std::function<void()> onTimer)
{
	auto now = std::chrono::steady_clock::now();
	int64_t nextTime = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() + timeoutMilliseconds;
	auto timer = std::make_shared<WaylandTimer>(timeoutMilliseconds, onTimer, nextTime);
	m_timers.push_back(timer);
	return timer.get();
}

void WaylandDisplayBackend::StopTimer(void* timerID)
{
	auto it = std::find_if(m_timers.begin(), m_timers.end(), [timerID](const std::shared_ptr<WaylandTimer>& t) { return t.get() == timerID; });
	if (it != m_timers.end()) m_timers.erase(it);
}

Size WaylandDisplayBackend::GetScreenSize()
{
	return s_ScreenSize;
}

void WaylandDisplayBackend::OnWindowCreated(WaylandDisplayWindow* window)
{
	s_Windows.push_back(window);
}

void WaylandDisplayBackend::OnWindowDestroyed(WaylandDisplayWindow* window)
{
	auto it = std::find(s_Windows.begin(), s_Windows.end(), window);
	if (it != s_Windows.end()) s_Windows.erase(it);

	// The backend caches raw pointers to the focused/hovered windows, and input
	// events keep arriving after a window is gone -- notably the key release
	// that follows the key press which closed it (Enter on the launcher). If
	// these are not cleared, the next event dereferences freed memory.
	if (m_ActiveWindow == window)
	{
		m_ActiveWindow = nullptr;
		StopKeyRepeat();
		// Held-key bookkeeping belonged to that window; nothing can release
		// these now, so drop them rather than replay them at the next window.
		m_PressedScancodes.clear();
		inputKeyStates.clear();
	}
	if (m_FocusWindow == window) m_FocusWindow = nullptr;
	if (m_MouseFocusWindow == window) m_MouseFocusWindow = nullptr;
	if (m_HoverWindow == window) m_HoverWindow = nullptr;
}

void WaylandDisplayBackend::SetCursor(StandardCursor cursor)
{
    // Implementation for cursor setting using wp_cursor_shape_v1 or wl_cursor
}

void WaylandDisplayBackend::ShowCursor(bool enable)
{
}

bool WaylandDisplayBackend::GetKeyState(InputKey key)
{
	return inputKeyStates[key];
}

void WaylandDisplayBackend::SetClipboardText(const std::string& text)
{
	m_ClipboardText = text;
	if (m_DataSource) wl_data_source_destroy(m_DataSource);
	m_DataSource = wl_data_device_manager_create_data_source(m_DataDeviceManager);
	wl_data_source_add_listener(m_DataSource, &data_source_listener, this);
	wl_data_source_offer(m_DataSource, "text/plain;charset=utf-8");
	wl_data_device_set_selection(m_DataDevice, m_DataSource, m_KeyboardSerial);
}

std::string WaylandDisplayBackend::GetClipboardText()
{
	return m_ClipboardContents;
}

void registry_handle_global(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version)
{
	WaylandDisplayBackend* backend = (WaylandDisplayBackend*)data;
	if (strcmp(interface, "wl_compositor") == 0) backend->m_waylandCompositor = (struct wl_compositor*)wl_registry_bind(registry, name, &wl_compositor_interface, 4);
	else if (strcmp(interface, "wl_shm") == 0) backend->m_waylandSHM = (struct wl_shm*)wl_registry_bind(registry, name, &wl_shm_interface, 1);
	else if (strcmp(interface, "wl_output") == 0) {
		backend->m_waylandOutput = (struct wl_output*)wl_registry_bind(registry, name, &wl_output_interface, 3);
		wl_output_add_listener(backend->m_waylandOutput, &output_listener, backend);
	}
	else if (strcmp(interface, "wl_seat") == 0) {
		backend->m_waylandSeat = (struct wl_seat*)wl_registry_bind(registry, name, &wl_seat_interface, 8);
		wl_seat_add_listener(backend->m_waylandSeat, &seat_listener, backend);
	}
	else if (strcmp(interface, "wl_data_device_manager") == 0) {
		backend->m_DataDeviceManager = (struct wl_data_device_manager*)wl_registry_bind(registry, name, &wl_data_device_manager_interface, 3);
		if (backend->m_waylandSeat) {
			backend->m_DataDevice = wl_data_device_manager_get_data_device(backend->m_DataDeviceManager, backend->m_waylandSeat);
			wl_data_device_add_listener(backend->m_DataDevice, &data_device_listener, backend);
		}
	}
	else if (strcmp(interface, "xdg_wm_base") == 0) {
		backend->m_XDGWMBase = (struct xdg_wm_base*)wl_registry_bind(registry, name, &xdg_wm_base_interface, 4);
		xdg_wm_base_add_listener(backend->m_XDGWMBase, &xdg_wm_base_listener, backend);
	}
	else if (strcmp(interface, "zxdg_output_manager_v1") == 0) {
		backend->m_XDGOutputManager = (struct zxdg_output_manager_v1*)wl_registry_bind(registry, name, &zxdg_output_manager_v1_interface, 3);
		if (backend->m_waylandOutput) {
			backend->m_XDGOutput = zxdg_output_manager_v1_get_xdg_output(backend->m_XDGOutputManager, backend->m_waylandOutput);
			zxdg_output_v1_add_listener(backend->m_XDGOutput, &xdg_output_listener, backend);
		}
	}
	else if (strcmp(interface, "wp_fractional_scale_manager_v1") == 0) backend->m_FractionalScaleManager = (struct wp_fractional_scale_manager_v1*)wl_registry_bind(registry, name, &wp_fractional_scale_manager_v1_interface, 1);
	else if (strcmp(interface, "zxdg_decoration_manager_v1") == 0) backend->m_XDGDecorationManager = (struct zxdg_decoration_manager_v1*)wl_registry_bind(registry, name, &zxdg_decoration_manager_v1_interface, 1);
	else if (strcmp(interface, "zwp_pointer_constraints_v1") == 0) backend->m_PointerConstraints = (struct zwp_pointer_constraints_v1*)wl_registry_bind(registry, name, &zwp_pointer_constraints_v1_interface, 1);
	else if (strcmp(interface, "zwp_relative_pointer_manager_v1") == 0) backend->m_RelativePointerManager = (struct zwp_relative_pointer_manager_v1*)wl_registry_bind(registry, name, &zwp_relative_pointer_manager_v1_interface, 1);
	else if (strcmp(interface, "xdg_toplevel_icon_manager_v1") == 0) backend->m_XDGToplevelIconManager = (struct xdg_toplevel_icon_manager_v1*)wl_registry_bind(registry, name, &xdg_toplevel_icon_manager_v1_interface, 1);
	else if (strcmp(interface, "wp_cursor_shape_manager_v1") == 0) {
		backend->m_CursorShapeManager = (struct wp_cursor_shape_manager_v1*)wl_registry_bind(registry, name, &wp_cursor_shape_manager_v1_interface, 1);
	}
	else if (strcmp(interface, "xdg_wm_dialog_v1") == 0) backend->m_XDGWMDialog = (struct xdg_wm_dialog_v1*)wl_registry_bind(registry, name, &xdg_wm_dialog_v1_interface, 1);
}


void seat_handle_capabilities(void* data, struct wl_seat* seat, uint32_t caps)
{
	WaylandDisplayBackend* backend = (WaylandDisplayBackend*)data;
	if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !backend->m_waylandKeyboard) {
		backend->m_waylandKeyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_add_listener(backend->m_waylandKeyboard, &keyboard_listener, backend);
	}
	else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && backend->m_waylandKeyboard) {
		wl_keyboard_destroy(backend->m_waylandKeyboard);
		backend->m_waylandKeyboard = NULL;
	}

	if ((caps & WL_SEAT_CAPABILITY_POINTER) && !backend->m_waylandPointer) {
		backend->m_waylandPointer = wl_seat_get_pointer(seat);
		wl_pointer_add_listener(backend->m_waylandPointer, &pointer_listener, backend);
		if (backend->m_RelativePointerManager) {
			backend->m_RelativePointer = zwp_relative_pointer_manager_v1_get_relative_pointer(backend->m_RelativePointerManager, backend->m_waylandPointer);
			zwp_relative_pointer_v1_add_listener(backend->m_RelativePointer, &relative_pointer_listener, backend);
		}
	}
	else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && backend->m_waylandPointer) {
		if (backend->m_RelativePointer) zwp_relative_pointer_v1_destroy(backend->m_RelativePointer);
		backend->m_RelativePointer = NULL;
		wl_pointer_destroy(backend->m_waylandPointer);
		backend->m_waylandPointer = NULL;
	}
}

void output_handle_mode(void* data, struct wl_output* output, uint32_t flags, int32_t width, int32_t height, int32_t refresh)
{
	WaylandDisplayBackend* backend = (WaylandDisplayBackend*)data;
	if (flags & WL_OUTPUT_MODE_CURRENT) {
		backend->s_ScreenSize = Size(width, height);
	}
}

void output_handle_scale(void* data, struct wl_output* output, int32_t factor)
{
	WaylandDisplayBackend* backend = (WaylandDisplayBackend*)data;
	backend->s_DpiScale = factor;
}

void xdg_output_handle_logical_size(void* data, struct zxdg_output_v1* zxdg_output_v1, int32_t width, int32_t height)
{
	WaylandDisplayBackend* backend = (WaylandDisplayBackend*)data;
	backend->s_ScreenSize = Size(width, height);
}

void keyboard_handle_keymap(void* data, struct wl_keyboard* wl_keyboard, uint32_t format, int32_t fd, uint32_t size)
{
	WaylandDisplayBackend* backend = (WaylandDisplayBackend*)data;
	char* map_str = (char*)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (map_str != MAP_FAILED) {
		if (backend->m_Keymap) WAYLAND->p_xkb_keymap_unref(backend->m_Keymap);
		backend->m_Keymap = WAYLAND->p_xkb_keymap_new_from_string(backend->m_KeymapContext, map_str, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
		munmap(map_str, size);
		if (backend->m_KeyboardState) WAYLAND->p_xkb_state_unref(backend->m_KeyboardState);
		backend->m_KeyboardState = WAYLAND->p_xkb_state_new(backend->m_Keymap);
	}
	close(fd);
}
void keyboard_handle_enter(void* data, struct wl_keyboard* wl_keyboard, uint32_t serial, struct wl_surface* surface, struct wl_array* keys)
{
	WaylandDisplayBackend* backend = (WaylandDisplayBackend*)data;
	for (auto window : backend->s_Windows)
	{
		if (window->m_AppSurface == surface)
		{
			backend->m_ActiveWindow = window;
			window->windowHost->OnWindowActivated();

			// The compositor reports which keys are already held at focus-in.
			// Register them so their eventual release has a matching press to
			// resolve against, and so GetKeyState() agrees with reality.
			if (keys)
			{
				// wl_array_for_each() relies on an implicit void* conversion
				// that is not valid in C++, so walk the array by hand.
				uint32_t* first = (uint32_t*)keys->data;
				uint32_t* last = (uint32_t*)((const char*)keys->data + keys->size);
				for (uint32_t* key = first; key < last; key++)
				{
					uint32_t scancode = *key + 8;
					xkb_keysym_t sym = WAYLAND->p_xkb_state_key_get_one_sym(backend->m_KeyboardState, scancode);
					InputKey ik = backend->XKBKeySymToInputKey(sym);
					backend->m_PressedScancodes[scancode] = ik;
					backend->inputKeyStates[ik] = true;
					window->windowHost->OnWindowKeyDown(ik);
				}
			}
			break;
		}
	}
}

void keyboard_handle_leave(void* data, struct wl_keyboard* wl_keyboard, uint32_t serial, struct wl_surface* surface)
{
	WaylandDisplayBackend* backend = (WaylandDisplayBackend*)data;
	if (backend->m_ActiveWindow && backend->m_ActiveWindow->m_AppSurface == surface) {
		// Per protocol, focus loss releases every held key -- the compositor
		// will not send the individual key-up events. Synthesize them, or the
		// client keeps keys latched down until something else resets state.
		//
		// Snapshot and clear before dispatching: a key-up handler is free to
		// destroy the window, which clears m_PressedScancodes and nulls
		// m_ActiveWindow underneath us. Re-check the window each iteration.
		backend->StopKeyRepeat();

		std::map<uint32_t, InputKey> held;
		held.swap(backend->m_PressedScancodes);
		for (const auto& entry : held)
		{
			backend->inputKeyStates[entry.second] = false;
			if (!backend->m_ActiveWindow)
				break;
			backend->m_ActiveWindow->windowHost->OnWindowKeyUp(entry.second);
		}

		if (backend->m_ActiveWindow)
		{
			backend->m_ActiveWindow->windowHost->OnWindowDeactivated();
			backend->m_ActiveWindow = nullptr;
		}
	}
}

void keyboard_handle_key(void* data, struct wl_keyboard* wl_keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
	WaylandDisplayBackend* backend = (WaylandDisplayBackend*)data;
	backend->m_KeyboardSerial = serial;
	uint32_t scancode = key + 8;

	// wl_keyboard v10 adds a third state, "repeated". Treat it as a press for
	// event delivery, but never as a state transition -- the key was already
	// down and is still down.
	const bool pressed = (state == WL_KEYBOARD_KEY_STATE_PRESSED);
	// wl_keyboard v10 (wayland 1.23) added a "repeated" key state. Older
	// libwayland headers -- Ubuntu 22.04's, for one -- predate it, so compare
	// against the protocol value rather than the enum constant, which may not
	// be declared. Defining the name ourselves is not an option: where the
	// header does declare it, a macro of the same name would corrupt the enum.
	constexpr uint32_t kKeyStateRepeated = 2;
	const bool repeated = (state == kKeyStateRepeated);

	InputKey ik;
	if (pressed || repeated)
	{
		auto it = backend->m_PressedScancodes.find(scancode);
		if (it != backend->m_PressedScancodes.end())
		{
			// Repeat, or a press we never saw released. Reuse the original
			// mapping so the eventual release matches.
			ik = it->second;
		}
		else
		{
			xkb_keysym_t sym = WAYLAND->p_xkb_state_key_get_one_sym(backend->m_KeyboardState, scancode);
			ik = backend->XKBKeySymToInputKey(sym);
			backend->m_PressedScancodes[scancode] = ik;
		}
		backend->inputKeyStates[ik] = true;

		if (repeated)
		{
			// The compositor is generating repeats itself, so stand down.
			backend->m_CompositorSendsRepeat = true;
			backend->StopKeyRepeat();
		}
		else
		{
			// Wayland repeats only the most recently pressed key.
			backend->StartKeyRepeat(scancode, ik);
		}
	}
	else
	{
		auto it = backend->m_PressedScancodes.find(scancode);
		if (it != backend->m_PressedScancodes.end())
		{
			ik = it->second;
			backend->m_PressedScancodes.erase(it);
		}
		else
		{
			// Release without a matching press (focus was gained while the key
			// was already held, and enter did not report it). Fall back to
			// resolving now; better an approximate key-up than none.
			xkb_keysym_t sym = WAYLAND->p_xkb_state_key_get_one_sym(backend->m_KeyboardState, scancode);
			ik = backend->XKBKeySymToInputKey(sym);
		}
		backend->inputKeyStates[ik] = false;

		if (backend->m_RepeatScancode == scancode)
			backend->StopKeyRepeat();
	}

	if (backend->m_ActiveWindow) {
		if (pressed || repeated) {
			backend->m_ActiveWindow->windowHost->OnWindowKeyDown(ik);
			// Text input is intentionally modifier- and layout-dependent, so
			// this stays on the live xkb state.
			char buffer[32];
			if (WAYLAND->p_xkb_state_key_get_utf8(backend->m_KeyboardState, scancode, buffer, sizeof(buffer)) > 0) {
				backend->m_ActiveWindow->windowHost->OnWindowKeyChar(buffer);
			}
		}
		else {
			backend->m_ActiveWindow->windowHost->OnWindowKeyUp(ik);
		}
	}
}

void keyboard_handle_modifiers(void* data, struct wl_keyboard* wl_keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
	WaylandDisplayBackend* backend = (WaylandDisplayBackend*)data;
	WAYLAND->p_xkb_state_update_mask(backend->m_KeyboardState, mods_depressed, mods_latched, mods_locked, 0, 0, group);
}

void keyboard_handle_repeat_info(void* data, struct wl_keyboard* wl_keyboard, int32_t rate, int32_t delay)
{
	// rate is repeats per second, delay the milliseconds before the first one.
	// A rate of zero means the compositor wants repeat disabled entirely.
	WaylandDisplayBackend* backend = (WaylandDisplayBackend*)data;
	backend->m_RepeatRate = rate;
	backend->m_RepeatDelay = delay;
	if (rate <= 0)
		backend->StopKeyRepeat();
}

void pointer_handle_enter(void* data, struct wl_pointer* wl_pointer, uint32_t serial, struct wl_surface* surface, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
	WaylandDisplayBackend* backend = (WaylandDisplayBackend*)data;
	backend->m_PointerSerial = serial;
	for (auto window : backend->s_Windows) {
		if (window->m_AppSurface == surface) {
			backend->m_HoverWindow = window;
			break;
		}
	}
}

void pointer_handle_leave(void* data, struct wl_pointer* wl_pointer, uint32_t serial, struct wl_surface* surface)
{
	WaylandDisplayBackend* backend = (WaylandDisplayBackend*)data;
	if (backend->m_HoverWindow && backend->m_HoverWindow->m_AppSurface == surface) {
		backend->m_HoverWindow->windowHost->OnWindowMouseLeave();
		backend->m_HoverWindow = NULL;
	}
}

void pointer_handle_motion(void* data, struct wl_pointer* wl_pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
	WaylandDisplayBackend* backend = (WaylandDisplayBackend*)data;
	if (backend->m_HoverWindow) {
        backend->m_HoverWindow->m_SurfaceMousePos = Point(WAYLAND_wl_fixed_to_double(surface_x), WAYLAND_wl_fixed_to_double(surface_y));
		backend->m_HoverWindow->windowHost->OnWindowMouseMove(backend->m_HoverWindow->m_SurfaceMousePos);
	}
}

void pointer_handle_button(void* data, struct wl_pointer* wl_pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
	WaylandDisplayBackend* backend = (WaylandDisplayBackend*)data;
	backend->m_PointerSerial = serial;
	InputKey ik = InputKey::None;
	if (button == 0x110) ik = InputKey::LeftMouse;
	else if (button == 0x111) ik = InputKey::RightMouse;
	else if (button == 0x112) ik = InputKey::MiddleMouse;

	if (backend->m_HoverWindow && ik != InputKey::None) {
		if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
            backend->m_HoverWindow->windowHost->OnWindowMouseDown(backend->m_HoverWindow->m_SurfaceMousePos, ik);
        }
		else {
            backend->m_HoverWindow->windowHost->OnWindowMouseUp(backend->m_HoverWindow->m_SurfaceMousePos, ik);
        }
	}
}

void pointer_handle_axis(void* data, struct wl_pointer* wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
{
	WaylandDisplayBackend* backend = (WaylandDisplayBackend*)data;
	if (backend->m_HoverWindow) {
		InputKey ik = (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) ? (WAYLAND_wl_fixed_to_double(value) > 0 ? InputKey::MouseWheelDown : InputKey::MouseWheelUp) : (WAYLAND_wl_fixed_to_double(value) > 0 ? InputKey::None : InputKey::None);
		backend->m_HoverWindow->windowHost->OnWindowMouseWheel(backend->m_HoverWindow->m_SurfaceMousePos, ik);
	}
}

void pointer_handle_frame(void* data, struct wl_pointer* wl_pointer) {}

void relative_pointer_handle_relative_motion(void* data, struct zwp_relative_pointer_v1* zwp_relative_pointer_v1, uint32_t utime_hi, uint32_t utime_lo, wl_fixed_t dx, wl_fixed_t dy, wl_fixed_t dx_unaccel, wl_fixed_t dy_unaccel)
{
	WaylandDisplayBackend* backend = (WaylandDisplayBackend*)data;
	if (backend->m_ActiveWindow) {
		backend->m_ActiveWindow->windowHost->OnWindowRawMouseMove(wl_fixed_to_int_hack(dx), wl_fixed_to_int_hack(dy));
	}
}

void data_offer_handle_offer(void* data, struct wl_data_offer* wl_data_offer, const char* mime_type)
{
	WaylandDisplayBackend* backend = (WaylandDisplayBackend*)data;
	if (strcmp(mime_type, "text/plain;charset=utf-8") == 0 || strcmp(mime_type, "text/plain") == 0) {
		backend->m_ClipboardMimeType = mime_type;
	}
}

void data_device_handle_data_offer(void* data, struct wl_data_device* wl_data_device, struct wl_data_offer* id)
{
	WaylandDisplayBackend* backend = (WaylandDisplayBackend*)data;
	wl_data_offer_add_listener(id, &data_offer_listener, backend);
}

void data_device_handle_selection(void* data, struct wl_data_device* wl_data_device, struct wl_data_offer* id)
{
	WaylandDisplayBackend* backend = (WaylandDisplayBackend*)data;
	if (!id) {
		backend->m_ClipboardContents = "";
		return;
	}
	int fds[2];
	pipe(fds);
	wl_data_offer_receive(id, backend->m_ClipboardMimeType.c_str(), fds[1]);
	close(fds[1]);
	wl_display_roundtrip(backend->s_waylandDisplay);
	char buffer[4096];
	int n = read(fds[0], buffer, sizeof(buffer));
	if (n > 0) backend->m_ClipboardContents = std::string(buffer, n);
	close(fds[0]);
}

void data_source_handle_send(void* data, struct wl_data_source* wl_data_source, const char* mime_type, int32_t fd)
{
	WaylandDisplayBackend* backend = (WaylandDisplayBackend*)data;
	if (strcmp(mime_type, "text/plain;charset=utf-8") == 0) {
		write(fd, backend->m_ClipboardText.c_str(), backend->m_ClipboardText.length());
	}
	close(fd);
}

void data_source_handle_cancelled(void* data, struct wl_data_source* wl_data_source)
{
	WaylandDisplayBackend* backend = (WaylandDisplayBackend*)data;
	if (backend->m_DataSource == wl_data_source) {
		wl_data_source_destroy(backend->m_DataSource);
		backend->m_DataSource = NULL;
	}
}


extern "C" {

struct wl_display* wl_display_connect(const char* name) { return WAYLAND->p_display_connect(name); }
void wl_display_disconnect(struct wl_display* display) { WAYLAND->p_display_disconnect(display); }
int wl_display_dispatch(struct wl_display* display) { return WAYLAND->p_display_dispatch(display); }
int wl_display_dispatch_pending(struct wl_display* display) { return WAYLAND->p_display_dispatch_pending(display); }
int wl_display_flush(struct wl_display* display) { return WAYLAND->p_display_flush(display); }
int wl_display_roundtrip(struct wl_display* display) { return WAYLAND->p_display_roundtrip(display); }
int wl_display_get_fd(struct wl_display* display) { return WAYLAND->p_display_get_fd(display); }
int wl_display_prepare_read(struct wl_display* display) { return WAYLAND->p_display_prepare_read(display); }
int wl_display_read_events(struct wl_display* display) { return WAYLAND->p_display_read_events(display); }
void wl_display_cancel_read(struct wl_display* display) { WAYLAND->p_display_cancel_read(display); }
int wl_display_get_error(struct wl_display* display) { return WAYLAND->p_display_get_error(display); }

int wl_proxy_add_listener(struct wl_proxy* proxy, void (**implementation)(void), void* data) { return WAYLAND->p_proxy_add_listener(proxy, implementation, data); }
void wl_proxy_destroy(struct wl_proxy* proxy) { WAYLAND->p_proxy_destroy(proxy); }
void wl_proxy_set_queue(struct wl_proxy* proxy, struct wl_event_queue* queue) { WAYLAND->p_proxy_set_queue(proxy, queue); }
void wl_proxy_set_user_data(struct wl_proxy* proxy, void* user_data) { WAYLAND->p_proxy_set_user_data(proxy, user_data); }
void* wl_proxy_get_user_data(struct wl_proxy* proxy) { return WAYLAND->p_proxy_get_user_data(proxy); }
uint32_t wl_proxy_get_version(struct wl_proxy* proxy) { return WAYLAND->p_proxy_get_version(proxy); }

struct wl_event_queue* wl_display_create_queue(struct wl_display* display) { return WAYLAND->p_display_create_queue(display); }
void wl_event_queue_destroy(struct wl_event_queue* queue) { WAYLAND->p_event_queue_destroy(queue); }

struct wl_cursor_theme* wl_cursor_theme_load(const char* name, int size, struct wl_shm* shm) { return WAYLAND->p_cursor_theme_load(name, size, shm); }
void wl_cursor_theme_destroy(struct wl_cursor_theme* theme) { WAYLAND->p_cursor_theme_destroy(theme); }
struct wl_cursor* wl_cursor_theme_get_cursor(struct wl_cursor_theme* theme, const char* name) { return WAYLAND->p_cursor_theme_get_cursor(theme, name); }
struct wl_buffer* wl_cursor_image_get_buffer(struct wl_cursor_image* image) { return WAYLAND->p_cursor_image_get_buffer(image); }

void* gp_wl_proxy_marshal = nullptr;
void* gp_wl_proxy_marshal_constructor = nullptr;
void* gp_wl_proxy_marshal_constructor_versioned = nullptr;
void* gp_wl_proxy_marshal_flags = nullptr;

__attribute__((naked)) void wl_proxy_marshal(struct wl_proxy* proxy, uint32_t opcode, ...)
{
	__asm__(
		"movq gp_wl_proxy_marshal(%rip), %r11\n"
		"jmp *%r11\n"
	);
}

__attribute__((naked)) struct wl_proxy* wl_proxy_marshal_constructor(struct wl_proxy* proxy, uint32_t opcode, const struct wl_interface* interface, ...)
{
	__asm__(
		"movq gp_wl_proxy_marshal_constructor(%rip), %r11\n"
		"jmp *%r11\n"
	);
}

__attribute__((naked)) struct wl_proxy* wl_proxy_marshal_constructor_versioned(struct wl_proxy* proxy, uint32_t opcode, const struct wl_interface* interface, uint32_t version, ...)
{
	__asm__(
		"movq gp_wl_proxy_marshal_constructor_versioned(%rip), %r11\n"
		"jmp *%r11\n"
	);
}

__attribute__((naked)) struct wl_proxy* wl_proxy_marshal_flags(struct wl_proxy* proxy, uint32_t opcode, const struct wl_interface* interface, uint32_t version, uint32_t flags, ...)
{
	__asm__(
		"movq gp_wl_proxy_marshal_flags(%rip), %r11\n"
		"jmp *%r11\n"
	);
}

} // extern "C"

InputKey WaylandDisplayBackend::XKBKeySymToInputKey(xkb_keysym_t keySym)
{
	switch (keySym)
	{
	case XKB_KEY_Escape: return InputKey::Escape;
	case XKB_KEY_Return: return InputKey::Enter;
	case XKB_KEY_BackSpace: return InputKey::Backspace;
	case XKB_KEY_Tab: return InputKey::Tab;
	case XKB_KEY_space: return InputKey::Space;
	case XKB_KEY_Left: return InputKey::Left;
	case XKB_KEY_Right: return InputKey::Right;
	case XKB_KEY_Up: return InputKey::Up;
	case XKB_KEY_Down: return InputKey::Down;
	case XKB_KEY_0: return InputKey::_0;
	case XKB_KEY_1: return InputKey::_1;
	case XKB_KEY_2: return InputKey::_2;
	case XKB_KEY_3: return InputKey::_3;
	case XKB_KEY_4: return InputKey::_4;
	case XKB_KEY_5: return InputKey::_5;
	case XKB_KEY_6: return InputKey::_6;
	case XKB_KEY_7: return InputKey::_7;
	case XKB_KEY_8: return InputKey::_8;
	case XKB_KEY_9: return InputKey::_9;

	// Shifted digits. The keysym is resolved once, at press time, so a digit
	// pressed while Shift is held arrives as its shifted symbol; without these
	// the key is simply dead.
	case XKB_KEY_exclam: return InputKey::_1;
	case XKB_KEY_at: return InputKey::_2;
	case XKB_KEY_numbersign: return InputKey::_3;
	case XKB_KEY_dollar: return InputKey::_4;
	case XKB_KEY_percent: return InputKey::_5;
	case XKB_KEY_asciicircum: return InputKey::_6;
	case XKB_KEY_ampersand: return InputKey::_7;
	case XKB_KEY_asterisk: return InputKey::_8;
	case XKB_KEY_parenleft: return InputKey::_9;
	case XKB_KEY_parenright: return InputKey::_0;

	// OEM punctuation. None of these were mapped, which left every punctuation
	// key dead on Wayland -- including grave, which is the console toggle.
	// Both the plain and shifted keysym map to the same physical key.
	case XKB_KEY_grave: case XKB_KEY_asciitilde: case XKB_KEY_dead_tilde: return InputKey::Tilde;
	case XKB_KEY_semicolon: case XKB_KEY_colon: return InputKey::Semicolon;
	case XKB_KEY_equal: case XKB_KEY_plus: return InputKey::Equals;
	case XKB_KEY_comma: case XKB_KEY_less: return InputKey::Comma;
	case XKB_KEY_minus: case XKB_KEY_underscore: return InputKey::Minus;
	case XKB_KEY_period: case XKB_KEY_greater: return InputKey::Period;
	case XKB_KEY_slash: case XKB_KEY_question: return InputKey::Slash;
	case XKB_KEY_bracketleft: case XKB_KEY_braceleft: return InputKey::LeftBracket;
	case XKB_KEY_backslash: case XKB_KEY_bar: return InputKey::Backslash;
	case XKB_KEY_bracketright: case XKB_KEY_braceright: return InputKey::RightBracket;
	case XKB_KEY_apostrophe: case XKB_KEY_quotedbl: return InputKey::SingleQuote;

	case XKB_KEY_a: case XKB_KEY_A: return InputKey::A;
	case XKB_KEY_b: case XKB_KEY_B: return InputKey::B;
	case XKB_KEY_c: case XKB_KEY_C: return InputKey::C;
	case XKB_KEY_d: case XKB_KEY_D: return InputKey::D;
	case XKB_KEY_e: case XKB_KEY_E: return InputKey::E;
	case XKB_KEY_f: case XKB_KEY_F: return InputKey::F;
	case XKB_KEY_g: case XKB_KEY_G: return InputKey::G;
	case XKB_KEY_h: case XKB_KEY_H: return InputKey::H;
	case XKB_KEY_i: case XKB_KEY_I: return InputKey::I;
	case XKB_KEY_j: case XKB_KEY_J: return InputKey::J;
	case XKB_KEY_k: case XKB_KEY_K: return InputKey::K;
	case XKB_KEY_l: case XKB_KEY_L: return InputKey::L;
	case XKB_KEY_m: case XKB_KEY_M: return InputKey::M;
	case XKB_KEY_n: case XKB_KEY_N: return InputKey::N;
	case XKB_KEY_o: case XKB_KEY_O: return InputKey::O;
	case XKB_KEY_p: case XKB_KEY_P: return InputKey::P;
	case XKB_KEY_q: case XKB_KEY_Q: return InputKey::Q;
	case XKB_KEY_r: case XKB_KEY_R: return InputKey::R;
	case XKB_KEY_s: case XKB_KEY_S: return InputKey::S;
	case XKB_KEY_t: case XKB_KEY_T: return InputKey::T;
	case XKB_KEY_u: case XKB_KEY_U: return InputKey::U;
	case XKB_KEY_v: case XKB_KEY_V: return InputKey::V;
	case XKB_KEY_w: case XKB_KEY_W: return InputKey::W;
	case XKB_KEY_x: case XKB_KEY_X: return InputKey::X;
	case XKB_KEY_y: case XKB_KEY_Y: return InputKey::Y;
	case XKB_KEY_z: case XKB_KEY_Z: return InputKey::Z;
	case XKB_KEY_F1: return InputKey::F1;
	case XKB_KEY_F2: return InputKey::F2;
	case XKB_KEY_F3: return InputKey::F3;
	case XKB_KEY_F4: return InputKey::F4;
	case XKB_KEY_F5: return InputKey::F5;
	case XKB_KEY_F6: return InputKey::F6;
	case XKB_KEY_F7: return InputKey::F7;
	case XKB_KEY_F8: return InputKey::F8;
	case XKB_KEY_F9: return InputKey::F9;
	case XKB_KEY_F10: return InputKey::F10;
	case XKB_KEY_F11: return InputKey::F11;
	case XKB_KEY_F12: return InputKey::F12;
	case XKB_KEY_Shift_L: return InputKey::LShift;
	case XKB_KEY_Shift_R: return InputKey::RShift;
	case XKB_KEY_Control_L: return InputKey::LControl;
	case XKB_KEY_Control_R: return InputKey::RControl;
	case XKB_KEY_Alt_L: return InputKey::Alt;
	case XKB_KEY_Alt_R: return InputKey::Alt;
	default: return InputKey::None;
	}
}

InputKey WaylandDisplayBackend::LinuxInputEventCodeToInputKey(uint32_t inputCode)
{
	return InputKey::None;
}

uint32_t WaylandDisplayBackend::GetWaylandCursorShape(StandardCursor cursor)
{
	return 0;
}

std::string WaylandDisplayBackend::GetWaylandCursorName(StandardCursor cursor)
{
	return "left_ptr";
}

