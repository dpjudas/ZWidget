
#include "gzdoom_launcher.h"
#include "zwidget/window/window.h"
#include "zwidget/core/theme.h"
#include "zwidget/core/resourcedata.h"
#include <stdexcept>
#include <fstream>
#include <vector>
#include <cstdint>
#include <string>

#ifdef __linux__
#include <fontconfig/fontconfig.h>
#endif

// Custom ResourceLoader that doesn't depend on GTK
class CustomResourceLoader : public ResourceLoader
{
public:
	std::vector<SingleFontData> LoadFont(const std::string& name) override
	{
		std::string fontPath;

#ifdef __linux__
		// Try to use fontconfig directly (without GTK dependency)
		if (name == "system" || name == "monospace")
		{
			const char* fcName = (name == "monospace") ? "monospace" : "sans-serif";

			FcConfig* config = FcInitLoadConfigAndFonts();
			if (config)
			{
				FcPattern* pat = FcNameParse((const FcChar8*)fcName);
				if (pat)
				{
					FcConfigSubstitute(config, pat, FcMatchPattern);
					FcDefaultSubstitute(pat);
					FcResult res;
					FcPattern* font = FcFontMatch(config, pat, &res);
					if (font)
					{
						FcChar8* file = nullptr;
						if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch)
							fontPath = (const char*)file;
						FcPatternDestroy(font);
					}
					FcPatternDestroy(pat);
				}
				FcConfigDestroy(config);
			}
		}

		// Fallback to DejaVu fonts if fontconfig fails
		if (fontPath.empty())
		{
			if (name == "monospace")
				fontPath = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf";
			else
				fontPath = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
		}
#elif defined(_WIN32)
		// Windows fallback
		if (name == "monospace")
			fontPath = "C:\\Windows\\Fonts\\consola.ttf"; // Consolas
		else
			fontPath = "C:\\Windows\\Fonts\\segoeui.ttf"; // Segoe UI
#else
		// macOS fallback
		if (name == "monospace")
			fontPath = "/System/Library/Fonts/Monaco.ttf";
		else
			fontPath = "/System/Library/Fonts/HelveticaNeue.ttc";
#endif

		if (!fontPath.empty())
		{
			SingleFontData fontdata;
			fontdata.fontdata = ReadAllBytes(fontPath);
			return { std::move(fontdata) };
		}

		throw std::runtime_error("Could not load font: " + name);
	}

	std::vector<uint8_t> ReadAllBytes(const std::string& filename) override
	{
		std::ifstream file(filename, std::ios::binary | std::ios::ate);
		if (!file)
			throw std::runtime_error("Could not open file: " + filename);

		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::vector<uint8_t> buffer(size);
		if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
			throw std::runtime_error("Could not read file: " + filename);

		return buffer;
	}
};

int main(int argc, char** argv)
{
	try
	{
		// Initialize display backend
		DisplayBackend::Set(DisplayBackend::TryCreateBackend());

#ifdef __linux__
		// Install custom resource loader on Linux only (fixes GTK dependency issue)
		// macOS and Windows use their native ZWidget resource loaders
		ResourceLoader::Set(std::make_unique<CustomResourceLoader>());
#endif

		// Set dark theme by default
		WidgetTheme::SetTheme(std::make_unique<DarkWidgetTheme>());

		// Create and show the launcher window
		auto launcher = new GZDoomLauncher();
		launcher->SetFrameGeometry(100.0, 100.0, 1100.0, 750.0);
		launcher->Show();

		// Run the event loop
		DisplayWindow::RunLoop();

		return 0;
	}
	catch (const std::exception& e)
	{
		printf("Error: %s\n", e.what());
		return 1;
	}
}
