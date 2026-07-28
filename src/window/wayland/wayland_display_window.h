#pragma once

#include "wayland_display_backend.h"
#include "window/ztimer/ztimer.h"

#include <stdexcept>
#include <array>
#include <memory>
#include <sstream>

#include <algorithm>
#include <random>
#include <map>
#include <vector>

#include "zwidget/window/window.h"
#include "zwidget/window/waylandnativehandle.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

struct wl_surface;
struct wl_buffer;
struct wl_callback;
struct xdg_surface;
struct xdg_toplevel;
struct xdg_popup;
struct wp_fractional_scale_v1;
struct zxdg_toplevel_decoration_v1;
struct zxdg_exported_v2;
struct zwp_locked_pointer_v1;
struct zwp_confined_pointer_v1;
struct xdg_toplevel_icon_v1;
struct wl_data_source;

template <typename R, typename T, typename... Args>
std::function<R(Args...)> bind_mem_fn(R(T::* func)(Args...), T *t)
{
  return [func, t] (Args... args)
	{
	  return (t->*func)(args...);
	};
}

class SharedMemHelper
{
public:
	SharedMemHelper(size_t size)
		: len(size)
	{
		std::stringstream ss;
		std::random_device device;
		std::default_random_engine engine(device());
		std::uniform_int_distribution<unsigned int> distribution(0, std::numeric_limits<unsigned int>::max());
		ss << distribution(engine);
		name = ss.str();

		fd = memfd_create(name.c_str(), 0);
		if(fd < 0)
			throw std::runtime_error("shm_open failed.");

		if(ftruncate(fd, size) < 0)
			throw std::runtime_error("ftruncate failed.");

		mem = mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if(mem == MAP_FAILED) // NOLINT
			throw std::runtime_error("mmap failed with len " + std::to_string(len) + ".");
	}

	~SharedMemHelper() noexcept
	{
		if(fd)
		{
			munmap(mem, len);
			close(fd);
			shm_unlink(name.c_str());
		}
	}

	int get_fd() const
	{
		return fd;
	}

	void *get_mem()
	{
		return mem;
	}

private:
	std::string name;
	int fd = 0;
	size_t len = 0;
	void *mem = nullptr;
};

class WaylandDisplayWindow : public DisplayWindow
{
public:
	WaylandDisplayWindow(WaylandDisplayBackend* backend, DisplayWindowHost* windowHost, WidgetType type, WaylandDisplayWindow* owner, RenderAPI renderAPI);
	~WaylandDisplayWindow();

	void SetWindowTitle(const std::string& text) override;
	void SetWindowIcon(const std::vector<std::shared_ptr<Image>>& images) override;
	void SetClientFrame(const Rect& box) override;
	void Show() override;
	void ShowFullscreen() override;
	void ShowMaximized() override;
	void ShowMinimized() override;
	void ShowNormal() override;
	void Hide() override;
	void Activate() override;
	void ShowCursor(bool enable) override;
	void LockKeyboard() override;
	void UnlockKeyboard() override;
	void LockCursor() override;
	void UnlockCursor() override;
	void CaptureMouse() override;
	void ReleaseMouseCapture() override;
	void Update() override;
	bool GetKeyState(InputKey key) override;
	void SetCursor(StandardCursor cursor, std::shared_ptr<CustomCursor> custom) override;

	Rect GetClientFrame() const override;
	Size GetClientSize() const override;
	int GetPixelWidth() const override;
	int GetPixelHeight() const override;
	double GetDpiScale() const override;

	void PresentBitmap(int width, int height, const uint32_t* pixels) override;

	void SetBorderColor(uint32_t bgra8) override;
	void SetCaptionColor(uint32_t bgra8) override;
	void SetCaptionTextColor(uint32_t bgra8) override;

	std::string GetClipboardText() override;
	void SetClipboardText(const std::string& text) override;

	Point MapFromGlobal(const Point& pos) const override;
	Point MapToGlobal(const Point& pos) const override;

	void* GetNativeHandle() override { return (void*)&m_NativeHandle; }

	std::vector<std::string> GetVulkanInstanceExtensions() override;
	VkSurfaceKHR CreateVulkanSurface(VkInstance instance) override;

	void* GetEGLNativeDisplay() override;
	void* GetEGLNativeWindow() override;

	struct wl_surface* GetWindowSurface() { return m_AppSurface; }

	bool IsWindowFullscreen() override;

	// Event handlers as otherwise linking DisplayWindowHost On...() functions with Wayland events directly crashes the app
	// Alternatively to avoid crashes one can capture by value ([=]) instead of reference ([&])
	void OnXDGToplevelConfigureEvent(int32_t width, int32_t height);
	void OnXDGToplevelStateEvent(struct wl_array* states);
	void OnExportHandleEvent(const char* handle);
	void OnExitEvent();

	void DrawSurface(uint32_t serial = 0);

	void InitializeToplevel();
	void InitializePopup();

	WaylandDisplayBackend* backend = nullptr;
	WaylandDisplayWindow* m_owner = nullptr;
	DisplayWindowHost* windowHost = nullptr;
	WidgetType m_WidgetType = WidgetType::Window;
	bool m_PopupWindow = false;
	// xdg-dialog-v1 marks a toplevel as a dialog so the compositor can treat
	// it accordingly. Only created for WidgetType::Dialog, and only when the
	// compositor advertises the protocol.
	struct xdg_dialog_v1* m_XDGDialog = nullptr;

	// Set by LockKeyboard(). While locked the window also reports physical key
	// positions through OnWindowRawKey, in addition to the translated events.
	bool m_KeyboardLocked = false;
	bool m_IsActivated = false;

	bool m_NeedsUpdate = true;

	Point m_WindowGlobalPos = Point(0, 0);
    Size m_WindowSize = Size(0, 0);
	Size m_LogicalSize = Size(640, 480); // Logical pixel size as sent by the compositor
	double m_ScaleFactor = 1.0;

	Point m_SurfaceMousePos = Point(0, 0);

	WaylandNativeHandle m_NativeHandle;
	RenderAPI m_renderAPI;

	struct wl_data_source* m_DataSource = nullptr;
	struct zxdg_toplevel_decoration_v1* m_XDGToplevelDecoration = nullptr;
	struct wp_fractional_scale_v1* m_FractionalScale = nullptr;
	struct wl_surface* m_AppSurface = nullptr;
	struct wl_buffer* m_AppSurfaceBuffer = nullptr;
	struct xdg_surface* m_XDGSurface = nullptr;
	struct xdg_toplevel* m_XDGToplevel = nullptr;
	struct xdg_popup* m_XDGPopup = nullptr;
	struct zxdg_exported_v2* m_XDGExported = nullptr;
	struct zwp_locked_pointer_v1* m_LockedPointer = nullptr;
	struct zwp_confined_pointer_v1* m_ConfinedPointer = nullptr;
	struct xdg_toplevel_icon_v1* m_XDGToplevelIcon = nullptr;
	struct wl_callback* m_FrameCallback = nullptr;

	std::string m_windowID;

	std::shared_ptr<SharedMemHelper> shared_mem;

	std::vector<std::shared_ptr<SharedMemHelper>> appIconSharedMems;
	std::vector<struct wl_buffer*> appIconBuffers;

	bool isFullscreen = false;

private:
	// Helper functions
	void CreateBuffers(int32_t width, int32_t height);

	void CreateAppIconBuffers(const std::vector<std::shared_ptr<Image>>& images);
	std::string GetWaylandWindowID();

	friend WaylandDisplayBackend;
};
