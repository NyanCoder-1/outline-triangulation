#include "core.hpp"
#include "mesh.hpp"
#include "utils.hpp"
#include <cstring>
#include <iostream>

Mesh::~Mesh()
{
	if (const auto core = this->coreWeak.lock()) {
		const auto vkDevice = core->GetVulkanDevice();
		if (this->vertexBuffer) {
			vkDestroyBuffer(vkDevice, this->vertexBuffer, nullptr);
			this->vertexBuffer = VK_NULL_HANDLE;
		}
		if (this->vertexBufferMemory) {
			vkFreeMemory(vkDevice, this->vertexBufferMemory, nullptr);
			this->vertexBufferMemory = VK_NULL_HANDLE;
		}
		if (this->indexBuffer) {
			vkDestroyBuffer(vkDevice, this->indexBuffer, nullptr);
			this->indexBuffer = VK_NULL_HANDLE;
		}
		if (this->indexBufferMemory) {
			vkFreeMemory(vkDevice, this->indexBufferMemory, nullptr);
			this->indexBufferMemory = VK_NULL_HANDLE;
		}
	}
}

void Mesh::Bind()
{
	if (const auto core = this->coreWeak.lock()) {
		const auto vkCommandBuffer = core->GetVulkanCurrentFrameCommandBuffer();

		VkBuffer vertexBuffers[] = {this->vertexBuffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(vkCommandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(vkCommandBuffer, this->indexBuffer, 0, VK_INDEX_TYPE_UINT16);
	}
}

void Mesh::Draw()
{
	if (const auto core = this->coreWeak.lock()) {
		const auto vkCommandBuffer = core->GetVulkanCurrentFrameCommandBuffer();

		VkBuffer vertexBuffers[] = { this->vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(vkCommandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(vkCommandBuffer, this->indexBuffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(vkCommandBuffer, indexCount, 1, 0, 0, 0);
	}
}

bool Mesh::Init(const CorePtr core, const Mesh::Vertices &vertices, const Mesh::Indices &indices)
{
	if (!core->GetVulkanDevice())
		return false;
	
	this->coreWeak = core;

	if (!this->CreateVertexBuffer(core, vertices))
		return false;
	if (!this->CreateIndexBuffer(core, indices))
		return false;

	indexCount = static_cast<uint32_t>(indices.size());

	return true;
}
bool Mesh::CreateVertexBuffer(const CorePtr core, const Mesh::Vertices &vertices)
{
	const auto vkPhysicalDevice = core->GetVulkanPhysicalDevice();
	const auto vkDevice = core->GetVulkanDevice();
	const auto vkGraphicsQueue = core->GetVulkanGraphicsQueue();
	const auto vkCommandPool = core->GetVulkanCommandPool();

	VkDeviceSize bufferSize = sizeof(Mesh::Vertices::value_type) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	if (!CreateBuffer(vkPhysicalDevice, vkDevice, bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory))
		return false;

	void *data;
	vkMapMemory(vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (std::size_t)bufferSize);
	vkUnmapMemory(vkDevice, stagingBufferMemory);

	if (!CreateBuffer(vkPhysicalDevice, vkDevice, bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		this->vertexBuffer, this->vertexBufferMemory))
		return false;

	CopyBuffer(vkDevice, vkGraphicsQueue, vkCommandPool, stagingBuffer,
		this->vertexBuffer, bufferSize);

	vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
	vkFreeMemory(vkDevice, stagingBufferMemory, nullptr);

	return true;
}
bool Mesh::CreateIndexBuffer(const CorePtr core, const Mesh::Indices &indices)
{
	const auto vkPhysicalDevice = core->GetVulkanPhysicalDevice();
	const auto vkDevice = core->GetVulkanDevice();
	const auto vkGraphicsQueue = core->GetVulkanGraphicsQueue();
	const auto vkCommandPool = core->GetVulkanCommandPool();

	VkDeviceSize bufferSize = sizeof(Mesh::Indices::value_type) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	if (!CreateBuffer(vkPhysicalDevice, vkDevice, bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory))
		return false;

	void *data;
	vkMapMemory(vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (std::size_t)bufferSize);
	vkUnmapMemory(vkDevice, stagingBufferMemory);

	if (!CreateBuffer(vkPhysicalDevice, vkDevice, bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		this->indexBuffer, this->indexBufferMemory))
		return false;

	CopyBuffer(vkDevice, vkGraphicsQueue, vkCommandPool, stagingBuffer,
		this->indexBuffer, bufferSize);

	vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
	vkFreeMemory(vkDevice, stagingBufferMemory, nullptr);

	return true;
}
bool Mesh::CreateBuffer(const VkPhysicalDevice vkPhysicalDevice,
	const VkDevice vkDevice, const VkDeviceSize size,
	const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties,
	VkBuffer &buffer, VkDeviceMemory &bufferMemory)
{
	VkBufferCreateInfo bufferInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr
	};

	if (!CHECK_VK_RESULT(vkCreateBuffer(vkDevice, &bufferInfo, nullptr, &buffer))) {
		std::cerr << "Vulkan: Failed to create buffer" << std::endl;
		return false;
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(vkDevice, buffer, &memRequirements);

	const auto [memoryTypeIndex, errorFlag] = FindMemoryType(vkPhysicalDevice, memRequirements.memoryTypeBits, properties);
	if (errorFlag)
		return false;
	VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = memoryTypeIndex
	};

	if (!CHECK_VK_RESULT(vkAllocateMemory(vkDevice, &allocInfo, nullptr, &bufferMemory))) {
		std::cerr << "Vulkan: Failed to allocate buffer memory" << std::endl;
		return false;
	}

	vkBindBufferMemory(vkDevice, buffer, bufferMemory, 0);

	return true;
}

void Mesh::CopyBuffer(const VkDevice vkDevice, const VkQueue graphicsQueue,
	const VkCommandPool commandPool, const VkBuffer srcBuffer,
	const VkBuffer dstBuffer, const VkDeviceSize size)
{
	VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(vkDevice, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = nullptr
	};

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = size
	};
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = nullptr,
		.pWaitDstStageMask = nullptr,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = nullptr
	};

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(vkDevice, commandPool, 1, &commandBuffer);
}

std::tuple<uint32_t, ErrorFlag> Mesh::FindMemoryType(const VkPhysicalDevice vkPhysicalDevice,
	const uint32_t typeFilter, const VkMemoryPropertyFlags properties)
{

	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return {i, false};
		}
	}

	std::cerr << "Vulkan: Failed to find suitable memory type!" << std::endl;
	return {0, true};
}
