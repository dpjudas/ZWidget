#include <iostream>
#include <zwidget/core/widget.h>
#include <zwidget/core/resourcedata.h>
#include <zwidget/core/image.h>
#include <zwidget/core/theme.h>
#include <zwidget/window/window.h>
#include <zwidget/widgets/dropdown/dropdown.h>
#include <zwidget/widgets/textedit/textedit.h>
#include <zwidget/widgets/mainwindow/mainwindow.h>
#include <zwidget/widgets/layout/vboxlayout.h>
#include <zwidget/widgets/layout/hboxlayout.h>
#include <zwidget/widgets/listview/listview.h>
#include <zwidget/widgets/imagebox/imagebox.h>
#include <zwidget/widgets/textlabel/textlabel.h>
#include <zwidget/widgets/pushbutton/pushbutton.h>
#include <zwidget/widgets/checkboxlabel/checkboxlabel.h>
#include <zwidget/widgets/lineedit/lineedit.h>
#include "picopng.h"
#include <zwidget/widgets/tabwidget/tabwidget.h>

// ************************************************************
// Prototypes
// ************************************************************

class LauncherWindowTab1 : public Widget
{
public:
	LauncherWindowTab1(Widget parent);
	void OnGeometryChanged() override;
private:
	TextEdit* Text = nullptr;
};

class LauncherWindowTab2 : public Widget
{
public:
	LauncherWindowTab2(Widget parent);
	void OnGeometryChanged() override;
private:
	TextLabel* WelcomeLabel = nullptr;
	TextLabel* VersionLabel = nullptr;
	TextLabel* SelectLabel = nullptr;
	TextLabel* GeneralLabel = nullptr;
	TextLabel* ExtrasLabel = nullptr;
	CheckboxLabel* FullscreenCheckbox = nullptr;
	CheckboxLabel* DisableAutoloadCheckbox = nullptr;
	CheckboxLabel* DontAskAgainCheckbox = nullptr;
	CheckboxLabel* LightsCheckbox = nullptr;
	CheckboxLabel* BrightmapsCheckbox = nullptr;
	CheckboxLabel* WidescreenCheckbox = nullptr;
	ListView* GamesList = nullptr;
};

class LauncherWindowTab3 : public Widget
{
public:
	LauncherWindowTab3(Widget parent);
	void OnGeometryChanged() override;
private:
	TextLabel* Label = nullptr;
	Dropdown* Choices = nullptr;
	PushButton* Popup = nullptr;
};

class LauncherWindow : public Widget
{
public:
	LauncherWindow();
private:
	void OnClose() override;
	void OnGeometryChanged() override;

	ImageBox* Logo = nullptr;
	TabWidget* Pages = nullptr;
	PushButton* ExitButton = nullptr;

	LauncherWindowTab1* Tab1 = nullptr;
	LauncherWindowTab2* Tab2 = nullptr;
	LauncherWindowTab3* Tab3 = nullptr;

	std::shared_ptr<CustomCursor> Cursor;
};

// ************************************************************
// UI implementation
// ************************************************************

LauncherWindow::LauncherWindow(): Widget(nullptr, WidgetType::Window)
{
	SetWindowTitle("ZWidget Demo");

	try
	{
		SetWindowIcon({
			Image::LoadResource("surreal-engine-icon-16.png"),
			Image::LoadResource("surreal-engine-icon-24.png"),
			Image::LoadResource("surreal-engine-icon-32.png"),
			Image::LoadResource("surreal-engine-icon-48.png"),
			Image::LoadResource("surreal-engine-icon-64.png"),
			Image::LoadResource("surreal-engine-icon-128.png"),
			Image::LoadResource("surreal-engine-icon-256.png")
			});

		Cursor = CustomCursor::Create({
			CustomCursorFrame(Image::LoadResource("Pentagram01.png"), 0.2),
			CustomCursorFrame(Image::LoadResource("Pentagram02.png"), 0.2),
			CustomCursorFrame(Image::LoadResource("Pentagram03.png"), 0.2),
			CustomCursorFrame(Image::LoadResource("Pentagram04.png"), 0.2),
			CustomCursorFrame(Image::LoadResource("Pentagram05.png"), 0.2),
			CustomCursorFrame(Image::LoadResource("Pentagram06.png"), 0.2),
			CustomCursorFrame(Image::LoadResource("Pentagram07.png"), 0.2),
			CustomCursorFrame(Image::LoadResource("Pentagram08.png"), 0.2),
			CustomCursorFrame(Image::LoadResource("Pentagram09.png"), 0.2),
			CustomCursorFrame(Image::LoadResource("Pentagram10.png"), 0.2),
			CustomCursorFrame(Image::LoadResource("Pentagram11.png"), 0.2),
			CustomCursorFrame(Image::LoadResource("Pentagram12.png"), 0.2),
			CustomCursorFrame(Image::LoadResource("Pentagram13.png"), 0.2),
			CustomCursorFrame(Image::LoadResource("Pentagram14.png"), 0.2)
			}, Point(16, 16));
	}
	catch (...)
	{
	}

	Logo = new ImageBox(this);
	ExitButton = new PushButton(this);
	Pages = new TabWidget(this);

	Logo->SetCursor(Cursor);

	Tab1 = new LauncherWindowTab1(this);
	Tab2 = new LauncherWindowTab2(this);
	Tab3 = new LauncherWindowTab3(this);

	Pages->AddTab(Tab1, "Welcome");
	Pages->AddTab(Tab2, "VKDoom");
	Pages->AddTab(Tab3, "ZWidgets");

	ExitButton->SetText("Exit");

	ExitButton->OnClick = []{
		DisplayWindow::ExitLoop();
	};

	try
	{
		auto filedata = ReadAllBytes("banner.png");
		std::vector<unsigned char> pixels;
		unsigned long width = 0, height = 0;
		int result = decodePNG(pixels, width, height, (const unsigned char*)filedata.data(), filedata.size(), true);
		if (result == 0)
		{
			Logo->SetImage(Image::Create(width, height, ImageFormat::R8G8B8A8, pixels.data()));
			std::cout << "Banner image loaded: " << width << "x" << height << std::endl;
		}
		else
		{
			std::cout << "Failed to decode banner.png, result=" << result << std::endl;
		}
	}
	catch (const std::exception& e)
	{
		std::cout << "Exception loading banner: " << e.what() << std::endl;
	}
	catch (...)
	{
		std::cout << "Unknown exception loading banner" << std::endl;
	}
}

void LauncherWindow::OnGeometryChanged()
{
	double y = 0, h;

	h = Logo->GetPreferredHeight();
	Logo->SetFrameGeometry(0, y, GetWidth(), h);
	y += h;

	h = GetHeight() - y - ExitButton->GetPreferredHeight() - 40;
	Pages->SetFrameGeometry(0, y, GetWidth(), h);
	y += h + 20;

	ExitButton->SetFrameGeometry(GetWidth() - 20 - 120, y, 120, ExitButton->GetPreferredHeight());
}

void LauncherWindow::OnClose()
{
	DisplayWindow::ExitLoop();
}

LauncherWindowTab1::LauncherWindowTab1(Widget parent): Widget(nullptr)
{
	Text = new TextEdit(this);

	Text->SetText(
		"Welcome to VKDoom\n\n"
		"Click the tabs to look at other widgets\n\n"
		"Also, this text is editable\n"
	);
}

void LauncherWindowTab1::OnGeometryChanged()
{
	Text->SetFrameGeometry(0, 10, GetWidth(), GetHeight());
}

LauncherWindowTab2::LauncherWindowTab2(Widget parent): Widget(nullptr)
{
	WelcomeLabel = new TextLabel(this);
	VersionLabel = new TextLabel(this);
	SelectLabel = new TextLabel(this);
	GeneralLabel = new TextLabel(this);
	ExtrasLabel = new TextLabel(this);
	FullscreenCheckbox = new CheckboxLabel(this);
	DisableAutoloadCheckbox = new CheckboxLabel(this);
	DontAskAgainCheckbox = new CheckboxLabel(this);
	LightsCheckbox = new CheckboxLabel(this);
	BrightmapsCheckbox = new CheckboxLabel(this);
	WidescreenCheckbox = new CheckboxLabel(this);
	GamesList = new ListView(this);

	auto label = new TextLabel();
	label->SetText("Label:");
	label->SetFixedWidth(100.0);
	auto lineedit = new LineEdit(nullptr);

	WelcomeLabel->SetText("Welcome to VKDoom");
	VersionLabel->SetText("Version 0xdeadbabe.");
	SelectLabel->SetText("Select which game file (IWAD) to run.");

	GamesList->AddItem("Doom");
	GamesList->AddItem("Doom 2: Electric Boogaloo");
	GamesList->AddItem("Doom 3D");
	GamesList->AddItem("Doom 4: The Quest for Peace");
	GamesList->AddItem("Doom on Ice");
	GamesList->AddItem("The Doom");
	GamesList->AddItem("Doom 2");

	GeneralLabel->SetText("General");
	ExtrasLabel->SetText("Extra Graphics");
	FullscreenCheckbox->SetText("Fullscreen");
	DisableAutoloadCheckbox->SetText("Disable autoload");
	DontAskAgainCheckbox->SetText("Don't ask me again");
	LightsCheckbox->SetText("Lights");
	BrightmapsCheckbox->SetText("Brightmaps");
	WidescreenCheckbox->SetText("Widescreen");

	auto layout = new VBoxLayout(this);
	layout->AddWidget(WelcomeLabel);
	layout->AddWidget(VersionLabel);
	layout->AddWidget(SelectLabel);
	layout->AddWidget(GamesList);

	auto line = new HBoxLayout();
	line->AddWidget(label);
	line->AddWidget(lineedit);
	layout->AddLayout(line);

	auto leftPanel = new VBoxLayout();
	leftPanel->AddWidget(FullscreenCheckbox);
	leftPanel->AddWidget(DisableAutoloadCheckbox);
	leftPanel->AddWidget(DontAskAgainCheckbox);

	auto rightPanel = new VBoxLayout();
	rightPanel->AddWidget(LightsCheckbox);
	rightPanel->AddWidget(BrightmapsCheckbox);
	rightPanel->AddWidget(WidescreenCheckbox);

	auto panelLine = new HBoxLayout();
	panelLine->AddLayout(leftPanel);
	panelLine->AddLayout(rightPanel);

	layout->AddWidget(ExtrasLabel);
	layout->AddLayout(panelLine);

	//layout->AddStretch();

	SetLayout(layout);
}

void LauncherWindowTab2::OnGeometryChanged()
{
	/*
	double y = 0, h;

	h = WelcomeLabel->GetPreferredHeight();
	WelcomeLabel->SetFrameGeometry(20, y, GetWidth() - 40, h);
	y += h;

	h = VersionLabel->GetPreferredHeight();
	VersionLabel->SetFrameGeometry(20, y, GetWidth() - 40, h);
	y += h + 10;

	h = SelectLabel->GetPreferredHeight();
	SelectLabel->SetFrameGeometry(20, y, GetWidth() - 40, h);
	y += h;

	double listViewTop = y + 10, listViewBottom;

	y = GetHeight();

	h = DontAskAgainCheckbox->GetPreferredHeight();
	y -= h;
	DontAskAgainCheckbox->SetFrameGeometry(20, y, 190, h);
	WidescreenCheckbox->SetFrameGeometry(GetWidth() - 170, y, 150, WidescreenCheckbox->GetPreferredHeight());

	h = DisableAutoloadCheckbox->GetPreferredHeight();
	y -= h;
	DisableAutoloadCheckbox->SetFrameGeometry(20, y, 190, h);
	BrightmapsCheckbox->SetFrameGeometry(GetWidth() - 170, y, 150, BrightmapsCheckbox->GetPreferredHeight());

	h = FullscreenCheckbox->GetPreferredHeight();
	y -= h;
	FullscreenCheckbox->SetFrameGeometry(20, y, 190, h);
	LightsCheckbox->SetFrameGeometry(GetWidth() - 170, y, 150, LightsCheckbox->GetPreferredHeight());

	h = GeneralLabel->GetPreferredHeight();
	y -= h;
	GeneralLabel->SetFrameGeometry(20, y, 190, GeneralLabel->GetPreferredHeight());
	ExtrasLabel->SetFrameGeometry(GetWidth() - 170, y, 150, ExtrasLabel->GetPreferredHeight());

	listViewBottom = y - 10;
	GamesList->SetFrameGeometry(20, listViewTop, GetWidth() - 40, std::max<double>(listViewBottom - listViewTop, 0));
	*/
}

LauncherWindowTab3::LauncherWindowTab3(Widget parent): Widget(nullptr)
{
	Label = new TextLabel(this);
	Choices = new Dropdown(this);
	Popup = new PushButton(this);

	Label->SetText("Oh my, even more widgets");
	Popup->SetText("Click me.");

	Choices->SetMaxDisplayItems(2);
	Choices->AddItem("First");
	Choices->AddItem("Second");
	Choices->AddItem("Third");
	Choices->AddItem("Fourth");
	Choices->AddItem("Fifth");
	Choices->AddItem("Sixth");

	Choices->OnChanged = [this](int index) {
		std::cout << "Selected " << index << ":" << Choices->GetItem(index) << std::endl;
	};

	Popup->OnClick = []{
		std::cout << "TODO: open popup" << std::endl;
	};
}

void LauncherWindowTab3::OnGeometryChanged()
{
	double y = 0, h;

	y += 10;

	h = Label->GetPreferredHeight();
	Label->SetFrameGeometry(20, y, GetWidth() - 40, h);
	y += h + 10;

	h = Choices->GetPreferredHeight();
	Choices->SetFrameGeometry(20, y, Choices->GetPreferredWidth(), h);
	y += h + 10;

	h = Popup->GetPreferredHeight();
	Popup->SetFrameGeometry(20, y, 120, h);
	y += h;
}

// ************************************************************
// Shared code
// ************************************************************

#ifdef __APPLE__
std::vector<uint8_t> LoadSystemFontData()
{
	// Try common macOS system font paths
	const char* fontPaths[] = {
		"/System/Library/Fonts/SFNS.ttf",                    // San Francisco (modern macOS)
		"/System/Library/Fonts/SFNSText.ttf",                // San Francisco Text
		"/System/Library/Fonts/Helvetica.ttc",               // Helvetica (fallback)
		"/System/Library/Fonts/HelveticaNeue.ttc",           // Helvetica Neue (fallback)
		nullptr
	};

	for (const char** path = fontPaths; *path != nullptr; ++path)
	{
		try
		{
			return ReadAllBytes(std::string(*path));
		}
		catch (...)
		{
			// Try next font path
		}
	}

	return std::vector<uint8_t>();
}
#endif

std::vector<SingleFontData> LoadWidgetFontData(const std::string& name)
{
	// Font loading strategy controlled by CMake option ZWIDGET_USE_BUNDLED_FONTS
	// OFF (default): Try system font first, fall back to bundled font
	// ON: Try bundled font first, fall back to system font

#ifdef ZWIDGET_USE_BUNDLED_FONTS
	// Try bundled font first
	try
	{
		std::string filename = name + ".ttf";
		return {
			{ReadAllBytes(filename), ""}
		};
	}
	catch (...)
	{
		// Fall back to system font if bundled font not found
		std::cout << "Bundled font '" << name << "' not found, trying system font" << std::endl;
#if defined(__APPLE__) || defined(WIN32)
		std::vector<uint8_t> systemFont = LoadSystemFontData();
		if (!systemFont.empty())
		{
			return { {std::move(systemFont), ""} };
		}
#endif
		throw std::runtime_error("Failed to load font: " + name);
	}
#else
	// Try system font first (default behavior)
#if defined(__APPLE__) || defined(WIN32)
	std::vector<uint8_t> systemFont = LoadSystemFontData();
	if (!systemFont.empty())
	{
		return { {std::move(systemFont), ""} };
	}
#endif

	// Fall back to bundled font if system font not available
	try
	{
		std::string filename = name + ".ttf";
		return {
			{ReadAllBytes(filename), ""}
		};
	}
	catch (...)
	{
		throw std::runtime_error("Failed to load font: " + name);
	}
#endif
}

std::vector<uint8_t> LoadWidgetData(const std::string& name)
{
	return ReadAllBytes(name);
}

enum class Backend
{
	Default, Win32, SDL2, X11, Wayland
};

enum class Theme
{
	Default, Light, Dark
};

int example(Backend backend = Backend::Default, Theme theme = Theme::Default)
{
	// just for testing themes
	switch (theme)
	{
		case Theme::Default: WidgetTheme::SetTheme(std::make_unique<DarkWidgetTheme>()); break;
		case Theme::Dark:    WidgetTheme::SetTheme(std::make_unique<DarkWidgetTheme>()); break;
		case Theme::Light:   WidgetTheme::SetTheme(std::make_unique<LightWidgetTheme>()); break;
	}

	// just for testing backends
	switch (backend)
	{
		case Backend::Default: DisplayBackend::Set(DisplayBackend::TryCreateBackend()); break;
		case Backend::Win32:   DisplayBackend::Set(DisplayBackend::TryCreateWin32());   break;
		case Backend::SDL2:    DisplayBackend::Set(DisplayBackend::TryCreateSDL2());    break;
		case Backend::X11:     DisplayBackend::Set(DisplayBackend::TryCreateX11());     break;
		case Backend::Wayland: DisplayBackend::Set(DisplayBackend::TryCreateWayland()); break;
	}

	auto launcher = new LauncherWindow();
	launcher->SetFrameGeometry(100.0, 100.0, 615.0, 668.0);
	launcher->Show();

	DisplayWindow::RunLoop();

	return 0;
}

// ************************************************************
// Platform-specific code
// ************************************************************

#ifdef WIN32

#include <Windows.h>

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
	SetProcessDPIAware();
	example();
}

#else

#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>

int main(int argc, const char** argv)
{
	Backend backend = Backend::Default;
	Theme theme = Theme::Default;

	for (auto i = 1; i < argc; i++)
	{
		std::string s = argv[i];
		std::transform(s.begin(), s.end(), s.begin(),
			[](unsigned char c){ return std::tolower(c); });

		if (s == "light") { theme = Theme::Light; continue; }
		if (s == "dark")  { theme = Theme::Dark;  continue; }

		if (s == "sdl2")    { backend = Backend::SDL2;    continue; }
		if (s == "x11")     { backend = Backend::X11;     continue; }
		if (s == "wayland") { backend = Backend::Wayland; continue; }
		if (s == "win32")   { backend = Backend::Win32;   continue; } // lol
	}

	example(backend, theme);
}

#endif
