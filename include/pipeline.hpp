#pragma once

#include "my_types.hpp"
#include "utils.hpp"
#include <vulkan/vulkan.h>
#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

class Pipeline
{
	struct Private { explicit Private() = default; };
	// Wrapper that automatically destroys the shader module
	struct ShaderModuleWrapper {
		VkShaderModule data;
		VkDevice device;
		constexpr operator bool () const { return data; }
		constexpr operator VkShaderModule () const { return data; }
		ShaderModuleWrapper() = delete;
		ShaderModuleWrapper(const ShaderModuleWrapper &) = delete;
		ShaderModuleWrapper(ShaderModuleWrapper &&) = delete;
		ShaderModuleWrapper(const VkDevice vkDevice, const VkShaderModule shaderModule)
		{
			device = vkDevice;
			data = shaderModule;
		}
		~ShaderModuleWrapper()
		{
			if (data) {
				vkDestroyShaderModule(device, data, nullptr);
				data = nullptr;
			}
		}
	};
	struct DynamicStateWrapper {
		VkPipelineDynamicStateCreateInfo data;
		constexpr operator const VkPipelineDynamicStateCreateInfo* () const { return &data; }
		DynamicStateWrapper() = delete;
		DynamicStateWrapper(const DynamicStateWrapper &) = delete;
		DynamicStateWrapper(DynamicStateWrapper &&) = delete;
		DynamicStateWrapper(const std::vector<VkDynamicState> &states);
		~DynamicStateWrapper() = default;
	private:
		std::vector<VkDynamicState> states;
	};
public:
	Pipeline() = delete;
	Pipeline(const Pipeline &) = delete;
	Pipeline(Pipeline &&) = delete;
	Pipeline(Private) {}
	~Pipeline();

	template <typename VertexType>
	static PipelinePtr Create(const CorePtr core, const std::filesystem::path vertexShaderFilePath, const std::filesystem::path fragmentShaderFilePath)
	{
		auto vertexShaderBuffer = ReadFile(vertexShaderFilePath);
		auto fragmentShaderBuffer = ReadFile(fragmentShaderFilePath);
		return Pipeline::Create<VertexType>(core, vertexShaderBuffer, fragmentShaderBuffer);
	}
	template <typename VertexType>
	static PipelinePtr Create(const CorePtr core, const std::string &vertexShaderCode, const std::string &fragmentShaderCode)
	{
		return Pipeline::Create<VertexType>(core, std::vector<uint8_t>{ vertexShaderCode.begin(), vertexShaderCode.end() }, std::vector<uint8_t>{ fragmentShaderCode.begin(), fragmentShaderCode.end() });
	}
	template <typename VertexType>
	static PipelinePtr Create(const CorePtr core, const std::vector<uint8_t> &vertexShaderCode, const std::vector<uint8_t> &fragmentShaderCode)
	{
		auto ptr = std::make_shared<Pipeline>(Private());
		if (!ptr->Init(VertexType::GetBindingDescription(), VertexType::GetAttributeDescriptions(), core, vertexShaderCode, fragmentShaderCode))
			return nullptr;
		return ptr;
	}

	void Bind() const;

private:
	bool Init(const VkVertexInputBindingDescription &vertexInputBindingDescription, const std::vector<VkVertexInputAttributeDescription> &vertexInputAttributeDescriptions, const CorePtr core, const std::vector<uint8_t> &vertexShaderCode, const std::vector<uint8_t> &fragmentShaderCode);
	static Pipeline::ShaderModuleWrapper CreateShaderModule(const VkDevice vkDevice, const std::vector<uint8_t> &code);
	static constexpr std::array<VkPipelineShaderStageCreateInfo, 2> GetShadersStageCreateInfo(const VkShaderModule vertexShaderModule, const VkShaderModule fragmentShaderModule);
	static constexpr VkPipelineVertexInputStateCreateInfo GetVertexInputStateCreateInfo(const VkVertexInputBindingDescription &vertexInputBindingDescription, const std::vector<VkVertexInputAttributeDescription> &vertexInputAttributeDescriptions);
	static constexpr VkPipelineInputAssemblyStateCreateInfo GetInputAssemblyStateCreateInfo();
	static constexpr VkPipelineViewportStateCreateInfo GetViewportStateCreateInfo();
	static constexpr VkPipelineRasterizationStateCreateInfo GetRasterizationStateCreateInfo();
	static constexpr VkPipelineMultisampleStateCreateInfo GetMultisampleStateCreateInfo();
	static constexpr VkPipelineColorBlendAttachmentState GetColorBlendAttachmentState();
	static constexpr VkPipelineColorBlendStateCreateInfo GetColorBlendStateCreateInfo(const VkPipelineColorBlendAttachmentState *colorBlendAttachmentState);
	static VkPipelineLayout CreatePipelineLayout(const VkDevice vkDevice);

	CoreWeakPtr coreWeak;
	VkPipeline vkPipeline = VK_NULL_HANDLE;
	VkPipelineLayout vkPipelineLayout = VK_NULL_HANDLE;
	VkRenderPass vkRenderPass = VK_NULL_HANDLE;
};
