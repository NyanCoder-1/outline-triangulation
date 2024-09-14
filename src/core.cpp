#include "core.hpp"
#include "utils.hpp"
#include <vulkan/vk_enum_string_helper.h>
#ifdef __USE_WAYLAND__
#include <vulkan/vulkan_wayland.h>
#elif defined(__PLATFORM_WINDOWS__)
#include <vulkan/vulkan_win32.h>
#endif
#include <cstring>
#include <iostream>
#include <string>

// constexpr void print_vk_result(std::string name, VkResult result)
// {
// 	if (result != VK_SUCCESS) {
// 		std::cerr << "Vulkan: Failed to run command `" << name << "`: " << string_VkResult(result) << std::endl;
// 	}
// }
// #define CHECK_VK_RESULT(result) print_vk_result(#result, result)

namespace {
	constexpr const char *appName = "outline-triangulation";

#ifdef __USE_WAYLAND__
	// Wayland
	constexpr const char *appTitle = "Outline Triangulation [Wayland/Vulkan]";
	void wlRegistryOnGlobalListener(void *data, wl_registry *registry, uint32_t name, const char* interface, uint32_t version)
	{
		std::cout << "Wayland: " << interface << " version " << version << std::endl;
		if (data) {
			if (auto onGlobal = reinterpret_cast<Core::WlRegistryListenerWrapper*>(data)->onGlobal)
				onGlobal(registry, name, interface, version);
		}
	}
	const wl_registry_listener wlRegistryListener = {
		.global = wlRegistryOnGlobalListener,
		.global_remove = nullptr
	};
	void xdgWmBasePingListener(void *data, xdg_wm_base *shell, uint32_t serial)
	{
		if (data) {
			if (auto onPing = reinterpret_cast<Core::XdgWmBaseListenerWrapper*>(data)->onPing)
				onPing(shell, serial);
		}
	};
	const xdg_wm_base_listener xdgWmBaseListener = {
		.ping = xdgWmBasePingListener
	};
	void xdgSurfaceOnConfigureListener(void *data, xdg_surface *shellSurface, uint32_t serial)
	{
		if (data) {
			if (auto onConfigure = reinterpret_cast<Core::XdgSurfaceListenerWrapper*>(data)->onConfigure)
				onConfigure(shellSurface, serial);
		}
	}
	const xdg_surface_listener xdgSurfaceListener = {
		.configure = xdgSurfaceOnConfigureListener
	};
	void xdgToplevelOnConfigureListener(void *data, xdg_toplevel *toplevel, int32_t width, int32_t height, wl_array *states)
	{
		if (data) {
			if (auto onConfigure = reinterpret_cast<Core::XdgToplevelListenerWrapper*>(data)->onConfigure)
				onConfigure(toplevel, width, height, states);
		}
	}
	void xdgToplevelOnCloseListener(void *data, xdg_toplevel *toplevel)
	{
		if (data) {
			if (auto onClose = reinterpret_cast<Core::XdgToplevelListenerWrapper*>(data)->onClose)
				onClose(toplevel);
		}
	}
	const xdg_toplevel_listener xdgToplevelListener = {
		.configure = xdgToplevelOnConfigureListener,
		.close = xdgToplevelOnCloseListener,
		.configure_bounds = nullptr,
		.wm_capabilities = nullptr
	};
#define VULKAN_PLATFORM_SURFACE_EXTENSION_NAME "VK_KHR_wayland_surface"
#endif // __USE_WAYLAND__

#ifdef __PLATFORM_WINDOWS__
	// WinAPI
	constexpr const char* windowClassName = "outline-triangulation [Core::Window]";
	constexpr const char* appTitle = "Outline Triangulation [WinAPI/Vulkan]";
	LRESULT WndProc(HWND, UINT, WPARAM, LPARAM);
	void RegisterWindowClass()
	{
		WNDCLASSEXA wcex;
		wcex.cbSize = sizeof(WNDCLASSEXA);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = nullptr;
		wcex.hIcon = nullptr;
		wcex.hCursor = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.lpszMenuName = nullptr;
		wcex.lpszClassName = windowClassName;
		wcex.hIconSm = nullptr;
		RegisterClassExA(&wcex);
	}
	LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		// Ultimate message (btw it's the main window, so if it closes - application closes)
		if (message == WM_DESTROY) {
			PostQuitMessage(0);
			return 0;
		}
		auto userData = GetWindowLongPtrA(hWnd, GWLP_USERDATA);
		if (auto wrapper = reinterpret_cast<Core::WndProcListenerWrapper*>(userData)) {
			// First of all try OnClose
			if ((message == WM_CLOSE) && wrapper->OnClose) {
				if (wrapper->OnClose(hWnd))
					DestroyWindow(hWnd);
				return 0;
			}
			// If there's OnProc callback, call it
			if (wrapper->OnProc) {
				const auto& [lResult, handled] = wrapper->OnProc(hWnd, message, wParam, lParam);
				// If handled do not proceed DefWindowProcA and return the lResult
				if (handled)
					return lResult;
			}
		}
		// Default WM_CLOSE action (it goes here only if either userdata is NULL or OnClose isn't set)
		if (message == WM_CLOSE) {
			DestroyWindow(hWnd);
			return 0;
		}
		// If there's no OnProc callback or if message is not handled call Default Proc
		return DefWindowProcA(hWnd, message, wParam, lParam);
	}
#define VULKAN_PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#endif // __PLATFORM_WINDOWS__

	// Vulkan
	constexpr const char* const instanceExtensionNames[] = {
		"VK_EXT_debug_utils",
		"VK_KHR_surface",
		VULKAN_PLATFORM_SURFACE_EXTENSION_NAME
	};
	constexpr const std::size_t instanceExtensionCount = std::size(instanceExtensionNames);
	constexpr const char* const layerNames[] = {
		"VK_LAYER_KHRONOS_validation"
	};
	constexpr const std::size_t layerCount = std::size(layerNames);
	constexpr const char* const deviceExtensionNames[] = {
		"VK_KHR_swapchain"
	};
	constexpr const std::size_t deviceExtensionCount = std::size(deviceExtensionNames);
	VkBool32 DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT type,
		const VkDebugUtilsMessengerCallbackDataEXT* data,
		void* userData)
	{
		(void)userData;
		auto typeName = [](VkDebugUtilsMessageTypeFlagsEXT type) {
			switch (type) {
			case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
				return "general";
			case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
				return "validation";
			case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
				return "performance";
			default:
				return "unknown";
			}
		};
		auto severityName = [](VkDebugUtilsMessageSeverityFlagBitsEXT severity) {
			switch (severity) {
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				return "error";
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				return "warning";
			default:
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
				return "info";
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
				return "verbose";
			}
		};

		std::cout << "Vulkan " << typeName(type) << " (" << severityName(severity) << "): " << data->pMessage << std::endl;

		return VK_FALSE;
	}
}

Core::~Core()
{
	CHECK_VK_RESULT(vkDeviceWaitIdle(this->vkDevice));

	this->DestroyVulkanSwapchain();
	if (this->vkCommandPool) {
		vkDestroyCommandPool(this->vkDevice, this->vkCommandPool, nullptr);
		this->vkCommandPool = nullptr;
	}
	if (this->vkSurface) {
		vkDestroySurfaceKHR(this->vkInstance, this->vkSurface, nullptr);
		this->vkSurface = nullptr;
	}
	if (this->vkDevice) {
		vkDestroyDevice(this->vkDevice, nullptr);
		this->vkDevice = nullptr;
	}
	if (this->vkInstance) {
		// Destroy debug messenger
		auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(this->vkInstance, "vkDestroyDebugUtilsMessengerEXT");
		if (vkDestroyDebugUtilsMessengerEXT) {
			vkDestroyDebugUtilsMessengerEXT(this->vkInstance, this->vkMessenger, nullptr);
		}
		this->vkMessenger = nullptr;
		vkDestroyInstance(this->vkInstance, nullptr);
		this->vkInstance = nullptr;
	}
#ifdef __USE_WAYLAND__
	if (this->xdgToplevel) {
		xdg_toplevel_destroy(this->xdgToplevel);
		this->xdgToplevel = nullptr;
	}
	if (this->xdgSurface) {
		xdg_surface_destroy(this->xdgSurface);
		this->xdgSurface = nullptr;
	}
	if (this->wlSurface) {
		wl_surface_destroy(this->wlSurface);
		this->wlSurface = nullptr;
	}
	if (this->xdgShell) {
		xdg_wm_base_destroy(this->xdgShell);
		this->xdgShell = nullptr;
	}
#endif // __USE_WAYLAND__
#ifdef __PLATFORM_WINDOWS__
#endif // __PLATFORM_WINDOWS__
}

bool Core::Init()
{
#ifdef __USE_WAYLAND__
	if (!this->InitWaylandWindow()) {
		std::cerr << "Wayland: Failed to initialize" << std::endl;
		return false;
	}
#endif // __USE_WAYLAND__
#ifdef __PLATFORM_WINDOWS__
	if (!this->InitWindowsWindow()) {
		std::cerr << "WinAPI: Failed to initialize" << std::endl;
		return false;
	}
#endif // __PLATFORM_WINDOWS__
	if (!this->InitVulkan()) {
		std::cerr << "Vulkan: Failed to initialize" << std::endl;
		return false;
	}

	this->isInitialized = true;
	return true;
}

#ifdef __USE_WAYLAND__
bool Core::InitWaylandWindow()
{
	// Connect to the wl_display
	this->wlDisplay = wl_display_connect(nullptr);
	if (!this->wlDisplay) {
		std::cerr << "Wayland: Failed to connect to wl_display" << std::endl;
		return false;
	}

	// Get the registry
	this->wlRegistry = wl_display_get_registry(this->wlDisplay);
	// Add the listener to get the compositor and shell
	this->wlRegistryListenerWrapper->onGlobal = [this](wl_registry *registry, uint32_t name, const char* interface, uint32_t version) {
		(void)version;
		if (strcmp(interface, wl_compositor_interface.name) == 0) {
			this->wlCompositor = reinterpret_cast<wl_compositor*>(wl_registry_bind(registry, name, &wl_compositor_interface, 1));
		}
		else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
			this->xdgShell = reinterpret_cast<xdg_wm_base*>(wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
		}
	};
	wl_registry_add_listener(this->wlRegistry, &wlRegistryListener, this->wlRegistryListenerWrapper.get());

	wl_display_roundtrip(this->wlDisplay);

	if (!this->wlCompositor) {
		std::cerr << "Wayland: Failed to get compositor" << std::endl;
		return false;
	}
	if (!this->xdgShell) {
		std::cerr << "Wayland: Failed to get xdg_wm_base" << std::endl;
		return false;
	}

	// Add the ping listener to the shell
	this->xdgWmBaseListenerWrapper->onPing = [this](xdg_wm_base *shell, uint32_t serial) {
		xdg_wm_base_pong(shell, serial);
	};
	xdg_wm_base_add_listener(this->xdgShell, &xdgWmBaseListener, this->xdgWmBaseListenerWrapper.get());

	// Create surface
	this->wlSurface = wl_compositor_create_surface(this->wlCompositor);
	if (!this->wlSurface) {
		std::cerr << "Wayland: Failed to create surface" << std::endl;
		return false;
	}

	// Create xdg surface
	this->xdgSurface = xdg_wm_base_get_xdg_surface(this->xdgShell, this->wlSurface);
	if (!this->xdgSurface) {
		std::cerr << "Wayland: Failed to get xdg surface" << std::endl;
		return false;
	}
	// Add listener to xdg surface
	this->xdgSurfaceListenerWrapper->onConfigure = [this](xdg_surface *shellSurface, uint32_t serial) {
		xdg_surface_ack_configure(shellSurface, serial);
		if (this->resize) {
			this->readyToResize = true;
		}
	};
	xdg_surface_add_listener(this->xdgSurface, &xdgSurfaceListener, this->xdgSurfaceListenerWrapper.get());

	// Get xdg toplevel
	this->xdgToplevel = xdg_surface_get_toplevel(this->xdgSurface);
	if (!this->xdgToplevel) {
		std::cerr << "Wayland: Failed to get xdg toplevel" << std::endl;
		return false;
	}
	// Add listener to xdg toplevel
	this->xdgToplevelListenerWrapper->onConfigure = [this](xdg_toplevel *toplevel, int32_t width, int32_t height, wl_array *states) {
		(void)toplevel;
		(void)states;
		if ((width > 0) && (height > 0)) {
			this->newWidth = width;
			this->newHeight = height;
			this->resize = true;
		}
	};
	this->xdgToplevelListenerWrapper->onClose = [this](xdg_toplevel *toplevel) {
		(void)toplevel;
		this->isGoingToClose = true;
	};
	xdg_toplevel_add_listener(this->xdgToplevel, &xdgToplevelListener, this->xdgToplevelListenerWrapper.get());

	// Fill info
	xdg_toplevel_set_title(this->xdgToplevel, appTitle);
	xdg_toplevel_set_app_id(this->xdgToplevel, appName);

	// Commit
	wl_surface_commit(this->wlSurface);
	wl_display_roundtrip(this->wlDisplay);

	if (this->readyToResize && this->resize && this->newWidth && this->newHeight) {
		this->width = this->newWidth;
		this->height = this->newHeight;
	}

	return true;
}
#endif // __USE_WAYLAND__

#ifdef __PLATFORM_WINDOWS__
bool Core::InitWindowsWindow()
{
	RegisterWindowClass();

	DWORD style = WS_OVERLAPPEDWINDOW;
	RECT rc = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
	AdjustWindowRect(&rc, style, FALSE);
	this->hWnd = CreateWindowExA(0L, windowClassName, "win32-vulkan [MainWindow]", style,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, nullptr, nullptr);
	if (!this->hWnd) {
		std::cerr << "Window: Failed to create window" << std::endl;
		return false;
	}
	ShowWindow(this->hWnd, SW_SHOW);

	SetWindowLongPtrA(this->hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this->wndProcListenerWrapper.get()));
	this->wndProcListenerWrapper->OnProc = [this](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)->std::tuple<LRESULT, Core::WndProcListenerWrapper::Handled> {
		Core::WndProcListenerWrapper::Handled handled = true;
		switch (message) {
		case WM_SIZE:
			this->newWidth = LOWORD(lParam);
			this->newHeight = HIWORD(lParam);
			std::cout << "Window: Resized to " << this->newWidth << "x" << this->newHeight << std::endl;
			this->resize = true;
			handled = false;
			break;
		default:
			return { 0, false };
		}
		return { 0, handled };
		};

	return true;
}
#endif // __PLATFORM_WINDOWS__

bool Core::InitVulkan()
{
	if (!this->InitVulkanInstance()) {
		std::cerr << "Vulkan: Failed to create Vulkan instance" << std::endl;
		return false;
	}
	if (!this->InitVulkanMessenger()) {
		std::cerr << "Vulkan: Failed to create Vulkan debug messenger" << std::endl;
		return false;
	}
	if (!this->InitVulkanDevice()) {
		std::cerr << "Vulkan: Failed to create Vulkan device" << std::endl;
		return false;
	}
	if (!this->InitVulkanSurface()) {
		std::cerr << "Vulkan: Failed to get create wayland surface" << std::endl;
		return false;
	}
	this->InitVulkanGraphicsQueue();
	if (!this->InitVulkanCommandPool()) {
		std::cerr << "Vulkan: Failed to create command pool" << std::endl;
		return false;
	}
	if (!this->InitVulkanSwapchain()) {
		std::cerr << "Vulkan: Failed to create swapchain" << std::endl;
		return false;
	}

	return true;
}

bool Core::InitVulkanInstance()
{
	VkApplicationInfo appInfo {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = nullptr,
		.pApplicationName = appName,
		.applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
		.pEngineName = appName,
		.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
		.apiVersion = VK_API_VERSION_1_0
	};
	VkInstanceCreateInfo createInfo {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = instanceExtensionCount,
		.ppEnabledExtensionNames = instanceExtensionNames
	};

	// Check if all required layers are available
	std::size_t foundLayers = 0;
	uint32_t instanceLayersCount = 0;
	CHECK_VK_RESULT(vkEnumerateInstanceLayerProperties(&instanceLayersCount, nullptr));
	std::vector<VkLayerProperties> instanceLayers(instanceLayersCount);
	CHECK_VK_RESULT(vkEnumerateInstanceLayerProperties(&instanceLayersCount, instanceLayers.data()));
	for (auto instanceLayer : instanceLayers) {
		for (const auto &layerName : layerNames) {
			if ((std::string_view)instanceLayer.layerName == layerName) {
				foundLayers++;
				break;
			}
		}
	}
	if (foundLayers >= layerCount) {
		createInfo.enabledLayerCount = layerCount;
		createInfo.ppEnabledLayerNames = layerNames;
	}

	// Create instance
	CHECK_VK_RESULT(vkCreateInstance(&createInfo, nullptr, &this->vkInstance));
	return this->vkInstance != nullptr;
}
bool Core::InitVulkanMessenger()
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.pNext = nullptr,
		.flags = 0,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT,
		.pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT)DebugCallback,
		.pUserData = nullptr
	};
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(this->vkInstance, "vkCreateDebugUtilsMessengerEXT");
	if (!func) {
		std::cerr << "Vulkan: Failed to get create debug messenger function" << std::endl;
		return false;
	}
	CHECK_VK_RESULT(func(this->vkInstance, &createInfo, nullptr, &this->vkMessenger));
	return this->vkMessenger != nullptr;
}
bool Core::InitVulkanDevice()
{
	uint32_t physicalDevicesCount = 0;
	CHECK_VK_RESULT(vkEnumeratePhysicalDevices(this->vkInstance, &physicalDevicesCount, nullptr));
	std::vector<VkPhysicalDevice> physicalDevices(physicalDevicesCount);
	CHECK_VK_RESULT(vkEnumeratePhysicalDevices(this->vkInstance, &physicalDevicesCount, physicalDevices.data()));
	if (physicalDevices.empty()) {
		std::cerr << "Vulkan: Failed to get physical devices" << std::endl;
		return false;
	}

	// Select physical device
	{
		uint32_t bestScore = 0;
		for (const auto &currentPhysicalDevice : physicalDevices) {
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(currentPhysicalDevice, &properties);

			uint32_t score;

			switch (properties.deviceType)
			{
			default:
				continue;
			case VK_PHYSICAL_DEVICE_TYPE_OTHER: score = 1; break;
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: score = 4; break;
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: score = 5; break;
			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: score = 3; break;
			case VK_PHYSICAL_DEVICE_TYPE_CPU: score = 2; break;
			}

			if (score > bestScore)
			{
				this->vkPhysicalDevice = currentPhysicalDevice;
				bestScore = score;
			}
		}

		if (!bestScore) {
			std::cerr << "Vulkan: Failed to select physical device" << std::endl;
			return false;
		}
	}
	

	// Create logical device
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(this->vkPhysicalDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(this->vkPhysicalDevice, &queueFamilyCount, queueFamilies.data());

	uint32_t i = 0;
	for (const auto &currentQueueFamily : queueFamilies)
	{
#ifdef __USE_WAYLAND__
		VkBool32 present = vkGetPhysicalDeviceWaylandPresentationSupportKHR(this->vkPhysicalDevice, i, this->wlDisplay);
#endif // __USE_WAYLAND__
#ifdef __PLATFORM_WINDOWS__
		VkBool32 present = vkGetPhysicalDeviceWin32PresentationSupportKHR(this->vkPhysicalDevice, i);
#endif // __PLATFORM_WINDOWS__
		if (present && (currentQueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT))
		{
			this->vkQueueFamilyIndex = i;
			break;
		}
		i++;
	}
	
	float priority = 1;
	VkDeviceQueueCreateInfo queueCreateInfo {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueFamilyIndex = this->vkQueueFamilyIndex,
		.queueCount = 1,
		.pQueuePriorities = &priority
	};
	VkDeviceCreateInfo createInfo {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queueCreateInfo,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = deviceExtensionCount,
		.ppEnabledExtensionNames = deviceExtensionNames,
		.pEnabledFeatures = nullptr
	};
	uint32_t layerPropertyCount = 0;
	CHECK_VK_RESULT(vkEnumerateDeviceLayerProperties(this->vkPhysicalDevice, &layerPropertyCount, nullptr));
	std::vector<VkLayerProperties> layerProperties(layerPropertyCount);
	CHECK_VK_RESULT(vkEnumerateDeviceLayerProperties(this->vkPhysicalDevice, &layerPropertyCount, layerProperties.data()));

	std::size_t foundLayers = 0;
	for (const auto &currentLayerProperty : layerProperties)
	{
		for (const auto &layerName : layerNames)
		{
			if ((std::string_view)currentLayerProperty.layerName == layerName)
			{
				foundLayers++;
			}
		}
	}

	if (foundLayers >= layerCount)
	{
		createInfo.enabledLayerCount = layerCount;
		createInfo.ppEnabledLayerNames = layerNames;
	}

	CHECK_VK_RESULT(vkCreateDevice(this->vkPhysicalDevice, &createInfo, nullptr, &this->vkDevice));

	return this->vkDevice != nullptr;
}
bool Core::InitVulkanSurface()
{
#ifdef __USE_WAYLAND__
	VkWaylandSurfaceCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
		.pNext = nullptr,
		.flags = 0,
		.display = this->wlDisplay,
		.surface = this->wlSurface
	};
	CHECK_VK_RESULT(vkCreateWaylandSurfaceKHR(this->vkInstance, &createInfo, nullptr, &this->vkSurface));
#endif // __USE_WAYLAND__
#ifdef __PLATFORM_WINDOWS__
	VkWin32SurfaceCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.pNext = nullptr,
		.flags = 0,
		.hinstance = nullptr,
		.hwnd = this->hWnd
	};
	CHECK_VK_RESULT(vkCreateWin32SurfaceKHR(this->vkInstance, &createInfo, nullptr, &this->vkSurface));
#endif // __PLATFORM_WINDOWS__

	return this->vkSurface != nullptr;
}
void Core::InitVulkanGraphicsQueue()
{
	vkGetDeviceQueue(this->vkDevice, this->vkQueueFamilyIndex, 0, &this->vkGraphicsQueue);
}
bool Core::InitVulkanCommandPool()
{
	VkCommandPoolCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = this->vkQueueFamilyIndex
	};
	CHECK_VK_RESULT(vkCreateCommandPool(this->vkDevice, &createInfo, nullptr, &this->vkCommandPool));

	return this->vkCommandPool != nullptr;
}
bool Core::InitVulkanSwapchain()
{
	vkSwapchainFormat = VK_FORMAT_UNDEFINED;
	int32_t width = 1920;
	int32_t height = 1080;

	{
		VkSurfaceCapabilitiesKHR capabilities;
		CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->vkPhysicalDevice, this->vkSurface, &capabilities));
		uint32_t formatsCount;
		CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(this->vkPhysicalDevice, this->vkSurface, &formatsCount, nullptr));
		std::vector<VkSurfaceFormatKHR> formats(formatsCount);
		CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(this->vkPhysicalDevice, this->vkSurface, &formatsCount, formats.data()));
		auto chosenFormat = formats[0];
		for (auto &currentFormat : formats) {
			if (currentFormat.format == VK_FORMAT_B8G8R8A8_UNORM) {
				chosenFormat = currentFormat;
				break;
			}
		}

		vkSwapchainFormat = chosenFormat.format;

		this->vkFramesCount = (capabilities.minImageCount + 1) < capabilities.maxImageCount ? capabilities.minImageCount + 1 : capabilities.maxImageCount;

		width = ~capabilities.currentExtent.width ? capabilities.currentExtent.width : capabilities.maxImageExtent.width;
		height = ~capabilities.currentExtent.height ? capabilities.currentExtent.height : capabilities.maxImageExtent.height;
		if (this->width && this->height) {
			width = this->width;
			height = this->height;
		}
		else {
			this->width = width;
			this->height = height;
		}

		VkSwapchainCreateInfoKHR createInfo = {
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.pNext = nullptr,
			.flags = 0,
			.surface = this->vkSurface,
			.minImageCount = this->vkFramesCount,
			.imageFormat = chosenFormat.format,
			.imageColorSpace = chosenFormat.colorSpace,
			.imageExtent = VkExtent2D{ .width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height) },
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr,
			.preTransform = capabilities.currentTransform,
#ifdef __USE_WAYLAND__
			.compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
#elif defined(__PLATFORM_WINDOWS__)
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
#endif
			.presentMode = VK_PRESENT_MODE_MAILBOX_KHR,
			.clipped = VK_TRUE,
			.oldSwapchain = VK_NULL_HANDLE
		};
		CHECK_VK_RESULT(vkCreateSwapchainKHR(this->vkDevice, &createInfo, nullptr, &this->vkSwapchain));
	}

	{
		VkAttachmentDescription attachments = {
			.flags = 0,
			.format = vkSwapchainFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		};
		VkAttachmentReference attachmentReference = {
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};
		VkSubpassDescription subpass = {
			.flags = 0,
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.inputAttachmentCount = 0,
			.pInputAttachments = nullptr,
			.colorAttachmentCount = 1,
			.pColorAttachments = &attachmentReference,
			.pResolveAttachments = nullptr,
			.pDepthStencilAttachment = nullptr,
			.preserveAttachmentCount = 0,
			.pPreserveAttachments = nullptr
		};
		VkRenderPassCreateInfo createInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.attachmentCount = 1,
			.pAttachments = &attachments,
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = 0,
			.pDependencies = nullptr
		};
		CHECK_VK_RESULT(vkCreateRenderPass(this->vkDevice, &createInfo, nullptr, &this->vkRenderPass));
	}

	CHECK_VK_RESULT(vkGetSwapchainImagesKHR(this->vkDevice, this->vkSwapchain, &this->vkFramesCount, nullptr));
	std::vector<VkImage> images(this->vkFramesCount);
	CHECK_VK_RESULT(vkGetSwapchainImagesKHR(this->vkDevice, this->vkSwapchain, &this->vkFramesCount, images.data()));
	this->vkSwapchainResources.resize(this->vkFramesCount);

	VkCommandBufferAllocateInfo cbAllocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = this->vkCommandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = this->vkFramesCount
	};
	std::vector<VkCommandBuffer> commandBuffers(this->vkFramesCount);
	CHECK_VK_RESULT(vkAllocateCommandBuffers(this->vkDevice, &cbAllocInfo, commandBuffers.data()));

	for (std::size_t i = 0; i < images.size(); i++) {
		auto &currentSwapchainResource = this->vkSwapchainResources[i];

		currentSwapchainResource.commandBuffer = commandBuffers[i];
		currentSwapchainResource.image = images[i];

		VkImageViewCreateInfo ivCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.image = currentSwapchainResource.image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = vkSwapchainFormat,
			.components = VkComponentMapping{
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY
			},
			.subresourceRange = VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};
		CHECK_VK_RESULT(vkCreateImageView(this->vkDevice, &ivCreateInfo, nullptr, &currentSwapchainResource.imageView));

		VkFramebufferCreateInfo fbCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.renderPass = this->vkRenderPass,
			.attachmentCount = 1,
			.pAttachments = &currentSwapchainResource.imageView,
			.width = static_cast<uint32_t>(width),
			.height = static_cast<uint32_t>(height),
			.layers = 1
		};
		CHECK_VK_RESULT(vkCreateFramebuffer(this->vkDevice, &fbCreateInfo, nullptr, &currentSwapchainResource.framebuffer));

		VkSemaphoreCreateInfo beginSemaphoreCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0
		};
		CHECK_VK_RESULT(vkCreateSemaphore(this->vkDevice, &beginSemaphoreCreateInfo, nullptr, &currentSwapchainResource.startSemaphore));
		VkSemaphoreCreateInfo endSemaphoreCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0
		};
		CHECK_VK_RESULT(vkCreateSemaphore(this->vkDevice, &endSemaphoreCreateInfo, nullptr, &currentSwapchainResource.endSemaphore));

		VkFenceCreateInfo fenceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT
		};
		CHECK_VK_RESULT(vkCreateFence(this->vkDevice, &fenceCreateInfo, nullptr, &currentSwapchainResource.fence));

		currentSwapchainResource.lastFence = VK_NULL_HANDLE;
	}

	return true;
}
void Core::DestroyVulkanSwapchain()
{
	for (auto &swapchainResource : this->vkSwapchainResources) {
		if (swapchainResource.fence) {
			vkDestroyFence(this->vkDevice, swapchainResource.fence, nullptr);
			swapchainResource.fence = nullptr;
		}
		if (swapchainResource.startSemaphore) {
			vkDestroySemaphore(this->vkDevice, swapchainResource.startSemaphore, nullptr);
			swapchainResource.startSemaphore = nullptr;
		}
		if (swapchainResource.endSemaphore) {
			vkDestroySemaphore(this->vkDevice, swapchainResource.endSemaphore, nullptr);
			swapchainResource.endSemaphore = nullptr;
		}
		if (swapchainResource.framebuffer) {
			vkDestroyFramebuffer(this->vkDevice, swapchainResource.framebuffer, nullptr);
			swapchainResource.framebuffer = nullptr;
		}
		if (swapchainResource.imageView) {
			vkDestroyImageView(this->vkDevice, swapchainResource.imageView, nullptr);
			swapchainResource.imageView = nullptr;
		}
		if (swapchainResource.commandBuffer) {
			vkFreeCommandBuffers(this->vkDevice, this->vkCommandPool, 1, &swapchainResource.commandBuffer);
			swapchainResource.commandBuffer = nullptr;
		}
	}
	this->vkSwapchainResources.clear();
	if (this->vkRenderPass) {
		vkDestroyRenderPass(this->vkDevice, this->vkRenderPass, nullptr);
		this->vkRenderPass = nullptr;
	}
	if (this->vkSwapchain) {
		vkDestroySwapchainKHR(this->vkDevice, this->vkSwapchain, nullptr);
		this->vkSwapchain = nullptr;
	}
}

bool Core::Render()
{
	// Wait for previous frame
	auto &currentSwapchainResource = this->vkSwapchainResources[this->vkCurrentFrame];

	CHECK_VK_RESULT(vkWaitForFences(this->vkDevice, 1, &currentSwapchainResource.fence, VK_TRUE, std::numeric_limits<uint64_t>::max()));
	VkResult result = vkAcquireNextImageKHR(this->vkDevice, this->vkSwapchain, std::numeric_limits<uint64_t>::max(), currentSwapchainResource.startSemaphore, VK_NULL_HANDLE, &this->vkNextFrame);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		CHECK_VK_RESULT(vkDeviceWaitIdle(this->vkDevice));
		return OnResize();
	}
	else if (result < VK_SUCCESS) {
		CHECK_VK_RESULT(result);
	}

	auto &nextSwapchainResource = this->vkSwapchainResources[this->vkNextFrame];
	if (nextSwapchainResource.lastFence) {
		CHECK_VK_RESULT(vkWaitForFences(this->vkDevice, 1, &nextSwapchainResource.lastFence, VK_TRUE, std::numeric_limits<uint64_t>::max()));
	}
	nextSwapchainResource.lastFence = currentSwapchainResource.fence;

	CHECK_VK_RESULT(vkResetFences(this->vkDevice, 1, &currentSwapchainResource.fence));
	VkCommandBufferBeginInfo beginInfo {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = nullptr
	};
	CHECK_VK_RESULT(vkBeginCommandBuffer(nextSwapchainResource.commandBuffer, &beginInfo));

	// Prepare the current frame
	if (onFrameCallback && !onFrameCallback(this->shared_from_this()))
		return false;

	// Present the current frame
	CHECK_VK_RESULT(vkEndCommandBuffer(nextSwapchainResource.commandBuffer));
	const VkPipelineStageFlags waitStageFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &currentSwapchainResource.startSemaphore,
		.pWaitDstStageMask = &waitStageFlag,
		.commandBufferCount = 1,
		.pCommandBuffers = &nextSwapchainResource.commandBuffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &currentSwapchainResource.endSemaphore
	};
	CHECK_VK_RESULT(vkQueueSubmit(this->vkGraphicsQueue, 1, &submitInfo, currentSwapchainResource.fence));
	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &currentSwapchainResource.endSemaphore,
		.swapchainCount = 1,
		.pSwapchains = &this->vkSwapchain,
		.pImageIndices = &this->vkNextFrame,
		.pResults = nullptr
	};
	result = vkQueuePresentKHR(this->vkGraphicsQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		if (!OnResize())
			return false;
	}
	else if (result < VK_SUCCESS) {
		CHECK_VK_RESULT(result);
	}

	this->vkCurrentFrame = (this->vkCurrentFrame + 1) % this->vkFramesCount;

	return true;
}

bool Core::OnResize()
{
	CHECK_VK_RESULT(vkDeviceWaitIdle(this->vkDevice));

	this->DestroyVulkanSwapchain();
	if (!this->InitVulkanSwapchain())
		return false;

	this->vkCurrentFrame = 0;

	return true;
}

void Core::OnDestroy()
{
	CHECK_VK_RESULT(vkDeviceWaitIdle(this->vkDevice));
	if (this->onDestroyCallback)
		this->onDestroyCallback(this->shared_from_this());
}

void Core::Run()
{
	auto defer = MyDefer([this]() { this->OnDestroy(); });
	if (!this->isInitialized)
		return;
	if (this->onInitCallback && !this->onInitCallback(this->shared_from_this())) {
		std::cerr << "Failed to initialize application" << std::endl;
		return;
	}
#ifdef __USE_WAYLAND__
	while (!this->isGoingToClose) {
		if (this->readyToResize && this->resize) {
			this->width = this->newWidth;
			this->height = this->newHeight;

			if (!this->OnResize()) {
				std::cerr << "Failed to resize renderer" << std::endl;
				break;
			}

			this->readyToResize = false;
			this->resize = false;

			wl_surface_commit(this->wlSurface);
		}

		if (!this->Render())
			break;
		
		wl_display_dispatch(this->wlDisplay);
	}
#endif // __USE_WAYLAND__
#ifdef __PLATFORM_WINDOWS__
	MSG msg = { 0 };
	while (WM_QUIT != msg.message) {
		if (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}
		else {
			if (this->resize) {
				this->width = this->newWidth;
				this->height = this->newHeight;

				if (!this->OnResize()) {
					DestroyWindow(this->hWnd);
					return;
				}

				this->resize = false;
			}

			if (!this->Render()) {
				DestroyWindow(this->hWnd);
				return;
			}
		}
	}
#endif // __PLATFORM_WINDOWS__

	CHECK_VK_RESULT(vkDeviceWaitIdle(this->vkDevice));

}