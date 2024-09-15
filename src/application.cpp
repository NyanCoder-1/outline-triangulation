#include "application.hpp"
#include "core.hpp"
#include "pipeline.hpp"
#include "mesh.hpp"
#include <filesystem>
#include <memory>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

namespace fs = std::filesystem;

namespace {
	PipelinePtr pipeline;
	MeshPtr meshSplineSegments;
	PipelinePtr pipelineSpline;
	MeshPtr meshSplineTriangle;
	MeshPtr meshSplineTriangle1;
	MeshPtr meshSplineTriangle2;

	//// Quad Data
	//const Mesh::Vertices vertices = {
	//	{ .position = {-0.5f, -0.5f, 0.0f}, .color = {1.0f, 0.0f, 0.0f, 1.0f}, .uv = {0.0f, 0.0f}},
	//	{ .position = {0.5f, -0.5f, 0.0f}, .color = {0.0f, 1.0f, 0.0f, 1.0f}, .uv = {1.0f, 0.0f}},
	//	{ .position = {0.5f, 0.5f, 0.0f}, .color = {0.0f, 0.0f, 1.0f, 1.0f}, .uv = {1.0f, 1.0f}},
	//	{ .position = {-0.5f, 0.5f, 0.0f}, .color = {1.0f, 1.0f, 1.0f, 1.0f}, .uv = {0.0f, 1.0f}},
	//};

	//const Mesh::Indices indices = {
	//	0, 1, 2, 2, 3, 0
	//};
	// Triangle Data
	const Mesh::Vertices splineVertices = {
		{.position = {-0.5f, 0.5f, 0.0f}, .color = {0.5f, 0.5f, 0.5f, 1.0f}, .uv = {0.0f, 0.0f}},
		{.position = {0.0f, -0.5f, 0.0f}, .color = {0.5f, 0.5f, 0.5f, 1.0f}, .uv = {0.5f, 0.0f}},
		{.position = {0.5f, 0.5f, 0.0f}, .color = {0.5f, 0.5f, 0.5f, 1.0f}, .uv = {1.0f, 1.0f}},
	};
	const Mesh::Indices splineIndices = {
		0, 1, 2
	};
}

bool Application::OnInitialize(const CorePtr core)
{
	pipeline = Pipeline::Create<Mesh::Vertex>(core, fs::path("../assets/shaders/simple-vs.spv"), fs::path("../assets/shaders/simple-fs.spv"));
	if (!pipeline)
		return false;
	pipelineSpline = Pipeline::Create<Mesh::Vertex>(core, fs::path("../assets/shaders/quadratic-spline-vs.spv"), fs::path("../assets/shaders/quadratic-spline-fs.spv"));
	if (!pipelineSpline)
		return false;

	meshSplineTriangle = Mesh::Create(core, splineVertices, splineIndices);
	if (!meshSplineTriangle)
		return false;

	// Split into two beziers
	{
		double t = 0.5;
		auto v1 = (splineVertices[1].position - splineVertices[0].position);
		auto p1 = v1;
		p1 *= t;
		p1 += splineVertices[0].position;
		auto v2 = (splineVertices[2].position - splineVertices[1].position);
		auto p2 = v2;
		p2 *= t;
		p2 += splineVertices[1].position;
		auto v3 = (p2 - p1);
		auto p3 = v3;
		p3 *= t;
		p3 += p1;

		{
			Mesh::Vertices vertices = {
				{.position = splineVertices[0].position, .color = {1.0f, 0.0f, 0.0f, 0.5f}, .uv = {0.0f, 0.0f}},
				{.position = p1, .color = {1.0f, 0.0f, 0.0f, 0.5f}, .uv = {0.5f, 0.0f}},
				{.position = p3, .color = {1.0f, 0.0f, 0.0f, 0.5f}, .uv = {1.0f, 1.0f}},
			};
			meshSplineTriangle1 = Mesh::Create(core, vertices, splineIndices);
			if (!meshSplineTriangle1)
				return false;
		}
		{
			Mesh::Vertices vertices = {
				{.position = p3, .color = {0.0f, 0.0f, 1.0f, 0.5f}, .uv = {0.0f, 0.0f}},
				{.position = p2, .color = {0.0f, 0.0f, 1.0f, 0.5f}, .uv = {0.5f, 0.0f}},
				{.position = splineVertices[2].position, .color = {0.0f, 0.0f, 1.0f, 0.5f}, .uv = {1.0f, 1.0f}},
			};
			meshSplineTriangle2 = Mesh::Create(core, vertices, splineIndices);
			if (!meshSplineTriangle2)
				return false;
		}
	}

	constexpr auto splineSegmentCount = 100;
	constexpr auto splineSegmentsLineVertexCount = (splineSegmentCount + 1) * 2;
	constexpr auto splineSegmentsLineIndexCount = splineSegmentCount * 6;
	constexpr auto splineSegmentsPointVertexCount = (splineSegmentCount + 1) * 4;
	constexpr auto splineSegmentsPointIndexCount = (splineSegmentCount + 1) * 6;
	Mesh::Vertices vertices(splineSegmentsLineVertexCount + splineSegmentsPointVertexCount);
	Mesh::Indices indices(splineSegmentsLineIndexCount + splineSegmentsPointIndexCount);
	constexpr auto lineThickness = 0.000f;
	constexpr auto pointSize = 0.003f;
	for (int i = 0; i <= splineSegmentCount; i++) {
		double t = i / (double)splineSegmentCount;
 		auto v1 = (splineVertices[1].position - splineVertices[0].position);
		auto p1 = v1;
		p1 *= t;
		p1 += splineVertices[0].position;
		auto v2 = (splineVertices[2].position - splineVertices[1].position);
		auto p2 = v2;
		p2 *= t;
		p2 += splineVertices[1].position;
		auto v3 = (p2 - p1);
		auto p3 = v3;
		p3 *= t;
		p3 += p1;
		auto perpendicular = glm::vec3(glm::normalize(glm::rotate(glm::identity<glm::mat4>(), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(v3, 0.0f)));
		perpendicular *= lineThickness;

		// Lines
		vertices[i * 2 + 0] = { .position = p3 + perpendicular, .color = {1.0f, 1.0f, 1.0f, 1.0f}, .uv = {0.0f, 0.0f} };
		vertices[i * 2 + 1] = { .position = p3 - perpendicular, .color = {1.0f, 1.0f, 1.0f, 1.0f}, .uv = {0.0f, 0.0f} };
		// Points
		vertices[splineSegmentsLineVertexCount + i * 4 + 0] = { .position = p3 + glm::vec3{-pointSize, -pointSize, 0.0f}, .color = {1.0f, 0.0f, 0.0f, 1.0f}, .uv = {0.0f, 0.0f} };
		vertices[splineSegmentsLineVertexCount + i * 4 + 1] = { .position = p3 + glm::vec3{pointSize, -pointSize, 0.0f}, .color = {1.0f, 0.0f, 0.0f, 1.0f}, .uv = {0.0f, 0.0f} };
		vertices[splineSegmentsLineVertexCount + i * 4 + 2] = { .position = p3 + glm::vec3{-pointSize, pointSize, 0.0f}, .color = {1.0f, 0.0f, 0.0f, 1.0f}, .uv = {0.0f, 0.0f} };
		vertices[splineSegmentsLineVertexCount + i * 4 + 3] = { .position = p3 + glm::vec3{pointSize, pointSize, 0.0f}, .color = {1.0f, 0.0f, 0.0f, 1.0f}, .uv = {0.0f, 0.0f} };

		if (i < splineSegmentCount) {
			// Lines
			indices[i * 6 + 0] = i * 2 + 0;
			indices[i * 6 + 1] = i * 2 + 1;
			indices[i * 6 + 2] = i * 2 + 2;
			indices[i * 6 + 3] = i * 2 + 2;
			indices[i * 6 + 4] = i * 2 + 1;
			indices[i * 6 + 5] = i * 2 + 3;
		}
		// Points
		indices[splineSegmentsLineIndexCount + i * 6 + 0] = splineSegmentsLineVertexCount + i * 4 + 0;
		indices[splineSegmentsLineIndexCount + i * 6 + 1] = splineSegmentsLineVertexCount + i * 4 + 1;
		indices[splineSegmentsLineIndexCount + i * 6 + 2] = splineSegmentsLineVertexCount + i * 4 + 2;
		indices[splineSegmentsLineIndexCount + i * 6 + 3] = splineSegmentsLineVertexCount + i * 4 + 2;
		indices[splineSegmentsLineIndexCount + i * 6 + 4] = splineSegmentsLineVertexCount + i * 4 + 1;
		indices[splineSegmentsLineIndexCount + i * 6 + 5] = splineSegmentsLineVertexCount + i * 4 + 3;
	}
	meshSplineSegments = Mesh::Create(core, vertices, indices);
	if (!meshSplineSegments)
		return false;

	return true;
}

void Application::OnDestroy(const CorePtr core)
{
	(void)core;
	pipeline = nullptr;
	meshSplineSegments = nullptr;
	pipelineSpline = nullptr;
	meshSplineTriangle = nullptr;
	meshSplineTriangle1 = nullptr;
	meshSplineTriangle2 = nullptr;
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

	pipelineSpline->Bind();
	meshSplineTriangle->Draw();

	pipeline->Bind();
	meshSplineSegments->Draw();

	pipelineSpline->Bind();
	meshSplineTriangle1->Draw();
	meshSplineTriangle2->Draw();

	vkCmdEndRenderPass(vkCommandBuffer);

	return true;
}
