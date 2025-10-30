
#include "core/resourcedata.h"
#include <stdexcept>
#include <vector>
#include <string>

#define WIN32_MEAN_AND_LEAN
#define NOMINMAX
#include <windows.h>
#include <ShlObj.h>

static std::wstring to_utf16(const std::string& str)
{
	if (str.empty()) return {};
	int needed = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
	if (needed == 0)
		throw std::runtime_error("MultiByteToWideChar failed");
	std::wstring result;
	result.resize(needed);
	needed = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &result[0], (int)result.size());
	if (needed == 0)
		throw std::runtime_error("MultiByteToWideChar failed");
	return result;
}

static std::vector<uint8_t> ReadAllBytes(const std::string& filename)
{
	HANDLE handle = CreateFile(to_utf16(filename).c_str(), FILE_READ_ACCESS, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (handle == INVALID_HANDLE_VALUE)
		throw std::runtime_error("Could not open " + filename);

	LARGE_INTEGER fileSize;
	BOOL result = GetFileSizeEx(handle, &fileSize);
	if (result == FALSE)
	{
		CloseHandle(handle);
		throw std::runtime_error("GetFileSizeEx failed");
	}

	std::vector<uint8_t> buffer(fileSize.QuadPart);

	DWORD bytesRead = 0;
	result = ReadFile(handle, buffer.data(), (DWORD)buffer.size(), &bytesRead, nullptr);
	if (result == FALSE || bytesRead != buffer.size())
	{
		CloseHandle(handle);
		throw std::runtime_error("ReadFile failed");
	}

	CloseHandle(handle);

	return buffer;
}

std::vector<uint8_t> LoadSystemFontData()
{
	std::vector<uint8_t> fontDataVector;

	// Try to load Segoe UI, the default Windows system font
	wchar_t fontsPath[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPathW(0, CSIDL_FONTS, nullptr, 0, fontsPath)))
	{
		std::wstring segoeUIPath = std::wstring(fontsPath) + L"\\segoeui.ttf";

		try
		{
			// Convert wstring path to string for ReadAllBytes
			int size_needed = WideCharToMultiByte(CP_UTF8, 0, segoeUIPath.c_str(), (int)segoeUIPath.length(), nullptr, 0, nullptr, nullptr);
			std::string strPath(size_needed, 0);
			WideCharToMultiByte(CP_UTF8, 0, segoeUIPath.c_str(), (int)segoeUIPath.length(), &strPath[0], size_needed, nullptr, nullptr);

			fontDataVector = ReadAllBytes(strPath);
		}
		catch (...)
		{
		}
	}

	return fontDataVector;
}
