#pragma once

#include <string>
#include <vector>
#include <cstdint>

// WAD file metadata structure
struct WADMetadata
{
	bool isValid = false;
	bool isIWAD = false;
	std::string wadType; // "IWAD" or "PWAD"
	int numLumps = 0;
	int directoryOffset = 0;
	std::vector<std::string> mapNames;
	std::string gameName; // Detected game (DOOM, DOOM2, HERETIC, etc.)

	bool HasMaps() const { return !mapNames.empty(); }
	int MapCount() const { return static_cast<int>(mapNames.size()); }
};

// WAD file parser utility
class WADParser
{
public:
	// Parse WAD file and extract metadata
	static WADMetadata ParseWAD(const std::string& filepath);

	// Auto-detect IWADs in common directories
	static std::vector<std::string> FindIWADs();

	// Detect GZDoom executables in common locations
	static std::vector<std::string> FindGZDoom();

private:
	static std::string DetectGameName(const std::vector<std::string>& lumps);
	static std::vector<std::string> ExtractMapNames(const std::vector<std::string>& lumps);
	static std::string ReadString(std::ifstream& file, size_t length);
};
