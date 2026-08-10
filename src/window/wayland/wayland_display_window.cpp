#include "wayland_display_window.h"
#include "xdg-dialog-v1-client-protocol.h"
#include "wayland_display_backend.h"
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
#include <cstring>
#include <dlfcn.h>
#include "core/image.h"

#define WAYLAND WaylandDynamic::Get()

// Listeners
static void xdg_surface_handle_configure(void* data, struct xdg_surface* xdg_surface, uint32_t serial)
{
	WaylandDisplayWindow* window = (WaylandDisplayWindow*)data;
	xdg_surface_ack_configure(xdg_surface, serial);

	// xdg-shell requires a buffer to be attached and committed after this ack;
	// until that happens the compositor has nothing to show. Ask for a paint
	// unconditionally here, because the toplevel configure cannot be relied on
	// to do it: it only requests one when it is given a non-zero size, and the
	// FIRST configure a compositor sends is normally 0x0 -- that is how it tells
	// the client to pick its own size. KWin does exactly this, so the launcher
	// came up blank and stayed blank until some unrelated event (moving the
	// pointer over it) happened to set m_NeedsUpdate again.
	//
	// m_NeedsUpdate starts true, but the run loop consumes it on its first pass,
	// which can come before this handshake completes -- DrawSurface() then
	// returns early because no buffer exists yet, and the flag has already been
	// cleared. Setting it here means every configure, initial or later, is
	// followed by a paint.
	window->m_NeedsUpdate = true;
}
static const struct xdg_surface_listener xdg_surface_listener = { xdg_surface_handle_configure };

static void xdg_toplevel_handle_configure(void* data, struct xdg_toplevel* xdg_toplevel, int32_t width, int32_t height, struct wl_array* states)
{
	WaylandDisplayWindow* window = (WaylandDisplayWindow*)data;
	window->OnXDGToplevelConfigureEvent(width, height);
	window->OnXDGToplevelStateEvent(states);
}
static void xdg_toplevel_handle_close(void* data, struct xdg_toplevel* xdg_toplevel)
{
	WaylandDisplayWindow* window = (WaylandDisplayWindow*)data;
	window->OnExitEvent();
}
static void xdg_toplevel_handle_configure_bounds(void* data, struct xdg_toplevel* xdg_toplevel, int32_t width, int32_t height) {}
static const struct xdg_toplevel_listener xdg_toplevel_listener = { xdg_toplevel_handle_configure, xdg_toplevel_handle_close, xdg_toplevel_handle_configure_bounds };

static void xdg_popup_handle_configure(void* data, struct xdg_popup* xdg_popup, int32_t x, int32_t y, int32_t width, int32_t height)
{
	WaylandDisplayWindow* window = (WaylandDisplayWindow*)data;
	window->SetClientFrame(Rect::xywh(x, y, width, height));
}
static void xdg_popup_handle_popup_done(void* data, struct xdg_popup* xdg_popup)
{
	WaylandDisplayWindow* window = (WaylandDisplayWindow*)data;
	window->OnExitEvent();
}
static void xdg_popup_handle_repositioned(void* data, struct xdg_popup* xdg_popup, uint32_t token) {}
static const struct xdg_popup_listener xdg_popup_listener = { xdg_popup_handle_configure, xdg_popup_handle_popup_done, xdg_popup_handle_repositioned };

static void fractional_scale_handle_preferred_scale(void* data, struct wp_fractional_scale_v1* fractional_scale, uint32_t scale)
{
	WaylandDisplayWindow* window = (WaylandDisplayWindow*)data;
	window->m_ScaleFactor = (scale > 0) ? (scale / 120.0) : 1.0;
	if (window->m_ScaleFactor < 0.001) window->m_ScaleFactor = 1.0;
	window->m_NeedsUpdate = true;
	window->windowHost->OnWindowDpiScaleChanged();
}
static const struct wp_fractional_scale_v1_listener fractional_scale_listener = { fractional_scale_handle_preferred_scale };

static void frame_handle_done(void* data, struct wl_callback* callback, uint32_t time)
{
	WaylandDisplayWindow* window = (WaylandDisplayWindow*)data;
	// Null out the tracked callback BEFORE destroying it so DrawSurface
	// doesn't try to destroy it a second time.
	window->m_FrameCallback = nullptr;
	if (callback) wl_callback_destroy(callback);
	window->m_NeedsUpdate = true;
}
static const struct wl_callback_listener frame_listener = { frame_handle_done };

static void buffer_handle_release(void* data, struct wl_buffer* buffer)
{
	// The compositor is done with this buffer — safe to destroy now.
	wl_buffer_destroy(buffer);
}
static const struct wl_buffer_listener buffer_release_listener = { buffer_handle_release };

static void xdg_exported_handle_handle(void* data, struct zxdg_exported_v2* zxdg_exported_v2, const char* handle)
{
	WaylandDisplayWindow* window = (WaylandDisplayWindow*)data;
	window->OnExportHandleEvent(handle);
}
static const struct zxdg_exported_v2_listener xdg_exported_listener = { xdg_exported_handle_handle };

static void locked_pointer_handle_locked(void* data, struct zwp_locked_pointer_v1* locked_pointer)
{
	WaylandDisplayWindow* window = (WaylandDisplayWindow*)data;
	window->backend->SetMouseLocked(true);
	window->ShowCursor(false);
}
static void locked_pointer_handle_unlocked(void* data, struct zwp_locked_pointer_v1* locked_pointer)
{
	WaylandDisplayWindow* window = (WaylandDisplayWindow*)data;
	window->backend->SetMouseLocked(false);
	window->ShowCursor(true);
}
static const struct zwp_locked_pointer_v1_listener locked_pointer_listener = { locked_pointer_handle_locked, locked_pointer_handle_unlocked };

WaylandDisplayWindow::WaylandDisplayWindow(WaylandDisplayBackend* backend, DisplayWindowHost* windowHost, WidgetType type, WaylandDisplayWindow* owner, RenderAPI renderAPI)
	: backend(backend), m_owner(owner), windowHost(windowHost), m_WidgetType(type), m_PopupWindow(type == WidgetType::Popup), m_renderAPI(renderAPI), m_WindowSize(0, 0), m_LogicalSize(0, 0)
{
	m_AppSurface = wl_compositor_create_surface(backend->m_waylandCompositor);

	m_NativeHandle.display = backend->s_waylandDisplay;
	m_NativeHandle.surface = m_AppSurface;

	if (backend->m_FractionalScaleManager)
	{
		m_FractionalScale = wp_fractional_scale_manager_v1_get_fractional_scale(backend->m_FractionalScaleManager, m_AppSurface);
		wp_fractional_scale_v1_add_listener(m_FractionalScale, &fractional_scale_listener, this);
	}

	m_XDGSurface = xdg_wm_base_get_xdg_surface(backend->m_XDGWMBase, m_AppSurface);
	xdg_surface_add_listener(m_XDGSurface, &xdg_surface_listener, this);

	if (m_PopupWindow) InitializePopup();
	else InitializeToplevel();

	backend->OnWindowCreated(this);
	this->DrawSurface();
}

WaylandDisplayWindow::~WaylandDisplayWindow()
{
	backend->OnWindowDestroyed(this);
	if (m_FrameCallback) wl_callback_destroy(m_FrameCallback);
	if (m_XDGToplevelIcon) xdg_toplevel_icon_v1_destroy(m_XDGToplevelIcon);
	if (m_XDGToplevelDecoration) zxdg_toplevel_decoration_v1_destroy(m_XDGToplevelDecoration);
	if (m_XDGDialog) xdg_dialog_v1_destroy(m_XDGDialog);
	if (m_XDGToplevel) xdg_toplevel_destroy(m_XDGToplevel);
	if (m_XDGPopup) xdg_popup_destroy(m_XDGPopup);
	if (m_XDGExported) zxdg_exported_v2_destroy(m_XDGExported);
	if (m_XDGSurface) xdg_surface_destroy(m_XDGSurface);
	if (m_FractionalScale) wp_fractional_scale_v1_destroy(m_FractionalScale);
	if (m_AppSurfaceBuffer) wl_buffer_destroy(m_AppSurfaceBuffer);
	if (m_AppSurface) wl_surface_destroy(m_AppSurface);
}

void WaylandDisplayWindow::InitializeToplevel()
{
	m_XDGToplevel = xdg_surface_get_toplevel(m_XDGSurface);
	xdg_toplevel_add_listener(m_XDGToplevel, &xdg_toplevel_listener, this);
	xdg_toplevel_set_title(m_XDGToplevel, "ZWidget Window");

	if (m_WidgetType == WidgetType::Dialog && backend->m_XDGWMDialog)
		m_XDGDialog = xdg_wm_dialog_v1_get_xdg_dialog(backend->m_XDGWMDialog, m_XDGToplevel);

	if (m_owner) xdg_toplevel_set_parent(m_XDGToplevel, m_owner->m_XDGToplevel);

	if (backend->m_XDGDecorationManager)
	{
		m_XDGToplevelDecoration = zxdg_decoration_manager_v1_get_toplevel_decoration(backend->m_XDGDecorationManager, m_XDGToplevel);
		zxdg_toplevel_decoration_v1_set_mode(m_XDGToplevelDecoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
	}

	wl_surface_commit(m_AppSurface);
	wl_display_roundtrip(backend->s_waylandDisplay);

	if (backend->m_XDGExporter)
	{
		m_XDGExported = zxdg_exporter_v2_export_toplevel(backend->m_XDGExporter, m_AppSurface);
		zxdg_exported_v2_add_listener(m_XDGExported, &xdg_exported_listener, this);
	}
}

void WaylandDisplayWindow::InitializePopup()
{
	if (!m_owner) throw std::runtime_error("Popup window must have an owner!");

	struct xdg_positioner* popupPositioner = xdg_wm_base_create_positioner(backend->m_XDGWMBase);
	xdg_positioner_set_anchor(popupPositioner, XDG_POSITIONER_ANCHOR_BOTTOM);
	xdg_positioner_set_anchor_rect(popupPositioner, 0, 0, 1, 30);
	xdg_positioner_set_size(popupPositioner, 1, 1);

	m_XDGPopup = xdg_surface_get_popup(m_XDGSurface, m_owner->m_XDGSurface, popupPositioner);
	xdg_popup_add_listener(m_XDGPopup, &xdg_popup_listener, this);
	xdg_positioner_destroy(popupPositioner);

	wl_surface_commit(m_AppSurface);
	wl_display_roundtrip(backend->s_waylandDisplay);
}

void WaylandDisplayWindow::SetWindowTitle(const std::string& text)
{
	if (m_XDGToplevel) xdg_toplevel_set_title(m_XDGToplevel, text.c_str());
}

void WaylandDisplayWindow::SetWindowIcon(const std::vector<std::shared_ptr<Image>>& images)
{
	if (backend->m_XDGToplevelIconManager && m_XDGToplevel)
	{
		if (images.empty())
		{
			xdg_toplevel_icon_manager_v1_set_icon(backend->m_XDGToplevelIconManager, m_XDGToplevel, nullptr);
			return;
		}

		if (m_XDGToplevelIcon) xdg_toplevel_icon_v1_destroy(m_XDGToplevelIcon);
		m_XDGToplevelIcon = xdg_toplevel_icon_manager_v1_create_icon(backend->m_XDGToplevelIconManager);

		CreateAppIconBuffers(images);
		for (auto& icon_buffer : appIconBuffers)
		{
			xdg_toplevel_icon_v1_add_buffer(m_XDGToplevelIcon, icon_buffer, 1);
		}
		xdg_toplevel_icon_manager_v1_set_icon(backend->m_XDGToplevelIconManager, m_XDGToplevel, m_XDGToplevelIcon);
	}
}


void WaylandDisplayWindow::SetClientFrame(const Rect& box)
{
	// Wayland has no way for a client to position itself; only the size
	// is actionable, and the compositor owns placement.
	m_WindowSize = Size(box.width, box.height);
	if (m_AppSurface) CreateBuffers(box.width, box.height);
}

void WaylandDisplayWindow::Show()
{
	if (m_renderAPI == RenderAPI::OpenGL || m_renderAPI == RenderAPI::Vulkan)
	{
		// EGL mode: the compositor receives content via eglSwapBuffers.
		// Just commit to satisfy the xdg_surface configure handshake;
		// do NOT attach the (null) SHM buffer.
		wl_surface_commit(m_AppSurface);
		return;
	}

	struct wl_region* region = wl_compositor_create_region(backend->m_waylandCompositor);
	wl_region_add(region, 0, 0, (int32_t)m_WindowSize.width, (int32_t)m_WindowSize.height);
	wl_surface_set_opaque_region(m_AppSurface, region);
	wl_region_destroy(region);

	wl_surface_attach(m_AppSurface, m_AppSurfaceBuffer, 0, 0);
	wl_surface_damage(m_AppSurface, 0, 0, m_WindowSize.width, m_WindowSize.height);
	wl_surface_commit(m_AppSurface);
}

void WaylandDisplayWindow::ShowFullscreen()
{
	if (m_XDGToplevel)
	{
		xdg_toplevel_set_fullscreen(m_XDGToplevel, backend->m_waylandOutput);
		isFullscreen = true;
	}
}

void WaylandDisplayWindow::ShowMaximized() { if (m_XDGToplevel) xdg_toplevel_set_maximized(m_XDGToplevel); }
void WaylandDisplayWindow::ShowMinimized() { if (m_XDGToplevel) xdg_toplevel_set_minimized(m_XDGToplevel); }
void WaylandDisplayWindow::ShowNormal() { if (m_XDGToplevel) xdg_toplevel_unset_fullscreen(m_XDGToplevel); }
bool WaylandDisplayWindow::IsWindowFullscreen() { return isFullscreen; }

void WaylandDisplayWindow::Hide()
{
	wl_surface_attach(m_AppSurface, nullptr, 0, 0);
	wl_surface_commit(m_AppSurface);
}

void WaylandDisplayWindow::Activate()
{
	if (backend->m_XDGActivation)
	{
		struct xdg_activation_token_v1* token = xdg_activation_v1_get_activation_token(backend->m_XDGActivation);
		// To do: add listener to get token string
		xdg_activation_token_v1_set_surface(token, m_AppSurface);
		xdg_activation_token_v1_commit(token);
		// For now just activate without token if possible or wait for roundtrip
	}
	backend->m_FocusWindow = this;
	backend->m_MouseFocusWindow = this;
	windowHost->OnWindowActivated();
}

void WaylandDisplayWindow::ShowCursor(bool enable) { backend->ShowCursor(enable); }

void WaylandDisplayWindow::LockKeyboard()
{
	m_KeyboardLocked = true;
}

void WaylandDisplayWindow::UnlockKeyboard()
{
	m_KeyboardLocked = false;
}

void WaylandDisplayWindow::LockCursor()
{
	if (backend->m_PointerConstraints)
	{
		m_LockedPointer = zwp_pointer_constraints_v1_lock_pointer(backend->m_PointerConstraints, m_AppSurface, backend->m_waylandPointer, nullptr, ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT);
		zwp_locked_pointer_v1_add_listener(m_LockedPointer, &locked_pointer_listener, this);
	}
}

void WaylandDisplayWindow::UnlockCursor()
{
	if (m_LockedPointer) { zwp_locked_pointer_v1_destroy(m_LockedPointer); m_LockedPointer = nullptr; }
	backend->SetMouseLocked(false);
	ShowCursor(true);
}

void WaylandDisplayWindow::CaptureMouse()
{
	if (backend->m_PointerConstraints)
	{
		m_ConfinedPointer = zwp_pointer_constraints_v1_confine_pointer(backend->m_PointerConstraints, m_AppSurface, backend->m_waylandPointer, nullptr, ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT);
		ShowCursor(false);
	}
}

void WaylandDisplayWindow::ReleaseMouseCapture()
{
	if (m_ConfinedPointer) { zwp_confined_pointer_v1_destroy(m_ConfinedPointer); m_ConfinedPointer = nullptr; }
	ShowCursor(true);
}

void WaylandDisplayWindow::Update() { m_NeedsUpdate = true; }
bool WaylandDisplayWindow::GetKeyState(InputKey key) { return backend->GetKeyState(key); }
void WaylandDisplayWindow::SetCursor(StandardCursor cursor, std::shared_ptr<CustomCursor> custom) { backend->SetCursor(cursor); }
Rect WaylandDisplayWindow::GetClientFrame() const { return Rect(m_WindowGlobalPos.x, m_WindowGlobalPos.y, m_LogicalSize.width, m_LogicalSize.height); }
Size WaylandDisplayWindow::GetClientSize() const { return m_LogicalSize; }
int WaylandDisplayWindow::GetPixelWidth() const { return m_WindowSize.width; }
int WaylandDisplayWindow::GetPixelHeight() const { return m_WindowSize.height; }
double WaylandDisplayWindow::GetDpiScale() const { return m_ScaleFactor; }

void WaylandDisplayWindow::PresentBitmap(int width, int height, const uint32_t* pixels)
{

	// width/height here are physical (pixel) dimensions from the renderer.
	if (!m_AppSurfaceBuffer || width != m_WindowSize.width || height != m_WindowSize.height)
	{
		CreateBuffers((int32_t)(width / m_ScaleFactor + 0.5), (int32_t)(height / m_ScaleFactor + 0.5));
	}
	
	if (m_AppSurfaceBuffer)
	{
		std::memcpy(shared_mem->get_mem(), (void*)pixels, width * height * 4);
		wl_surface_attach(m_AppSurface, m_AppSurfaceBuffer, 0, 0);
		wl_surface_damage_buffer(m_AppSurface, 0, 0, width, height);
		wl_surface_commit(m_AppSurface);
	}
	else
	{
	}
}

void WaylandDisplayWindow::SetBorderColor(uint32_t bgra8) {}
void WaylandDisplayWindow::SetCaptionColor(uint32_t bgra8) {}
void WaylandDisplayWindow::SetCaptionTextColor(uint32_t bgra8) {}

std::string WaylandDisplayWindow::GetClipboardText() { return backend->GetClipboardText(); }

void WaylandDisplayWindow::SetClipboardText(const std::string& text)
{
	// To do: implement using C API
}

Point WaylandDisplayWindow::MapFromGlobal(const Point& pos) const { return (pos - m_WindowGlobalPos) / m_ScaleFactor; }
Point WaylandDisplayWindow::MapToGlobal(const Point& pos) const { return (m_WindowGlobalPos + pos) / m_ScaleFactor; }

void WaylandDisplayWindow::OnXDGToplevelConfigureEvent(int32_t width, int32_t height)
{
	// The compositor sends logical (unscaled) dimensions in the configure event.
	// Store them as logical so CreateBuffers can correctly scale to physical pixels.
	if (width > 0 && height > 0)
	{
		m_LogicalSize = Size(width, height);
		windowHost->OnWindowGeometryChanged();
		m_NeedsUpdate = true;
	}
}

void WaylandDisplayWindow::OnXDGToplevelStateEvent(struct wl_array* states)
{
	uint32_t* state;
	bool activated = false;
	for (state = (uint32_t*)(states)->data; (const char*)state < ((const char*)(states)->data + (states)->size); state++) { if (*state == XDG_TOPLEVEL_STATE_ACTIVATED) activated = true; }
	if (activated == m_IsActivated) return;
	m_IsActivated = activated;
	if (activated)
	{
		backend->m_FocusWindow = this;
		backend->m_MouseFocusWindow = this;
		windowHost->OnWindowActivated();
	}
	else windowHost->OnWindowDeactivated();
}

void WaylandDisplayWindow::OnExportHandleEvent(const char* handle) { m_windowID = handle; }
void WaylandDisplayWindow::OnExitEvent() { windowHost->OnWindowClose(); }

void WaylandDisplayWindow::DrawSurface(uint32_t serial)
{
	// For EGL/OpenGL the compositor receives content via eglSwapBuffers — do not
	// mix SHM buffer commits on the same wl_surface or the window stays blank.
	if (m_renderAPI == RenderAPI::OpenGL || m_renderAPI == RenderAPI::Vulkan)
		return;

	if (m_renderAPI == RenderAPI::Unspecified || m_renderAPI == RenderAPI::Bitmap)
	{
		if (!m_AppSurfaceBuffer) return;
		// Do not schedule a new frame callback if one is already pending.
		if (!m_FrameCallback)
		{
			m_FrameCallback = wl_surface_frame(m_AppSurface);
			wl_callback_add_listener(m_FrameCallback, &frame_listener, this);
		}

		wl_surface_attach(m_AppSurface, m_AppSurfaceBuffer, 0, 0);
		wl_surface_damage_buffer(m_AppSurface, 0, 0, m_WindowSize.width, m_WindowSize.height);
		wl_surface_commit(m_AppSurface);
	}
}

void WaylandDisplayWindow::CreateBuffers(int32_t width, int32_t height)
{
	if (width <= 0 || height <= 0) return;

	// For EGL/OpenGL mode the surface content comes from eglSwapBuffers — we
	// must NOT create or attach SHM buffers on the same wl_surface.
	if (m_renderAPI == RenderAPI::OpenGL || m_renderAPI == RenderAPI::Vulkan)
	{
		// Still track logical/physical size so GetPixelWidth/Height are correct.
		int phys_width  = (int)(width  * m_ScaleFactor + 0.5);
		int phys_height = (int)(height * m_ScaleFactor + 0.5);
		if (phys_width <= 0 || phys_height <= 0) return;
		m_WindowSize = Size(phys_width, phys_height);
		m_LogicalSize = Size(width, height);
		return;
	}

	// width/height are logical pixels — scale up to physical buffer dimensions.
	int phys_width  = (int)(width  * m_ScaleFactor + 0.5);
	int phys_height = (int)(height * m_ScaleFactor + 0.5);
	if (phys_width <= 0 || phys_height <= 0) return;
	// Skip if size hasn't actually changed.
	if (m_AppSurfaceBuffer && m_WindowSize.width == phys_width && m_WindowSize.height == phys_height) return;

	shared_mem = std::make_shared<SharedMemHelper>(phys_width * phys_height * 4);
	struct wl_shm_pool* pool = wl_shm_create_pool(backend->m_waylandSHM, shared_mem->get_fd(), phys_width * phys_height * 4);
	// Don't destroy the old buffer directly — attach a release listener so the
	// compositor tells us when it's safe. The listener callback destroys it.
	if (m_AppSurfaceBuffer)
		wl_buffer_add_listener(m_AppSurfaceBuffer, &buffer_release_listener, nullptr);
	m_AppSurfaceBuffer = wl_shm_pool_create_buffer(pool, 0, phys_width, phys_height, phys_width * 4, WL_SHM_FORMAT_XRGB8888);
	wl_shm_pool_destroy(pool);
	m_WindowSize = Size(phys_width, phys_height);
	// Also record the logical size for geometry queries.
	m_LogicalSize = Size(width, height);
}

void WaylandDisplayWindow::CreateAppIconBuffers(const std::vector<std::shared_ptr<Image>>& images)
{
	appIconBuffers.clear();
	appIconSharedMems.clear();
	for (auto& image: images)
	{
		int size = image->GetWidth() * image->GetHeight() * 4;
		auto shm = std::make_shared<SharedMemHelper>(size);
		struct wl_shm_pool* pool = wl_shm_create_pool(backend->m_waylandSHM, shm->get_fd(), size);
		struct wl_buffer* buffer = wl_shm_pool_create_buffer(pool, 0, image->GetWidth(), image->GetHeight(), image->GetWidth() * 4, WL_SHM_FORMAT_ARGB8888);
		wl_shm_pool_destroy(pool);
		std::memcpy(shm->get_mem(), image->GetData(), size);
		appIconSharedMems.push_back(shm);
		appIconBuffers.push_back(buffer);
	}
}

std::string WaylandDisplayWindow::GetWaylandWindowID() { return m_windowID; }

// Vulkan integration
#ifndef VK_VERSION_1_0
#define VKAPI_CALL
#define VKAPI_PTR VKAPI_CALL
typedef uint32_t VkFlags;
typedef enum VkStructureType { VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR = 1000006000 } VkStructureType;
typedef enum VkResult { VK_SUCCESS = 0 } VkResult;
typedef struct VkAllocationCallbacks VkAllocationCallbacks;
typedef void (VKAPI_PTR* PFN_vkVoidFunction)(void);
typedef PFN_vkVoidFunction(VKAPI_PTR* PFN_vkGetInstanceProcAddr)(VkInstance instance, const char* pName);
typedef struct VkWaylandSurfaceCreateInfoKHR { VkStructureType sType; const void* pNext; VkFlags flags; struct wl_display* display; struct wl_surface* surface; } VkWaylandSurfaceCreateInfoKHR;
typedef VkResult(VKAPI_PTR* PFN_vkCreateWaylandSurfaceKHR)(VkInstance instance, const VkWaylandSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);
#endif

class ZWidgetWaylandVulkanLoader
{
public:
	ZWidgetWaylandVulkanLoader()
	{
#ifdef __APPLE__
		module = dlopen("libvulkan.dylib", RTLD_NOW | RTLD_LOCAL);
#else
		module = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
#endif
		if (module) vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(module, "vkGetInstanceProcAddr");
	}
	~ZWidgetWaylandVulkanLoader() { if (module) dlclose(module); }
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
	void* module = nullptr;
};

VkSurfaceKHR WaylandDisplayWindow::CreateVulkanSurface(VkInstance instance)
{
	static ZWidgetWaylandVulkanLoader loader;
	if (!loader.vkGetInstanceProcAddr) throw std::runtime_error("Vulkan not found");
	auto vkCreateWaylandSurfaceKHR = (PFN_vkCreateWaylandSurfaceKHR)loader.vkGetInstanceProcAddr(instance, "vkCreateWaylandSurfaceKHR");
	if (!vkCreateWaylandSurfaceKHR) throw std::runtime_error("vkCreateWaylandSurfaceKHR not found");
	VkWaylandSurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR };
	createInfo.display = m_NativeHandle.display;
	createInfo.surface = m_NativeHandle.surface;
	VkSurfaceKHR surface = {};
	if (vkCreateWaylandSurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS) throw std::runtime_error("Failed to create Vulkan surface");
	return surface;
}

void* WaylandDisplayWindow::GetEGLNativeDisplay()
{
	return (void*)m_NativeHandle.display;
}

void* WaylandDisplayWindow::GetEGLNativeWindow()
{
	return (void*)m_NativeHandle.surface;
}

std::vector<std::string> WaylandDisplayWindow::GetVulkanInstanceExtensions() { return { "VK_KHR_surface", "VK_KHR_wayland_surface" }; }
