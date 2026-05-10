#include "zwidget/core/resourcedata.h"
#include <fstream>
#include <stdexcept>
#include <vector>
#include <string>
#include <cmath>
#include <Font.h>
#include <InterfaceDefs.h>

static std::vector<uint8_t> ReadAllBytes(const std::string& filename)
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

std::vector<SingleFontData> ResourceData::LoadSystemFont()
{
    return { SingleFontData{ReadAllBytes("/system/data/fonts/ttfonts/NotoSans-Regular.ttf"), ""} };
}

std::vector<SingleFontData> ResourceData::LoadMonospaceSystemFont()
{
    return { SingleFontData{ReadAllBytes("/system/data/fonts/ttfonts/NotoSansMono-Regular.ttf"), ""} };
}

double ResourceData::GetSystemFontSize()
{
	return be_plain_font ? be_plain_font->Size() : 11.0;
}

class ResourceLoaderHaiku : public ResourceLoader
{
public:
	std::vector<SingleFontData> LoadFont(const std::string& name) override
	{
		if (name == "system")
			return ResourceData::LoadSystemFont();
		else if (name == "monospace")
			return ResourceData::LoadMonospaceSystemFont();
		else
			return { SingleFontData{ReadAllBytes(name + ".ttf"), ""} };
	}

	std::vector<uint8_t> ReadAllBytes(const std::string& filename) override
	{
		return ::ReadAllBytes(filename);
	}
};

struct ResourceDefaultLoader
{
	ResourceDefaultLoader() { loader = std::make_unique<ResourceLoaderHaiku>(); }
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
