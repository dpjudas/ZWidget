#include "zwidget/core/resourcedata.h"
#include <fstream>
#include <stdexcept>
#include <vector>
#include <string>

std::vector<uint8_t> ReadAllBytes(const std::string& filename)
{
	std::ifstream file(filename, std::ios::binary | std::ios::ate);
	if (!file)
		throw std::runtime_error("ReadFile failed");

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<uint8_t> buffer(size);
	if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
		throw std::runtime_error("ReadFile failed ");

	return buffer;
}
