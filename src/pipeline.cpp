#include "core.hpp"
#include "pipeline.hpp"
#include "utils.hpp"
#include <iostream>

Pipeline::~Pipeline()
{
	if (auto core = this->coreWeak.lock()) {
		const auto vkDevice = core->GetVulkanDevice();
		if (this->vkPipeline) {
			vkDestroyPipeline(vkDevice, this->vkPipeline, nullptr);
			this->vkPipeline = VK_NULL_HANDLE;
		}
		if (this->vkPipelineLayout) {
			vkDestroyPipelineLayout(vkDevice, this->vkPipelineLayout, nullptr);
			this->vkPipelineLayout = VK_NULL_HANDLE;
		}
	}
}

void Pipeline::Bind() const
{
	if (const auto core = this->coreWeak.lock()) {
		const auto vkCommandBuffer = core->GetVulkanCurrentFrameCommandBuffer();
		vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->vkPipeline);

		VkExtent2D swapChainExtent = {
			.width = core->GetWidth(),
			.height = core->GetHeight()
		};

		VkViewport viewport = {
			.x = 0.0f,
			.y = 0.0f,
			.width = static_cast<float>(swapChainExtent.width),
			.height = static_cast<float>(swapChainExtent.height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
		vkCmdSetViewport(vkCommandBuffer, 0, 1, &viewport);

		VkRect2D scissor = {
			.offset = {0, 0},
			.extent = swapChainExtent
		};
		vkCmdSetScissor(vkCommandBuffer, 0, 1, &scissor);
	}
}

bool Pipeline::Init(const VkVertexInputBindingDescription &vertexInputBindingDescription,
	const std::vector<VkVertexInputAttributeDescription> &vertexInputAttributeDescriptions,
	const CorePtr core, const std::vector<uint8_t> &vertexShaderCode,
	const std::vector<uint8_t> &fragmentShaderCode)
{
	if (!core->GetVulkanDevice())
		return false;

	this->coreWeak = core;
	const auto vkDevice = core->GetVulkanDevice();

	const auto vertexShaderModule = CreateShaderModule(vkDevice, vertexShaderCode);
	const auto fragmentShaderModule = CreateShaderModule(vkDevice, fragmentShaderCode);
	if (!vertexShaderModule || !fragmentShaderModule)
		return false;

	const auto shaderStages = GetShadersStageCreateInfo(vertexShaderModule, fragmentShaderModule);
	const auto dynamicState = Pipeline::DynamicStateWrapper({VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR});
	const auto vertexInputState = GetVertexInputStateCreateInfo(vertexInputBindingDescription, vertexInputAttributeDescriptions);
	const auto inputAssemblyState = GetInputAssemblyStateCreateInfo();
	const auto viewportState = GetViewportStateCreateInfo();
	const auto rasterizationState = GetRasterizationStateCreateInfo();
	const auto multisampleState = GetMultisampleStateCreateInfo();
	const auto colorBlendAttachmentState = GetColorBlendAttachmentState();
	const auto colorBlendState = GetColorBlendStateCreateInfo(&colorBlendAttachmentState);

	this->vkPipelineLayout = CreatePipelineLayout(vkDevice);
	const auto vkRenderPass = core->GetVulkanRenderPass();

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stageCount = 2,
		.pStages = shaderStages.data(),
		.pVertexInputState = &vertexInputState,
		.pInputAssemblyState = &inputAssemblyState,
		.pTessellationState = nullptr,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizationState,
		.pMultisampleState = &multisampleState,
		.pDepthStencilState = nullptr,
		.pColorBlendState = &colorBlendState,
		.pDynamicState = dynamicState,
		.layout = vkPipelineLayout,
		.renderPass = vkRenderPass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1
	};

	if (!CHECK_VK_RESULT(vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &this->vkPipeline))) {
		std::cerr << "Vulkan: Failed to create pipeline" << std::endl;
		return false;
	}

	return true;
}

Pipeline::ShaderModuleWrapper Pipeline::CreateShaderModule(const VkDevice vkDevice, const std::vector<uint8_t> &code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	VkShaderModule shaderModule;
	if (!CHECK_VK_RESULT(vkCreateShaderModule(vkDevice, &createInfo, nullptr, &shaderModule))) {
		std::cerr << "Vulkan: Failed to create shader module" << std::endl;
		return Pipeline::ShaderModuleWrapper(VK_NULL_HANDLE, VK_NULL_HANDLE);;
	}

	return Pipeline::ShaderModuleWrapper(vkDevice, shaderModule);
}
constexpr std::array<VkPipelineShaderStageCreateInfo, 2> Pipeline::GetShadersStageCreateInfo(const VkShaderModule vertexShaderModule, const VkShaderModule fragmentShaderModule)
{
	return {
		VkPipelineShaderStageCreateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vertexShaderModule,
			.pName = "main",
			.pSpecializationInfo = nullptr
		},
		VkPipelineShaderStageCreateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = fragmentShaderModule,
			.pName = "main",
			.pSpecializationInfo = nullptr
		}
	};
}
constexpr VkPipelineVertexInputStateCreateInfo Pipeline::GetVertexInputStateCreateInfo(
		const VkVertexInputBindingDescription &vertexInputBindingDescription,
		const std::vector<VkVertexInputAttributeDescription> &vertexInputAttributeDescriptions)
{
	return VkPipelineVertexInputStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &vertexInputBindingDescription,
		.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributeDescriptions.size()),
		.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data()
	};
}
constexpr VkPipelineInputAssemblyStateCreateInfo Pipeline::GetInputAssemblyStateCreateInfo()
{
	return VkPipelineInputAssemblyStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};
}
constexpr VkPipelineViewportStateCreateInfo Pipeline::GetViewportStateCreateInfo()
{
	return VkPipelineViewportStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.viewportCount = 1,
		.pViewports = nullptr,
		.scissorCount = 1,
		.pScissors = nullptr
	};
}
constexpr VkPipelineRasterizationStateCreateInfo Pipeline::GetRasterizationStateCreateInfo()
{
	return VkPipelineRasterizationStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp = 0.0f,
		.depthBiasSlopeFactor = 0.0f,
		.lineWidth = 1.0f
	};
}
constexpr VkPipelineMultisampleStateCreateInfo Pipeline::GetMultisampleStateCreateInfo()
{
	return VkPipelineMultisampleStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 0.0f,
		.pSampleMask = nullptr,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE
	};
}
constexpr VkPipelineColorBlendAttachmentState Pipeline::GetColorBlendAttachmentState()
{
	return VkPipelineColorBlendAttachmentState{
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};
}
constexpr VkPipelineColorBlendStateCreateInfo Pipeline::GetColorBlendStateCreateInfo(const VkPipelineColorBlendAttachmentState *colorBlendAttachmentState)
{
	return VkPipelineColorBlendStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = colorBlendAttachmentState,
		.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f }
	};
}

VkPipelineLayout Pipeline::CreatePipelineLayout(const VkDevice vkDevice)
{
	VkPipelineLayout pipelineLayout;
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = 0,
		.pSetLayouts = nullptr,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr
	};
	if (!CHECK_VK_RESULT(vkCreatePipelineLayout(vkDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout))) {
		std::cerr << "Vulkan: Failed to create pipeline layout" << std::endl;
		return nullptr;
	}
	return pipelineLayout;
}

Pipeline::DynamicStateWrapper::DynamicStateWrapper(const std::vector<VkDynamicState> &states) : states(std::move(states))
{
	this->data = VkPipelineDynamicStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.dynamicStateCount = static_cast<uint32_t>(this->states.size()),
		.pDynamicStates = this->states.data()
	};
}
