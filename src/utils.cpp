#include "utils.hpp"
#include <fstream>
#include <iostream>
#include <string>

#ifndef __INLINE_CHECK_VK_RESULT__
bool print_vk_result(std::string name, VkResult result)
{
	if (result != VK_SUCCESS) {
		std::cerr << "Vulkan: Failed to run command `" << name << "`: " << string_VkResult(result) << std::endl;
		return false;
	}
	return true;
}
#endif // __INLINE_CHECK_VK_RESULT__

std::vector<uint8_t> ReadFile(const std::filesystem::path& filePath)
{
	std::ifstream file(filePath.string(), std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "File: " << filePath.string() << " failed to open" << std::endl;
		return {};
	}
	std::size_t fileSize = static_cast<std::size_t>(file.tellg());
	std::vector<uint8_t> buffer(fileSize);
	file.seekg(0);
	file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
	file.close();

	return buffer;
}
