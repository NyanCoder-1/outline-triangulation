#pragma once

#include "my_types.hpp"
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <vector>

class Mesh {
	struct Private { explicit Private() = default; };
public:
	struct Vertex {
		glm::vec3 position;
		glm::vec4 color;

		static VkVertexInputBindingDescription GetBindingDescription() {
			VkVertexInputBindingDescription bindingDescription = {
				.binding = 0,
				.stride = sizeof(Vertex),
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
			};
			return bindingDescription;
		}
		static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions() {
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {
				VkVertexInputAttributeDescription{
					.location = 0,
					.binding = 0,
					.format = VK_FORMAT_R32G32_SFLOAT,
					.offset = offsetof(Vertex, position)
				},
				VkVertexInputAttributeDescription{
					.location = 1,
					.binding = 0,
					.format = VK_FORMAT_R32G32B32_SFLOAT,
					.offset = offsetof(Vertex, color)
				}
			};
			return attributeDescriptions;
		}
	};
	typedef std::vector<Vertex> Vertices;
	typedef std::vector<uint16_t> Indices;

	Mesh() = delete;
	Mesh(const Mesh &) = delete;
	Mesh(Mesh &&) = delete;
	Mesh(Private) {}
	~Mesh();

	static MeshPtr Create(const CorePtr core, const Mesh::Vertices &vertices, const Mesh::Indices &indices)
	{
		auto ptr = std::make_shared<Mesh>(Private());
		if (!ptr->Init(core, vertices, indices))
			return nullptr;
		return ptr;
	}

	void Bind();

private:
	bool Init(const CorePtr core, const Mesh::Vertices &vertices, const Mesh::Indices &indices);
	bool CreateVertexBuffer(const CorePtr core, const Mesh::Vertices &vertices);
	bool CreateIndexBuffer(const CorePtr core, const Mesh::Indices &indices);
	static bool CreateBuffer(const VkPhysicalDevice vkPhysicalDevice, const VkDevice vkDevice, const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);
	static void CopyBuffer(const VkDevice vkDevice, const VkQueue graphicsQueue, const VkCommandPool commandPool, const VkBuffer srcBuffer, const VkBuffer dstBuffer, const VkDeviceSize size);
	static std::tuple<uint32_t, ErrorFlag> FindMemoryType(const VkPhysicalDevice vkPhysicalDevice, const uint32_t typeFilter, const VkMemoryPropertyFlags properties);

	CoreWeakPtr coreWeak;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
};
