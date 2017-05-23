#define VK_USE_PLATFORM_WIN32_KHR 1

#include <Windows.h>
#include <vector>
#include <vulkan/vulkan.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

typedef HGLRC __stdcall wgl_create_context_attribs_arb(HDC hDC,
	HGLRC hShareContext, const int *attribList);

LRESULT CALLBACK WndProc(HWND WindowPtr, UINT msg, WPARAM wParam,
	LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int CommandShow)
{
	WNDCLASSEX WindowsClassStructure;
	WindowsClassStructure.cbSize = sizeof(WNDCLASSEX);
	WindowsClassStructure.style = CS_OWNDC;
	WindowsClassStructure.lpfnWndProc = WndProc;
	WindowsClassStructure.cbClsExtra = 0;
	WindowsClassStructure.cbWndExtra = 0;
	WindowsClassStructure.hInstance = hInstance;
	WindowsClassStructure.hIcon = LoadIcon(0, IDI_APPLICATION);
	WindowsClassStructure.hCursor = LoadCursor(0, IDC_ARROW);
	WindowsClassStructure.hbrBackground = (HBRUSH)(COLOR_WINDOW + 3);
	WindowsClassStructure.lpszMenuName = 0;
	WindowsClassStructure.lpszClassName =
		(const char*)"VulkanPlayground";
	WindowsClassStructure.hIconSm = LoadIcon(0, IDI_APPLICATION);

	if (!RegisterClassEx(&WindowsClassStructure))
	{
		MessageBox(0, "Window Registration Failed!", "Error!",
			MB_ICONEXCLAMATION | MB_OK);
	}

	HWND WindowPtr = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		(const char*)"VulkanPlayground",
		"Vulkan Playground",
		WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT,
		0, 0, hInstance, 0);

	if (!WindowPtr)
	{
		MessageBox(0, "Window Creation Failed!", "Error!",
			MB_ICONEXCLAMATION | MB_OK);
	}

	UpdateWindow(WindowPtr);
	ShowWindow(WindowPtr, CommandShow);

	//HDC WindowDeviceContext = GetDC(WindowPtr);

	// Pre-Checks of Vulkan Instance
	VkInstance VulkanInst;

	VkApplicationInfo ApplicationInfo = {
		VK_STRUCTURE_TYPE_APPLICATION_INFO,
		0, "Vulkan Playground", VK_MAKE_VERSION(1, 0, 0),
		"Vulkan Practice", VK_MAKE_VERSION(1, 0, 0),
		VK_API_VERSION_1_0
	};

	uint32_t InstanceExtensionsCount = 0;
	if ((vkEnumerateInstanceExtensionProperties(0, &InstanceExtensionsCount, 0) != VK_SUCCESS) ||
		(InstanceExtensionsCount == 0))
	{
		MessageBox(WindowPtr, "Error occured during instance extensions enumeration.", 0, 0);
	}

	std::vector<VkExtensionProperties> AvailableInstanceExtensions(InstanceExtensionsCount);
	if ((vkEnumerateInstanceExtensionProperties(0, &InstanceExtensionsCount, &AvailableInstanceExtensions[0]) != VK_SUCCESS) ||
		(InstanceExtensionsCount == 0))
	{
		MessageBox(WindowPtr, "Error occured during instance extensions enumeration.", 0, 0);
	}
	
	bool Error = true;
	for (size_t i = 0; i < AvailableInstanceExtensions.size(); ++i)
	{
		if (strcmp(AvailableInstanceExtensions[i].extensionName, VK_KHR_SURFACE_EXTENSION_NAME) == 0)
		{
			Error = false;
		}
	}
	if (Error)
	{
		MessageBox(WindowPtr, "Couldn't find VK_KHR_SURFACE_EXTENSION_NAME", 0, 0);
	}

	std::vector<const char*> Extensions = {
		VK_KHR_SURFACE_EXTENSION_NAME
#if defined(VK_USE_PLATFORM_WIN32_KHR)
		,VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#endif
	};

	VkInstanceCreateInfo InstanceCreateInfo = {
		VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		0, 0, &ApplicationInfo, 0, 0, static_cast<uint32_t>(Extensions.size()), &Extensions[0]
	};

	// Create Vulkan Instance
	if (vkCreateInstance(&InstanceCreateInfo, 0, &VulkanInst) != VK_SUCCESS)
	{
		MessageBox(WindowPtr, "Failed To Create A Vulkan Instance", 0, 0);
	}

	// Get Number of Devices (note the double call, first is purely to get all devices)
	unsigned int NumDevices = 0;
	if ((vkEnumeratePhysicalDevices(VulkanInst, &NumDevices, 0) != VK_SUCCESS) ||
		(NumDevices == 0))
	{
		MessageBox(WindowPtr, "Could Not Detect a Physical Device", 0, 0);
	}

	// Get Handles to the devices
	VkPhysicalDevice PhysicalDevice[1]; 
	if (vkEnumeratePhysicalDevices(VulkanInst, &NumDevices, &PhysicalDevice[0]) != VK_SUCCESS)
	{
		MessageBox(WindowPtr, "Failed to Retrieve Handles to Physical Devices", 0, 0);
	}

	// Check Device Properties
	VkPhysicalDevice SelectedPhysicalDevice = VK_NULL_HANDLE;
	//uint32_t SelectedQueueFamilyIndex = UINT32_MAX;
	
	VkPhysicalDeviceProperties DeviceProperties;
	VkPhysicalDeviceFeatures DeviceFeatures;

	vkGetPhysicalDeviceProperties(PhysicalDevice[0], &DeviceProperties);
	vkGetPhysicalDeviceFeatures(PhysicalDevice[0], &DeviceFeatures);

	uint32_t MajorVersion = VK_VERSION_MAJOR(DeviceProperties.apiVersion);
	uint32_t MinorVersion = VK_VERSION_MINOR(DeviceProperties.apiVersion);
	uint32_t PatchVersion = VK_VERSION_PATCH(DeviceProperties.apiVersion);

	//  Check Available Queue Families
	uint32_t QueueFamiliesCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice[0], &QueueFamiliesCount, 0);
	if (QueueFamiliesCount == 0)
	{
		MessageBox(WindowPtr, "Physical Device Does Not Have Any Queue Families.", 0, 0);
	}


	if ((MajorVersion < 1) && (DeviceProperties.limits.maxImageDimension2D < 4096))
	{
		MessageBox(WindowPtr, "Physical Device Does Not Support Required Parameters", 0, 0);
	}

	SelectedPhysicalDevice = PhysicalDevice[0];
	if (SelectedPhysicalDevice == VK_NULL_HANDLE)
	{
		MessageBox(WindowPtr, "Could not select physical device based on the chosen properties.", 0, 0);
	}
	
	// Check physical device extensions for swap chain
	uint32_t PhysicalDeviceExtensionCount = 0;
	if ((vkEnumerateDeviceExtensionProperties(SelectedPhysicalDevice, 0,
		&PhysicalDeviceExtensionCount, 0) != VK_SUCCESS) ||
		(PhysicalDeviceExtensionCount == 0))
	{
		MessageBox(WindowPtr, "Error occurred during physical device extensions enumeration.", 0, 0);
	}

	std::vector<VkExtensionProperties> AvailablePhysicalDeviceExtensions(PhysicalDeviceExtensionCount);
	if ((vkEnumerateDeviceExtensionProperties(SelectedPhysicalDevice, 0,
		&PhysicalDeviceExtensionCount, &AvailablePhysicalDeviceExtensions[0]) != VK_SUCCESS) ||
		(PhysicalDeviceExtensionCount == 0))
	{
		MessageBox(WindowPtr, "Error occurred during physical device extensions enumeration.", 0, 0);
	}

	std::vector<const char*> DeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	Error = true;
	for (size_t i = 0; i < AvailablePhysicalDeviceExtensions.size(); ++i)
	{
		if (strcmp(AvailablePhysicalDeviceExtensions[i].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
		{
			Error = false;
		}
	}
	if (Error)
	{
		MessageBox(WindowPtr, "Couldn't find VK_KHR_SWAPCHAIN_EXTENSION_NAME", 0, 0);
	}


	// left off here

	std::vector<VkQueueFamilyProperties> QueueFamilyProperties(QueueFamiliesCount);
	vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice[0],
		&QueueFamiliesCount, QueueFamilyProperties.data());

	uint32_t QueueFamilyIndex = 0;
	for (uint32_t Index = 0; Index < QueueFamiliesCount; ++Index)
	{
		if ((QueueFamilyProperties[0].queueCount > 0) &&
			(QueueFamilyProperties[0].queueFlags & VK_QUEUE_GRAPHICS_BIT))
		{
			QueueFamilyIndex = Index;
		}
	}

	std::vector<float> QueuePriorities = { 1.0f };

	VkDeviceQueueCreateInfo QueueCreateInfo = {
		VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		0, 0, QueueFamilyIndex, static_cast<uint32_t>(QueuePriorities.size()),
		&QueuePriorities[0]
	};

	VkDeviceCreateInfo DeviceCreateInfo = {
		VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, 
		0, 0, 1, &QueueCreateInfo, 0, 0, 0, 0, 0
	};

	VkDevice VulkanDevice;
	if (vkCreateDevice(SelectedPhysicalDevice, &DeviceCreateInfo, 0, &VulkanDevice) != VK_SUCCESS)
	{
		MessageBox(WindowPtr, "Could not create a Vulkan Device.", 0, 0);
	}

	VkQueue VulkanQueue;
	vkGetDeviceQueue(VulkanDevice, QueueFamilyIndex, 0, &VulkanQueue);

#if defined(VK_USE_PLATFORM_WIN32_KHR)
	VkSurfaceKHR VulkanPresentSurface;
	VkWin32SurfaceCreateInfoKHR SurfaceCreateInfo = {
		VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		0, 0, hInstance, WindowPtr
	};

	if (vkCreateWin32SurfaceKHR(VulkanInst, &SurfaceCreateInfo, 0, &VulkanPresentSurface) != VK_SUCCESS)
	{
		MessageBox(WindowPtr, "Failed to create a win32 presentation surface for the Vulkan API.", 0, 0);
	}
#elif
	MessageBox(WindowPtr, "No Operating System defined for swap chain creation.", 0, 0);
#endif

	MSG Message = {};
	while (Message.message != WM_QUIT)
	{
		if (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}
	}

	if (VulkanDevice != VK_NULL_HANDLE)
	{
		vkDeviceWaitIdle(VulkanDevice);
		vkDestroyDevice(VulkanDevice, 0);
	}

	if (VulkanInst != VK_NULL_HANDLE)
	{
		vkDestroyInstance(VulkanInst, 0);
	}

	// Incase we loaded the functions for the graphics driver libraries.
	//if (VulkanLibrary)
	//{
	//	FreeLibrary(VulkanLibrary);
	//	dlclose(VulkanLibrary);
	//}

	return Message.message;
}


LRESULT CALLBACK WndProc(HWND WindowPtr, UINT Message,
	WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
	case WM_CREATE:
		break;
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_SIZE:
	{

	} break;
	case WM_CHAR:
	{
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
	} break;

	default:
		return DefWindowProc(WindowPtr, Message, wParam, lParam);
	}

	return 0;
}
