#pragma once

#include "my_types.hpp"
#ifdef __USE_WAYLAND__
#include "xdg-shell.h"
#include <wayland-client.h>
#endif // __USE_WAYLAND__
#ifdef __PLATFORM_WINDOWS__
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif // __PLATFORM_WINDOWS__
#include <vulkan/vulkan.h>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

class Core : public std::enable_shared_from_this<Core> {
	struct Private { explicit Private() = default; };
public:
#ifdef __USE_WAYLAND__
	struct WlRegistryListenerWrapper {
		std::function<void(wl_registry *registry, uint32_t name, const char* interface, uint32_t version)> onGlobal;
		~WlRegistryListenerWrapper() {
			onGlobal = nullptr;
		}
	};
	struct XdgWmBaseListenerWrapper {
		std::function<void(xdg_wm_base *shell, uint32_t serial)> onPing;
		~XdgWmBaseListenerWrapper() {
			onPing = nullptr;
		}
	};
	struct XdgSurfaceListenerWrapper {
		std::function<void(xdg_surface *shellSurface, uint32_t serial)> onConfigure;
		~XdgSurfaceListenerWrapper() {
			onConfigure = nullptr;
		}
	};
	struct XdgToplevelListenerWrapper {
		std::function<void(xdg_toplevel *toplevel, int32_t width, int32_t height, wl_array *states)> onConfigure;
		std::function<void(xdg_toplevel *toplevel)> onClose;
		~XdgToplevelListenerWrapper() {
			onConfigure = nullptr;
			onClose = nullptr;
		}
	};
	struct XdgPopupListenerWrapper {
		std::function<void(xdg_popup *popup, int32_t x, int32_t y, int32_t width, int32_t height)> onConfigure;
		~XdgPopupListenerWrapper() {
			onConfigure = nullptr;
		}
	};
#elif __PLATFORM_WINDOWS__
	struct WndProcListenerWrapper {
		typedef bool Handled;
		std::function<std::tuple<LRESULT, Core::WndProcListenerWrapper::Handled>(HWND, UINT, WPARAM, LPARAM)> OnProc;
		std::function<bool(HWND)> OnClose;
		~WndProcListenerWrapper()
		{
			OnProc = nullptr;
			OnClose = nullptr;
		}
	};
#endif
	struct SwapchainResources {
		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
		VkImage image = VK_NULL_HANDLE;
		VkImageView imageView = VK_NULL_HANDLE;
		VkFramebuffer framebuffer = VK_NULL_HANDLE;
		VkSemaphore startSemaphore = VK_NULL_HANDLE;
		VkSemaphore endSemaphore = VK_NULL_HANDLE;
		VkFence fence = VK_NULL_HANDLE;
		VkFence lastFence = VK_NULL_HANDLE;
	};

public:
	Core() = delete;
	Core(const Core &) = delete;
	Core(Core &&) = delete;
	Core& operator=(const Core&) = delete;
	Core& operator=(Core&&) = delete;
	Core(Private) {}
	~Core();

	static CorePtr Create() {
		auto ptr = std::make_unique<Core>(Private());
		if (!ptr->Init())
			return nullptr;
		return ptr;
	}

	void Run();

	void SetOnInitCallback(OnInitType callback) { onInitCallback = callback; }
	void SetOnDestroyCallback(OnDestroyType callback) { onDestroyCallback = callback; }
	void SetOnResizeCallback(OnResizeType callback) { onResizeCallback = callback; }
	void SetOnFrameCallback(OnFrameType callback) { onFrameCallback = callback; }

	VkInstance GetVulkanInstance() const { return vkInstance; }
	VkPhysicalDevice GetVulkanPhysicalDevice() const { return vkPhysicalDevice; }
	VkDevice GetVulkanDevice() const { return vkDevice; }
	uint32_t GetVulkanQueueFamilyIndex() const { return vkQueueFamilyIndex; }
	VkQueue GetVulkanGraphicsQueue() const { return vkGraphicsQueue; }
	VkSurfaceKHR GetVulkanSurface() const { return vkSurface; }
	VkCommandPool GetVulkanCommandPool() const { return vkCommandPool; }
	VkSwapchainKHR GetVulkanSwapchain() const { return vkSwapchain; }
	VkRenderPass GetVulkanRenderPass() const { return vkRenderPass; }
	std::vector<SwapchainResources>& GetVulkanSwapchainResources() { return vkSwapchainResources; }
	VkCommandBuffer GetVulkanCurrentFrameCommandBuffer() const { return vkSwapchainResources[vkNextFrame].commandBuffer; }
	VkImage GetVulkanCurrentFrameImage() const { return vkSwapchainResources[vkNextFrame].image; }
	VkImageView GetVulkanCurrentFrameImageView() const { return vkSwapchainResources[vkNextFrame].imageView; }
	VkFramebuffer GetVulkanCurrentFrameFramebuffer() const { return vkSwapchainResources[vkNextFrame].framebuffer; }
	VkSemaphore GetVulkanCurrentFrameStartSemaphore() const { return vkSwapchainResources[vkNextFrame].startSemaphore; }
	VkSemaphore GetVulkanCurrentFrameEndSemaphore() const { return vkSwapchainResources[vkNextFrame].endSemaphore; }
	VkFence GetVulkanCurrentFrameFence() const { return vkSwapchainResources[vkNextFrame].fence; }
	VkFence GetVulkanCurrentFrameLastFence() const { return vkSwapchainResources[vkNextFrame].lastFence; }
	uint32_t GetWidth() const { return width; }
	uint32_t GetHeight() const { return height; }

private:
	bool Init();
	bool Render();
	bool OnResize();
	void OnDestroy();

#ifdef __USE_WAYLAND__
	// Wayland
	bool InitWaylandWindow();

	wl_display *wlDisplay = nullptr;
	wl_registry *wlRegistry = nullptr;
	wl_compositor *wlCompositor = nullptr;
	xdg_wm_base *xdgShell = nullptr;
	wl_surface *wlSurface = nullptr;
	xdg_surface *xdgSurface = nullptr;
	xdg_toplevel *xdgToplevel = nullptr;
	std::unique_ptr<Core::WlRegistryListenerWrapper> wlRegistryListenerWrapper = std::make_unique<Core::WlRegistryListenerWrapper>();
	std::unique_ptr<Core::XdgWmBaseListenerWrapper> xdgWmBaseListenerWrapper = std::make_unique<Core::XdgWmBaseListenerWrapper>();
	std::unique_ptr<Core::XdgSurfaceListenerWrapper> xdgSurfaceListenerWrapper = std::make_unique<Core::XdgSurfaceListenerWrapper>();
	std::unique_ptr<Core::XdgToplevelListenerWrapper> xdgToplevelListenerWrapper = std::make_unique<Core::XdgToplevelListenerWrapper>();
#endif

#ifdef __PLATFORM_WINDOWS__
	// Windows
	bool InitWindowsWindow();

	HWND hWnd = nullptr;
	std::unique_ptr<WndProcListenerWrapper> wndProcListenerWrapper = std::make_unique<WndProcListenerWrapper>();
#endif

	// Vulkan
	bool InitVulkan();
	bool InitVulkanInstance();
	bool InitVulkanMessenger();
	bool InitVulkanDevice();
	void InitVulkanGraphicsQueue();
	bool InitVulkanSurface();
	bool InitVulkanCommandPool();
	bool InitVulkanSwapchain();
	void DestroyVulkanSwapchain();

	VkInstance vkInstance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT vkMessenger = VK_NULL_HANDLE;
	VkPhysicalDevice vkPhysicalDevice = VK_NULL_HANDLE;
	VkDevice vkDevice = VK_NULL_HANDLE;
	VkQueue vkGraphicsQueue = VK_NULL_HANDLE;
	VkSurfaceKHR vkSurface = VK_NULL_HANDLE;
	VkCommandPool vkCommandPool = VK_NULL_HANDLE;
	VkSwapchainKHR vkSwapchain = VK_NULL_HANDLE;
	VkRenderPass vkRenderPass = VK_NULL_HANDLE;
	std::vector<Core::SwapchainResources> vkSwapchainResources;
	uint32_t vkFramesCount = 0;
	uint32_t vkCurrentFrame = 0;
	uint32_t vkNextFrame = 0;
	uint32_t vkQueueFamilyIndex = 0;
	VkFormat vkSwapchainFormat = VK_FORMAT_UNDEFINED;

	// Common
	uint32_t width = 1280;
	uint32_t height = 720;
	uint32_t newWidth = 0;
	uint32_t newHeight = 0;
	OnInitType onInitCallback;
	OnDestroyType onDestroyCallback;
	OnResizeType onResizeCallback;
	OnFrameType onFrameCallback;

	// bitfield
	bool isInitialized : 1 = false;
	bool resize : 1 = false;
	bool readyToResize : 1 = false;
	bool isGoingToClose : 1 = false;
};
