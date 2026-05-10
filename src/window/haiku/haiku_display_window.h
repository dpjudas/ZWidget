#pragma once

#include <zwidget/window/window.h>
#include <zwidget/window/haikunativehandle.h>
#include <vector>
#include <string>
#include <memory>

#include "haiku_window.h"
#include <Bitmap.h>

class HaikuDisplayWindow : public DisplayWindow
{
public:
	HaikuDisplayWindow(DisplayWindowHost* windowHost, WidgetType type, HaikuDisplayWindow* owner, RenderAPI renderAPI);
	~HaikuDisplayWindow();

	void SetWindowTitle(const std::string& text) override;
	void SetWindowIcon(const std::vector<std::shared_ptr<Image>>& images) override;
	void SetClientFrame(const Rect& box) override;
	void Show() override;
	void ShowFullscreen() override;
	void ShowMaximized() override;
	void ShowMinimized() override;
	void ShowNormal() override;
	bool IsWindowFullscreen() override;
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

	void* GetNativeHandle() override { return &handle; }

	std::vector<std::string> GetVulkanInstanceExtensions() override;
	VkSurfaceKHR CreateVulkanSurface(VkInstance instance) override;

	DisplayWindowHost* windowHost = nullptr;
	HaikuNativeHandle handle;
	ZWindow* window = nullptr;
	BBitmap* bitmap = nullptr;
	bool in_draw = false;
    bool cursor_locked = false;
};
