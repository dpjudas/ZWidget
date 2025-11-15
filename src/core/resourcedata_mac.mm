#include "zwidget/core/resourcedata.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>

#import <Cocoa/Cocoa.h>
#import <CoreText/CoreText.h>

extern "C"
{
	typedef const void* CFTypeRef;
	void CFRelease(CFTypeRef cf);
}

std::vector<SingleFontData> ResourceData::LoadSystemFont()
{
	SingleFontData fontData;
	@autoreleasepool
	{
		NSFont* systemFont = [NSFont systemFontOfSize:13.0]; // Use a default size
		if (!systemFont)
			throw std::runtime_error("Failed to get system font");

		CTFontRef ctFont = (__bridge CTFontRef)systemFont;
		CFURLRef fontURL = (CFURLRef)CTFontCopyAttribute(ctFont, kCTFontURLAttribute);

		std::string fontPath;

		if (fontURL)
		{
			CFStringRef cfPath = CFURLCopyFileSystemPath(fontURL, kCFURLPOSIXPathStyle);
			if (cfPath)
			{
				fontPath = std::string([(NSString*)cfPath UTF8String]);
				CFRelease(cfPath);
			}
			CFRelease(fontURL);
		}

		// Fallback to known macOS font paths if system font URL lookup fails
		// This handles newer macOS versions where system fonts might be embedded
		if (fontPath.empty())
		{
			// Try common macOS system font locations
			const char* fallbackPaths[] = {
				"/System/Library/Fonts/SFNS.ttf",                    // San Francisco (newer macOS)
				"/System/Library/Fonts/SFNSText.ttf",                // San Francisco Text
				"/System/Library/Fonts/Helvetica.ttc",               // Helvetica
				"/System/Library/Fonts/HelveticaNeue.ttc",           // Helvetica Neue
				"/System/Library/Fonts/LucidaGrande.ttc",            // Lucida Grande (older macOS)
				"/Library/Fonts/Arial.ttf",                          // Arial fallback
			};

			for (const char* path : fallbackPaths)
			{
				std::ifstream test(path);
				if (test.good())
				{
					fontPath = path;
					break;
				}
			}
		}

		if (fontPath.empty())
			throw std::runtime_error("Could not find system font");

		fontData.fontdata = ReadAllBytes(fontPath);
	}
	return { fontData };
}

std::vector<SingleFontData> ResourceData::LoadMonospaceSystemFont()
{
	SingleFontData fontData;
	@autoreleasepool
	{
					NSFont* systemFont = nil;
					if (@available(macOS 10.15, *)) {
						systemFont = [NSFont monospacedSystemFontOfSize:13.0 weight:NSFontWeightRegular]; // Use a default size
					}
					if (!systemFont) {
						// Fallback for older macOS versions
						systemFont = [NSFont systemFontOfSize:13.0];
					}
					if (!systemFont) // Double check after fallback
						throw std::runtime_error("Failed to get system font");
		
					CTFontRef ctFont = (__bridge CTFontRef)systemFont;		CFURLRef fontURL = (CFURLRef)CTFontCopyAttribute(ctFont, kCTFontURLAttribute);

		std::string fontPath;

		if (fontURL)
		{
			CFStringRef cfPath = CFURLCopyFileSystemPath(fontURL, kCFURLPOSIXPathStyle);
			if (cfPath)
			{
				fontPath = std::string([(NSString*)cfPath UTF8String]);
				CFRelease(cfPath);
			}
			CFRelease(fontURL);
		}

		// Fallback to known macOS monospace font paths if system font URL lookup fails
		if (fontPath.empty())
		{
			// Try common macOS monospace font locations
			const char* fallbackPaths[] = {
				"/System/Library/Fonts/SFNSMono.ttf",                // SF Mono (newer macOS)
				"/System/Library/Fonts/Monaco.ttf",                  // Monaco
				"/Library/Fonts/Courier New.ttf",                    // Courier New
				"/System/Library/Fonts/Courier.dfont",               // Courier fallback
				"/Library/Fonts/Menlo.ttc",                          // Menlo
			};

			for (const char* path : fallbackPaths)
			{
				std::ifstream test(path);
				if (test.good())
				{
					fontPath = path;
					break;
				}
			}
		}

		if (fontPath.empty())
			throw std::runtime_error("Could not find monospace system font");

		fontData.fontdata = ReadAllBytes(fontPath);
	}
	return { std::move(fontData) };
}

double ResourceData::GetSystemFontSize()
{
	return [NSFont systemFontSize];
}

class ResourceLoaderMac : public ResourceLoader
{
public:
	std::vector<SingleFontData> LoadFont(const std::string& name)
	{
		if (name == "system")
			return ResourceData::LoadSystemFont();
		else if (name == "monospace")
			return ResourceData::LoadMonospaceSystemFont();
		else
			return { SingleFontData{ReadAllBytes(name + ".ttf"), ""} };
	}

	std::vector<uint8_t> ReadAllBytes(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::binary | std::ios::ate);
		if (!file)
			throw std::runtime_error("Could not open: " + filename);

		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::vector<uint8_t> buffer(size);
		if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
			throw std::runtime_error("Could not read: " + filename);

		return buffer;
	}
};

struct ResourceDefaultLoader
{
	ResourceDefaultLoader() { loader = std::make_unique<ResourceLoaderMac>(); }
	std::unique_ptr<ResourceLoader> loader;
};

static std::unique_ptr<ResourceLoader>& GetLoader()
{
	static ResourceDefaultLoader loader;
	return loader.loader;
}

ResourceLoader* ResourceLoader::Get()
{
	return GetLoader().get();
}

void ResourceLoader::Set(std::unique_ptr<ResourceLoader> instance)
{
	GetLoader() = std::move(instance);
}
