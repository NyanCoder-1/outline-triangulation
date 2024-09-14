#pragma once

#include <vulkan/vk_enum_string_helper.h>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <vector>

#ifdef __INLINE_CHECK_VK_RESULT__
constexpr bool print_vk_result(std::string name, VkResult result)
{
	if (result != VK_SUCCESS) {
		std::cerr << "Vulkan: Failed to run command `" << name << "`: " << string_VkResult(result) << std::endl;
		return false;
	}
	return true;
}
#else // __INLINE_CHECK_VK_RESULT__
bool print_vk_result(std::string name, VkResult result);
#endif // __INLINE_CHECK_VK_RESULT__
#define CHECK_VK_RESULT(result) print_vk_result(#result, result)

std::vector<uint8_t> ReadFile(const std::filesystem::path& filePath);
