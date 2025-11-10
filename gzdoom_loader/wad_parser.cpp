#include "wad_parser.h"
#include <fstream>
#include <cstring>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#endif

// WAD file header structure
#pragma pack(push, 1)
struct WADHeader
{
	char identification[4]; // "IWAD" or "PWAD"
	uint32_t numLumps;
	uint32_t directoryOffset;
};

struct LumpInfo
{
	uint32_t filePos;
	uint32_t size;
	char name[8];
};
#pragma pack(pop)

WADMetadata WADParser::ParseWAD(const std::string& filepath)
{
	WADMetadata metadata;

	std::ifstream file(filepath, std::ios::binary);
	if (!file.is_open())
		return metadata;

	// Read WAD header
	WADHeader header;
	file.read(reinterpret_cast<char*>(&header), sizeof(WADHeader));

	if (file.gcount() != sizeof(WADHeader))
		return metadata;

	// Validate identification
	std::string wadType(header.identification, 4);
	if (wadType != "IWAD" && wadType != "PWAD")
		return metadata;

	metadata.isValid = true;
	metadata.isIWAD = (wadType == "IWAD");
	metadata.wadType = wadType;
	metadata.numLumps = header.numLumps;
	metadata.directoryOffset = header.directoryOffset;

	// Read lump directory
	file.seekg(header.directoryOffset, std::ios::beg);

	std::vector<std::string> lumpNames;
	lumpNames.reserve(header.numLumps);

	for (uint32_t i = 0; i < header.numLumps; i++)
	{
		LumpInfo lump;
		file.read(reinterpret_cast<char*>(&lump), sizeof(LumpInfo));

		if (file.gcount() != sizeof(LumpInfo))
			break;

		// Convert lump name to string (null-terminated or 8 chars)
		std::string lumpName;
		for (int j = 0; j < 8 && lump.name[j] != '\0'; j++)
		{
			lumpName += lump.name[j];
		}
		lumpNames.push_back(lumpName);
	}

	file.close();

	// Extract metadata
	metadata.mapNames = ExtractMapNames(lumpNames);
	metadata.gameName = DetectGameName(lumpNames);

	return metadata;
}

std::string WADParser::DetectGameName(const std::vector<std::string>& lumps)
{
	// Check for specific lumps that identify games
	for (const auto& lump : lumps)
	{
		if (lump == "E1M1") return "DOOM";
		if (lump == "MAP01") return "DOOM2";
		if (lump == "E1M1" && std::find(lumps.begin(), lumps.end(), "MUS_E1M1") != lumps.end()) return "HERETIC";
		if (lump == "MAP01" && std::find(lumps.begin(), lumps.end(), "STARTUP") != lumps.end()) return "HEXEN";
	}

	// Check for title lumps
	for (const auto& lump : lumps)
	{
		if (lump == "TITLEPIC") return "DOOM-based";
		if (lump == "TITLE") return "Heretic/Hexen-based";
	}

	return "Unknown";
}

std::vector<std::string> WADParser::ExtractMapNames(const std::vector<std::string>& lumps)
{
	std::vector<std::string> maps;

	for (const auto& lump : lumps)
	{
		// Doom 1/Heretic format: ExMx (Episode x, Map x)
		if (lump.length() == 4 && lump[0] == 'E' && lump[2] == 'M' &&
			std::isdigit(lump[1]) && std::isdigit(lump[3]))
		{
			maps.push_back(lump);
		}
		// Doom 2/Hexen format: MAPxx
		else if (lump.length() == 5 && lump.substr(0, 3) == "MAP" &&
			std::isdigit(lump[3]) && std::isdigit(lump[4]))
		{
			maps.push_back(lump);
		}
	}

	return maps;
}

std::vector<std::string> WADParser::FindIWADs()
{
	std::vector<std::string> iwads;
	std::vector<std::string> searchPaths;

#ifdef _WIN32
	// Windows common locations
	char programFiles[MAX_PATH];
	if (SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILES, NULL, 0, programFiles) == S_OK)
	{
		searchPaths.push_back(std::string(programFiles) + "\\Doom");
		searchPaths.push_back(std::string(programFiles) + "\\GZDoom");
		searchPaths.push_back(std::string(programFiles) + "\\Steam\\steamapps\\common\\Doom 2");
		searchPaths.push_back(std::string(programFiles) + "\\Steam\\steamapps\\common\\Ultimate Doom");
	}
	searchPaths.push_back("C:\\Games\\Doom");
	searchPaths.push_back("C:\\Doom");

#elif defined(__APPLE__)
	// macOS common locations
	const char* home = getenv("HOME");
	if (home)
	{
		// GZDoom config directories
		searchPaths.push_back(std::string(home) + "/Library/Application Support/GZDoom");
		searchPaths.push_back(std::string(home) + "/Library/Application Support/Doom");
		searchPaths.push_back(std::string(home) + "/.config/gzdoom");

		// Steam installations (multiple possible locations)
		searchPaths.push_back(std::string(home) + "/Library/Application Support/Steam/steamapps/common/Ultimate Doom");
		searchPaths.push_back(std::string(home) + "/Library/Application Support/Steam/steamapps/common/Doom 2");
		searchPaths.push_back(std::string(home) + "/Library/Application Support/Steam/steamapps/common/Final Doom");
		searchPaths.push_back(std::string(home) + "/Library/Application Support/Steam/steamapps/common/Heretic");
		searchPaths.push_back(std::string(home) + "/Library/Application Support/Steam/steamapps/common/Hexen");
		searchPaths.push_back(std::string(home) + "/Library/Application Support/Steam/steamapps/common/DOOM 3 BFG Edition/base/wads");

		// User directories
		searchPaths.push_back(std::string(home) + "/Documents/Doom");
		searchPaths.push_back(std::string(home) + "/Games/Doom");
		searchPaths.push_back(std::string(home) + "/doom");
	}

	// System-wide locations
	searchPaths.push_back("/Applications/GZDoom.app/Contents/MacOS");
	searchPaths.push_back("/Applications/GZDoom.app/Contents/Resources");
	searchPaths.push_back("/usr/local/share/games/doom");
	searchPaths.push_back("/usr/share/games/doom");
	searchPaths.push_back("/opt/doom");

	// Homebrew installations
	searchPaths.push_back("/opt/homebrew/share/games/doom");
	searchPaths.push_back("/usr/local/opt/gzdoom/share/doom");

#else
	// Linux common locations
	const char* home = getenv("HOME");
	if (home)
	{
		searchPaths.push_back(std::string(home) + "/.config/gzdoom");
		searchPaths.push_back(std::string(home) + "/.local/share/games/doom");
		searchPaths.push_back(std::string(home) + "/doom");
	}
	searchPaths.push_back("/usr/share/games/doom");
	searchPaths.push_back("/usr/local/share/games/doom");
	searchPaths.push_back("/usr/share/doom");
	searchPaths.push_back("/opt/doom");
	// Flatpak/Snap locations
	if (home)
	{
		searchPaths.push_back(std::string(home) + "/.var/app/org.zdoom.GZDoom/data/gzdoom");
	}
#endif

	// Common IWAD filenames to search for
	std::vector<std::string> iwadFiles = {
		"doom.wad", "DOOM.WAD",
		"doom2.wad", "DOOM2.WAD",
		"doom2f.wad", "DOOM2F.WAD",
		"plutonia.wad", "PLUTONIA.WAD",
		"tnt.wad", "TNT.WAD",
		"heretic.wad", "HERETIC.WAD",
		"hexen.wad", "HEXEN.WAD",
		"hexdd.wad", "HEXDD.WAD",
		"strife1.wad", "STRIFE1.WAD",
		"freedoom1.wad", "FREEDOOM1.WAD",
		"freedoom2.wad", "FREEDOOM2.WAD"
	};

	// Search for IWADs
	for (const auto& path : searchPaths)
	{
		for (const auto& iwadFile : iwadFiles)
		{
			std::string fullPath = path + "/" + iwadFile;

			// Check if file exists
			std::ifstream test(fullPath);
			if (test.good())
			{
				test.close();
				// Verify it's actually a valid IWAD
				auto metadata = ParseWAD(fullPath);
				if (metadata.isValid && metadata.isIWAD)
				{
					iwads.push_back(fullPath);
				}
			}
		}
	}

	return iwads;
}

std::vector<std::string> WADParser::FindGZDoom()
{
	std::vector<std::string> executables;

#ifdef _WIN32
	std::vector<std::string> searchPaths;
	char programFiles[MAX_PATH];
	if (SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILES, NULL, 0, programFiles) == S_OK)
	{
		searchPaths.push_back(std::string(programFiles) + "\\GZDoom");
		searchPaths.push_back(std::string(programFiles) + "\\Doom");
	}
	searchPaths.push_back("C:\\Games\\GZDoom");
	searchPaths.push_back("C:\\GZDoom");

	for (const auto& path : searchPaths)
	{
		std::string exePath = path + "\\gzdoom.exe";
		std::ifstream test(exePath);
		if (test.good())
		{
			test.close();
			executables.push_back(exePath);
		}
	}

#elif defined(__APPLE__)
	std::vector<std::string> searchPaths = {
		"/Applications/GZDoom.app/Contents/MacOS/gzdoom",
		"/usr/local/bin/gzdoom",
		"/opt/homebrew/bin/gzdoom"
	};

	const char* home = getenv("HOME");
	if (home)
	{
		searchPaths.push_back(std::string(home) + "/Applications/GZDoom.app/Contents/MacOS/gzdoom");
	}

	for (const auto& path : searchPaths)
	{
		std::ifstream test(path);
		if (test.good())
		{
			test.close();
			executables.push_back(path);
		}
	}

#else
	// Linux - check common install locations
	std::vector<std::string> searchPaths = {
		"/usr/bin/gzdoom",
		"/usr/local/bin/gzdoom",
		"/usr/games/gzdoom",
		"/opt/gzdoom/gzdoom",
		"/snap/bin/gzdoom",
		"/var/lib/flatpak/exports/bin/org.zdoom.GZDoom"
	};

	const char* home = getenv("HOME");
	if (home)
	{
		searchPaths.push_back(std::string(home) + "/.local/bin/gzdoom");
		searchPaths.push_back(std::string(home) + "/bin/gzdoom");
	}

	for (const auto& path : searchPaths)
	{
		// Check if file exists and is executable
		struct stat sb;
		if (stat(path.c_str(), &sb) == 0 && (sb.st_mode & S_IXUSR))
		{
			executables.push_back(path);
		}
	}
#endif

	return executables;
}
