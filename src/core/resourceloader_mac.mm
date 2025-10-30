#include "zwidget/core/resourcedata.h"
#include <fstream>
#include <stdexcept>
#include <vector>

#import <Cocoa/Cocoa.h>
#import <CoreText/CoreText.h>

static std::vector<uint8_t> ReadAllBytes(const std::string& filename);

static std::vector<uint8_t> LoadSystemFontData()
{
    std::cerr << "LoadSystemFontData: Attempting to load system font." << std::endl;
    std::vector<uint8_t> fontDataVector;
    @autoreleasepool {
        NSFont* systemFont = [NSFont systemFontOfSize:13.0]; // Use a default size
        if (!systemFont)
        {
            NSLog(@"Failed to get system font.");
            return fontDataVector;
        }

        CTFontRef ctFont = (__bridge CTFontRef)systemFont;
        CFURLRef fontURL = (CFURLRef)CTFontCopyAttribute(ctFont, kCTFontURLAttribute);
        if (!fontURL)
        {
            NSLog(@"Failed to get font URL from system font.");
            return fontDataVector;
        }

        // Convert CFURLRef to NSString path
        // __bridge_transfer transfers ownership to ARC, so no manual CFRelease is needed
        NSString* fontPath = (__bridge_transfer NSString*)CFURLCopyFileSystemPath(fontURL, kCFURLPOSIXPathStyle);
        if (!fontPath)
        {
            NSLog(@"Failed to convert font URL to file path.");
            CFRelease(fontURL);
            return fontDataVector;
        }

        // Read the font file data
        try
        {
            fontDataVector = ReadAllBytes(std::string([fontPath UTF8String]));
        }
        catch (const std::runtime_error& e)
        {
            NSLog(@"Error reading system font file: %s", e.what());
        }

        CFRelease(fontURL);
        // fontPath is __bridge_transfer, so it's autoreleased(ARC)
    }
    return fontDataVector;
}

static std::vector<uint8_t> ReadAllBytes(const std::string& filename)
{
    std::cerr << "ReadAllBytes: Attempting to read file: " << filename << std::endl;
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file)
        throw std::runtime_error("ReadFile failed: " + filename);

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
        throw std::runtime_error("ReadFile failed: " + filename);

    return buffer;
}

std::vector<SingleFontData> ResourceLoader::LoadWidgetFontData(const std::string& name)
{
    if (name == "SystemDefault")
    {
        std::vector<uint8_t> rawData = LoadSystemFontData();
        if (!rawData.empty())
        {
            return { SingleFontData{std::move(rawData), ""} };
        }
        throw std::runtime_error("Failed to load system default font.");
    }
    return {
        SingleFontData{ReadAllBytes(name), ""}
    };
}

std::vector<uint8_t> ResourceLoader::LoadWidgetData(const std::string& name)
{
    std::cerr << "ResourceLoader::LoadWidgetData: Loading resource: " << name << std::endl;
    return ReadAllBytes(name);
}
