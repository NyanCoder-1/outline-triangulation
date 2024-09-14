#include "application.hpp"
#include "core.hpp"
#include "pipeline.hpp"
#include "mesh.hpp"
#include <filesystem>
#include <memory>
#include <vector>

namespace fs = std::filesystem;

namespace {
	PipelinePtr pipeline;
	MeshPtr meshTriangle;

	//// Quad Data
	//const Mesh::Vertices vertices = {
	//	{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
	//	{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
	//	{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
	//	{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}
	//};

	//const Mesh::Indices indices = {
	//	0, 1, 2, 2, 3, 0
	//};
	// Triangle Data
	const Mesh::Vertices vertices = {
		{{0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
		{{0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
		{{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}
	};
	const Mesh::Indices indices = {
		0, 1, 2
	};
}

bool Application::OnInitialize(const CorePtr core)
{
	pipeline = Pipeline::Create<Mesh::Vertex>(core, fs::path("../assets/shaders/simple-vs.spv"), fs::path("../assets/shaders/simple-fs.spv"));
	if (!pipeline)
		return false;

	meshTriangle = Mesh::Create(core, vertices, indices);
	if (!meshTriangle)
		return false;

	return true;
}

void Application::OnDestroy(const CorePtr core)
{
	(void)core;
	meshTriangle = nullptr;
	pipeline = nullptr;
}

bool Application::OnFrame(const CorePtr core)
{
	const auto vkCommandBuffer = core->GetVulkanCurrentFrameCommandBuffer();

	VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 0.5f };
	VkRenderPassBeginInfo renderPassBeginInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,
		.renderPass = core->GetVulkanRenderPass(),
		.framebuffer = core->GetVulkanCurrentFrameFramebuffer(),
		.renderArea = {
			.offset = VkOffset2D{ .x = 0, .y = 0 },
			.extent = VkExtent2D{ .width = core->GetWidth(), .height = core->GetHeight() }
		},
		.clearValueCount = 1,
		.pClearValues = &clearColor
	};

	vkCmdBeginRenderPass(vkCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	pipeline->Bind();

	meshTriangle->Bind();

	vkCmdDrawIndexed(vkCommandBuffer, indices.size(), 1, 0, 0, 0);

	vkCmdEndRenderPass(vkCommandBuffer);

	return true;
}
