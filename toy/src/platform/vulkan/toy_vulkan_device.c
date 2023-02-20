#include "../../include/platform/vulkan/toy_vulkan_device.h"

#include "../../toy_assert.h"
#include "toy_vulkan_debug.h"
#include "../../include/toy_log.h"

typedef struct toy_vk_name_list_t {
	const char** names;
	uint32_t name_count;
}toy_vk_name_list_t;

static void toy_enumerate_vk_instance_layer_names_R (
	const toy_allocator_t* stack_alc_L,
	const toy_allocator_t* stack_alc_R,
	toy_vk_name_list_t* output,
	toy_error_t* error)
{
	const char* candidates[2];
	uint32_t candidate_count = 0;
#if TOY_DEBUG_VULKAN
	candidates[candidate_count++] = "VK_LAYER_KHRONOS_validation";
#endif
	TOY_ASSERT(candidate_count <= sizeof(candidates) / sizeof(candidates[0]));

	if (0 == candidate_count) {
		output->names = NULL;
		output->name_count = 0;
		toy_ok(error);
		return;
	}

	uint32_t cnt = 0;
	VkResult vk_err = vkEnumerateInstanceLayerProperties(&cnt, NULL);
	if (toy_unlikely(VK_SUCCESS != vk_err || 0 == cnt)) {
		toy_err_vkerr(TOY_ERROR_ASSERT_FAILED, vk_err, "vkEnumerateInstanceLayerProperties failed", error);
		return;
	}

	output->names = toy_alloc_aligned(stack_alc_R, sizeof(char*) * candidate_count, sizeof(char*));
	TOY_ASSERT(NULL != output->names);

	VkLayerProperties* props = toy_alloc_aligned(stack_alc_L, sizeof(VkLayerProperties) * cnt, sizeof(uint32_t));
	TOY_ASSERT(NULL != props);

	vk_err = vkEnumerateInstanceLayerProperties(&cnt, props);
	if (toy_unlikely(VK_SUCCESS != vk_err || 0 == cnt)) {
		toy_free_aligned(stack_alc_L, props);
		toy_free_aligned(stack_alc_R, (toy_aligned_p)output->names);
		toy_err_vkerr(TOY_ERROR_ASSERT_FAILED, vk_err, "vkEnumerateInstanceLayerProperties failed", error);
		return;
	}

	output->name_count = 0;
	for (uint32_t ci = 0; ci < candidate_count; ++ci) {
		for (uint32_t li = 0; li < cnt; ++li) {
			if (0 == strcmp(props[li].layerName, candidates[ci])) {
				output->names[output->name_count++] = candidates[ci];
				break;
			}
		}
	}

	toy_free_aligned(stack_alc_L, props);
	toy_ok(error);
}


static void toy_enumerate_vk_instance_extension_names_R (
	const toy_allocator_t* stack_alc_L,
	const toy_allocator_t* stack_alc_R,
	toy_vk_name_list_t* output,
	toy_error_t* error)
{
	const char* candidates[] = {
		VK_KHR_DISPLAY_EXTENSION_NAME,
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#if TOY_DEBUG_VULKAN
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
	};
	uint32_t candidate_count = sizeof(candidates) / sizeof(candidates[0]);

	uint32_t cnt = 0;
	VkResult vk_err = vkEnumerateInstanceExtensionProperties(NULL, &cnt, NULL);
	if (toy_unlikely(VK_SUCCESS != vk_err || 0 == cnt)) {
		toy_err_vkerr(TOY_ERROR_ASSERT_FAILED, vk_err, "vkEnumerateInstanceExtensionProperties failed", error);
		return;
	}

	output->names = toy_alloc_aligned(stack_alc_R, sizeof(char*) * candidate_count, sizeof(char*));
	TOY_ASSERT(NULL != output->names);

	VkExtensionProperties* props = toy_alloc_aligned(stack_alc_L, sizeof(VkExtensionProperties) * cnt, sizeof(uint32_t));
	TOY_ASSERT(NULL != props);

	vk_err = vkEnumerateInstanceExtensionProperties(NULL, &cnt, props);
	if (VK_SUCCESS != vk_err || 0 == cnt) {
		toy_free_aligned(stack_alc_L, props);
		toy_free_aligned(stack_alc_R, (toy_aligned_p)output->names);
		toy_err_vkerr(TOY_ERROR_ASSERT_FAILED, vk_err, "Second vkEnumerateInstanceExtensionProperties failed", error);
		return;
	}

	output->name_count = 0;
	for (uint32_t ci = 0; ci < candidate_count; ++ci) {
		for (uint32_t li = 0; li < cnt; ++li) {
			if (0 == strcmp(props[li].extensionName, candidates[ci])) {
				output->names[output->name_count++] = candidates[ci];
				break;
			}
		}
	}

	toy_free_aligned(stack_alc_L, props);
	toy_ok(error);
}


static void toy_create_vk_instance (
	toy_vulkan_device_setup_info_t* setup_info,
	VkDebugUtilsMessengerCreateInfoEXT* debug_messenger_ci,
	const toy_allocator_t* stack_alc_L,
	const toy_allocator_t* stack_alc_R,
	const VkAllocationCallbacks* vk_alc_cb,
	VkInstance* output,
	toy_error_t* error)
{
	VkApplicationInfo app_info;
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pNext = NULL;
	app_info.pApplicationName = (NULL == setup_info->app_name) ? "demo" : setup_info->app_name;
	app_info.applicationVersion = VK_MAKE_VERSION(setup_info->app_version[0], setup_info->app_version[1], setup_info->app_version[2]);
	app_info.pEngineName = "toy";
	app_info.apiVersion = VK_API_VERSION_1_0;

	toy_vk_name_list_t inst_layers;
	toy_enumerate_vk_instance_layer_names_R(stack_alc_L, stack_alc_R, &inst_layers, error);
	if (toy_unlikely(toy_is_failed(*error)))
		return;

	toy_vk_name_list_t inst_extensions;
	toy_enumerate_vk_instance_extension_names_R(stack_alc_L, stack_alc_R, &inst_extensions, error);
	if (toy_unlikely(toy_is_failed(*error))) {
		toy_free_aligned(stack_alc_R, (toy_aligned_p)(inst_layers.names));
		return;
	}

	void* pNext = NULL;
	if (NULL != debug_messenger_ci) {
		pNext = debug_messenger_ci;
#if TOY_DEBUG_VULKAN_BEST_PRACTICES
		VkValidationFeatureEnableEXT enables[] = { VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT };
		VkValidationFeaturesEXT features;
		features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
		features.pNext = pNext;
		features.enabledValidationFeatureCount = 1;
		features.pEnabledValidationFeatures = enables;
		features.disabledValidationFeatureCount = 0;
		features.pDisabledValidationFeatures = NULL;
		pNext = &features;
#endif
	}

	VkInstanceCreateInfo inst_ci;
	inst_ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	inst_ci.pNext = pNext;
	inst_ci.flags = 0;
	inst_ci.pApplicationInfo = &app_info;
	inst_ci.enabledLayerCount = inst_layers.name_count;
	inst_ci.ppEnabledLayerNames = inst_layers.names;
	inst_ci.enabledExtensionCount = inst_extensions.name_count;
	inst_ci.ppEnabledExtensionNames = inst_extensions.names;

	VkResult vk_err = vkCreateInstance(&inst_ci, vk_alc_cb, output);

	toy_free_aligned(stack_alc_R, (toy_aligned_p)(inst_extensions.names));
	toy_free_aligned(stack_alc_R, (toy_aligned_p)(inst_layers.names));

	if (toy_unlikely(VK_SUCCESS != vk_err))
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreateInstance failed", error);
	else
		toy_ok(error);
}


void toy_enumerate_vk_physical_device (
	VkInstance inst,
	const toy_allocator_t* stack_alc_R,
	toy_vulkan_physical_device_t* output,
	toy_error_t* error)
{
	TOY_ASSERT(VK_NULL_HANDLE != inst && NULL != stack_alc_R && NULL != output);
	TOY_ASSERT(NULL != error);

	uint32_t cnt = 0;
	VkResult vk_err = vkEnumeratePhysicalDevices(inst, &cnt, NULL);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "vkEnumeratePhysicalDevices failed", error);
		return;
	}
	if (toy_unlikely(0 == cnt)) {
		toy_err(TOY_ERROR_ASSERT_FAILED, "vkEnumeratePhysicalDevices returns 0 device", error);
		return;
	}

	VkPhysicalDevice* phy_devs = (VkPhysicalDevice*)toy_alloc_aligned(
		stack_alc_R, sizeof(VkPhysicalDevice) * cnt, sizeof(VkPhysicalDevice));
	if (toy_unlikely(NULL == phy_devs)) {
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "Alloc candidate VkPhysicalDevice failed", error);
		return;
	}

	vk_err = vkEnumeratePhysicalDevices(inst, &cnt, phy_devs);
	if (toy_unlikely(VK_SUCCESS != vk_err || 0 == cnt)) {
		toy_free_aligned(stack_alc_R, phy_devs);
		if (VK_SUCCESS != vk_err)
			toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "Secend vkEnumeratePhysicalDevices failed", error);
		else
			toy_err_vkerr(TOY_ERROR_ASSERT_FAILED, VK_ERROR_DEVICE_LOST, "Secend vkEnumeratePhysicalDevices returns 0 device", error);
		return;
	}

	int found = 0;
	for (uint32_t i = 0; i < cnt; ++i) {
		output->handle = phy_devs[i];
		vkGetPhysicalDeviceProperties(phy_devs[i], &(output->properties));
		vkGetPhysicalDeviceMemoryProperties(phy_devs[i], &(output->memory_properties));
		vkGetPhysicalDeviceFeatures(phy_devs[i], &(output->features));

		// Todo: filter GPUs that not match the required features
		// Todo: rate GPUs to select a best one or just show all supported for player to choose
		if (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU == output->properties.deviceType) {
			found = 1;
			break;
		}
	}

	toy_free_aligned(stack_alc_R, phy_devs);

	if (toy_likely(!!found))
		toy_ok(error);
	else
		toy_err(TOY_ERROR_OPERATION_FAILED, "Can't find suitable VkPhysicalDevice", error);
}


static VkResult toy_create_vk_surface_handle (
	VkInstance inst,
	const toy_window_t* window,
	const VkAllocationCallbacks* vk_alc_cb,
	VkSurfaceKHR* output)
{
#if TOY_OS_WINDOWS
	VkWin32SurfaceCreateInfoKHR winSurfCI;
	winSurfCI.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	winSurfCI.pNext = NULL;
	winSurfCI.flags = 0;
	winSurfCI.hinstance = window->os_particular.hInst;
	winSurfCI.hwnd = window->os_particular.hWnd;
	return vkCreateWin32SurfaceKHR(inst, &winSurfCI, vk_alc_cb, output);
#else
#error "Unsupported OS to create Vulkan surface"
#endif
}


static void toy_select_vk_surface_format (
	VkPhysicalDevice phy_dev,
	VkSurfaceKHR surface,
	const toy_allocator_t* stack_alc_R,
	VkSurfaceFormatKHR* output,
	toy_error_t* error)
{
	TOY_ASSERT(NULL != error);
	TOY_ASSERT(VK_NULL_HANDLE != phy_dev && VK_NULL_HANDLE != surface && NULL != stack_alc_R && NULL != output);

	uint32_t cnt = 0;
	VkResult vk_err = vkGetPhysicalDeviceSurfaceFormatsKHR(phy_dev, surface, &cnt, NULL);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "vkGetPhysicalDeviceSurfaceFormatsKHR failed", error);
		return;
	}
	TOY_ASSERT(cnt >= 1); // The number of format pairs supported must be greater than or equal to 1. -- by specification

	VkSurfaceFormatKHR* fmts = (VkSurfaceFormatKHR*)toy_alloc_aligned(
		stack_alc_R, sizeof(VkSurfaceFormatKHR) * cnt, sizeof(VkSurfaceFormatKHR*));
	if (toy_unlikely(NULL == fmts)) {
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "Alloc candidate VkSurfaceFormatKHR failed", error);
		return;
	}

	vk_err = vkGetPhysicalDeviceSurfaceFormatsKHR(phy_dev, surface, &cnt, fmts);
	if (VK_SUCCESS != vk_err) {
		toy_free_aligned(stack_alc_R, fmts);
		toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "Second vkGetPhysicalDeviceSurfaceFormatsKHR failed", error);
		return;
	}
	TOY_ASSERT(cnt >= 1);

	output->format = VK_FORMAT_MAX_ENUM;
	output->colorSpace = VK_COLOR_SPACE_MAX_ENUM_KHR;
	for (uint32_t i = 0; i < cnt; ++i) {
		// VK_FORMAT_B8G8R8A8_SRGB is the most common format supported
		if (VK_FORMAT_B8G8R8A8_SRGB == fmts[i].format && VK_COLORSPACE_SRGB_NONLINEAR_KHR == fmts[i].colorSpace) {
			*output = fmts[i];
			break;
		}
	}
	if (VK_FORMAT_MAX_ENUM == output->format)
		*output = fmts[0];

	toy_free_aligned(stack_alc_R, fmts);
	toy_ok(error);
}


void toy_select_vk_present_mode (
	VkPhysicalDevice phy_dev,
	VkSurfaceKHR surface,
	VkPresentModeKHR suggest,
	const toy_allocator_t* stack_alc_R,
	VkPresentModeKHR* output,
	toy_error_t* error)
{
	TOY_ASSERT(NULL != error);
	TOY_ASSERT(VK_NULL_HANDLE != phy_dev && VK_NULL_HANDLE != surface && NULL != stack_alc_R && NULL != output);

	uint32_t cnt = 0;
	VkResult vk_err = vkGetPhysicalDeviceSurfacePresentModesKHR(phy_dev, surface, &cnt, NULL);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_err_vkerr(TOY_ERROR_ASSERT_FAILED, vk_err, "vkGetPhysicalDeviceSurfacePresentModesKHR failed", error);
		return;
	}
	TOY_ASSERT(cnt >= 1);  // VK_PRESENT_MODE_FIFO_KHR is required to be supported by specification

	VkPresentModeKHR* modes = (VkPresentModeKHR*)toy_alloc_aligned(
		stack_alc_R, sizeof(VkPresentModeKHR) * cnt, sizeof(VkPresentModeKHR*));
	if (toy_unlikely(NULL == modes)) {
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "Alloc candidate VkPresentModeKHR failed", error);
		return;
	}

	vk_err = vkGetPhysicalDeviceSurfacePresentModesKHR(phy_dev, surface, &cnt, modes);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_free_aligned(stack_alc_R, modes);
		toy_err_vkerr(TOY_ERROR_ASSERT_FAILED, vk_err, "Second vkGetPhysicalDeviceSurfacePresentModesKHR failed", error);
		return;
	}
	TOY_ASSERT(cnt >= 1);

	for (uint32_t i = 0; i < cnt; ++i) {
		if (suggest == modes[i]) {
			*output = suggest;
			break;
		}
	}

	if (*output != suggest) {
		int vSyncOff = 1;
		*output = VK_PRESENT_MODE_FIFO_KHR; // V-sync is on by default
		for (uint32_t i = 0; i < cnt; ++i) {
			if (VK_PRESENT_MODE_FIFO_RELAXED_KHR == modes[i] && vSyncOff) {
				*output = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
				break;
			}
		}
	}

	toy_free_aligned(stack_alc_R, modes);
	toy_ok(error);
}


static void toy_create_vk_surface (
	VkInstance inst,
	VkPhysicalDevice phy_dev,
	const toy_window_t* window,
	const toy_allocator_t* stack_alc_R,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_vulkan_surface_t* output,
	toy_error_t* error)
{
	TOY_ASSERT(NULL != error);
	TOY_ASSERT(VK_NULL_HANDLE != inst && VK_NULL_HANDLE != phy_dev && NULL != stack_alc_R && NULL != output);
	VkResult vk_err;

	if (NULL == window) {
		memset(output, 0, sizeof(toy_vulkan_surface_t));
		output->handle = VK_NULL_HANDLE;
		toy_ok(error);
		return;
	}

	vk_err = toy_create_vk_surface_handle(inst, window, vk_alc_cb, &output->handle);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "toy_create_vulkan_surface_handle failed", error);
		return;
	}

	vk_err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phy_dev, output->handle, &output->capabilities);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_err_vkerr(TOY_ERROR_UNKNOWN_ERROR, vk_err, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed", error);
		vkDestroySurfaceKHR(inst, output->handle, vk_alc_cb);
		return;
	}

	toy_select_vk_surface_format(phy_dev, output->handle, stack_alc_R, &output->format, error);
	if (toy_unlikely(toy_is_failed(*error))) {
		vkDestroySurfaceKHR(inst, output->handle, vk_alc_cb);
		return;
	}

	toy_select_vk_present_mode(phy_dev, output->handle, VK_PRESENT_MODE_MAILBOX_KHR, stack_alc_R, &output->present_mode, error);
	if (toy_unlikely(toy_is_failed(*error))) {
		vkDestroySurfaceKHR(inst, output->handle, vk_alc_cb);
		return;
	}

	toy_ok(error);
}


struct toy_queue_family_cmp_t {
	uint32_t queue_index;
	VkQueueFamilyProperties* prop;
};
static int toy_cmp_vulkan_queue_family_cnt(const void* a, const void* b) {
	uint32_t a_cnt = ((struct toy_queue_family_cmp_t*)a)->prop->queueCount;
	uint32_t b_cnt = ((struct toy_queue_family_cmp_t*)b)->prop->queueCount;
	if (a_cnt > b_cnt)
		return -1;
	else if (a_cnt == b_cnt)
		return 0;
	else
		return 1;
}
static void toy_select_vk_device_queue_families (
	VkPhysicalDevice phy_dev,
	VkSurfaceKHR surface,
	const toy_allocator_t* stack_alc_R,
	toy_vulkan_device_queue_families_t* output,
	toy_error_t* error)
{
	TOY_ASSERT(VK_NULL_HANDLE != phy_dev && VK_NULL_HANDLE != surface && NULL != stack_alc_R && NULL != output);
	TOY_ASSERT(NULL != error);

	uint32_t cnt = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(phy_dev, &cnt, NULL);
	TOY_ASSERT(cnt >= 1); // Implementations must support at least one queue family -- by specification

	VkQueueFamilyProperties* props = toy_alloc_aligned(stack_alc_R, sizeof(VkQueueFamilyProperties) * cnt, sizeof(VkQueueFlags));
	if (toy_unlikely(NULL == props)) {
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "Alloc VkQueueFamilyProperties failed", error);
		return;
	}

	vkGetPhysicalDeviceQueueFamilyProperties(phy_dev, &cnt, props);
	TOY_ASSERT(cnt >= 1);

	struct toy_queue_family_cmp_t* sort_arr = (struct toy_queue_family_cmp_t*)toy_alloc_aligned(stack_alc_R, sizeof(struct toy_queue_family_cmp_t) * cnt, sizeof(uint32_t));
	if (toy_unlikely(NULL == sort_arr)) {
		toy_free_aligned(stack_alc_R, props);
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "Alloc toy_queue_family_cmp_t failed", error);
		return;
	}

	for (uint32_t i = 0; i < cnt; ++i) {
		sort_arr[i].queue_index = i;
		sort_arr[i].prop = &(props[i]);
	}
	if (cnt >= 2)
		qsort(sort_arr, cnt, sizeof(struct toy_queue_family_cmp_t), toy_cmp_vulkan_queue_family_cnt);

	uint32_t graphic_family_index = VK_QUEUE_FAMILY_IGNORED;
	for (uint32_t i = 0; i < cnt; ++i) {
		if (sort_arr[i].prop->queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			graphic_family_index = sort_arr[i].queue_index;
			break;
		}
	}

	uint32_t compute_family_index = VK_QUEUE_FAMILY_IGNORED;
	for (uint32_t i = 0; i < cnt; ++i) {
		if (sort_arr[i].prop->queueFlags & VK_QUEUE_COMPUTE_BIT) {
			compute_family_index = sort_arr[i].queue_index;
			if (compute_family_index != graphic_family_index)
				break;
		}
	}

	uint32_t present_family_index = VK_QUEUE_FAMILY_IGNORED;
	if (VK_NULL_HANDLE != surface) {
		for (uint32_t i = 0; i < cnt; ++i) {
			VkBool32 present_supported = VK_FALSE;
			VkResult vk_err = vkGetPhysicalDeviceSurfaceSupportKHR(phy_dev, i, surface, &present_supported);
			if (VK_SUCCESS != vk_err) {
				toy_free_aligned(stack_alc_R, sort_arr);
				toy_free_aligned(stack_alc_R, props);
				toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "vkGetPhysicalDeviceSurfaceSupportKHR failed", error);
			}
			if (present_supported) {
				if (graphic_family_index == present_family_index) {
					present_family_index = graphic_family_index;
					break;
				}
				if (VK_QUEUE_FAMILY_IGNORED == present_family_index)
					present_family_index = i;
			}
		}
	}

	/*
	* All commands that are allowed on a queue that supports transfer operations
	* are also allowed on a queue that supports either graphics or compute operations.
	* Thus, if the capabilities of a queue family include VK_QUEUE_GRAPHICS_BIT or
	* VK_QUEUE_COMPUTE_BIT, then reporting the VK_QUEUE_TRANSFER_BIT
	* capability separately for that queue family is optional.
	*/
	uint32_t transfer_family_index = VK_QUEUE_FAMILY_IGNORED;
	for (uint32_t i = 0; i < cnt; ++i) {
		if (sort_arr[i].prop->queueFlags & VK_QUEUE_TRANSFER_BIT) {
			uint32_t queue_index = sort_arr[i].queue_index;
			if (queue_index != graphic_family_index && queue_index != compute_family_index) {
				transfer_family_index = queue_index;
				break;
			}
			else if (transfer_family_index == VK_QUEUE_FAMILY_IGNORED) {
				transfer_family_index = queue_index;
			}
		}
	}

	/* Todo: modify count and offset for better performance */
	output->graphic.family = graphic_family_index;
	output->graphic.count = 1; // family_props[graphic_family_index].queueCount;
	output->graphic.offset = 0;
	output->compute.family = compute_family_index;
	output->compute.count = 1; // family_props[compute_family_index].queueCount;
	output->compute.offset = 0;
	output->transfer.family = transfer_family_index;
	output->transfer.count = 1; // family_props[transfer_family_index].queueCount;
	output->transfer.offset = 0;
	output->present.family = present_family_index;
	output->present.count = 1;
	output->present.offset = 0;

	toy_free_aligned(stack_alc_R, sort_arr);
	toy_free_aligned(stack_alc_R, props);
	toy_ok(error);
}


static void toy_select_enabled_vk_device_features (
	const VkPhysicalDeviceFeatures* phy_dev_features,
	VkPhysicalDeviceFeatures* output)
{
	*output = *phy_dev_features;
}


static void toy_enumerate_vk_device_extension_names_R (
	VkPhysicalDevice phy_dev,
	const toy_allocator_t* stack_alc_L,
	const toy_allocator_t* stack_alc_R,
	toy_vk_name_list_t* output,
	toy_error_t* error)
{
	const char* candidates[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_MAINTENANCE1_EXTENSION_NAME,
	};
	const uint32_t candidate_count = sizeof(candidates) / sizeof(candidates[0]);

	uint32_t cnt = 0;
	VkResult vk_err = vkEnumerateDeviceExtensionProperties(phy_dev, NULL, &cnt, NULL);
	if (toy_unlikely(VK_SUCCESS != vk_err || 0 == cnt)) {
		toy_err_vkerr(TOY_ERROR_ASSERT_FAILED, vk_err, "vkEnumerateDeviceExtensionProperties failed", error);
		return;
	}

	output->names = toy_alloc_aligned(stack_alc_R, sizeof(char*) * candidate_count, sizeof(char*));
	TOY_ASSERT(NULL != output->names);

	VkExtensionProperties* props = toy_alloc_aligned(stack_alc_L, sizeof(VkExtensionProperties) * cnt, sizeof(uint32_t));
	TOY_ASSERT(NULL != props);

	vk_err = vkEnumerateDeviceExtensionProperties(phy_dev, NULL, &cnt, props);
	if (VK_SUCCESS != vk_err || 0 == cnt) {
		toy_free_aligned(stack_alc_L, props);
		toy_free_aligned(stack_alc_R, (toy_aligned_p)output->names);
		toy_err_vkerr(TOY_ERROR_ASSERT_FAILED, vk_err, "Second vkEnumerateDeviceExtensionProperties failed", error);
		return;
	}

	// Use for flip Y axis of Vulkan viewport with Vulkan 1.0
	// https://www.saschawillems.de/blog/2019/03/29/flipping-the-vulkan-viewport/
	output->name_count = 0;
	for (uint32_t ci = 0; ci < candidate_count; ++ci) {
		for (uint32_t li = 0; li < cnt; ++li) {
			if (0 == strcmp(props[li].extensionName, candidates[ci])) {
				output->names[output->name_count++] = candidates[ci];
				break;
			}
		}
	}

	toy_free_aligned(stack_alc_L, props);
	toy_ok(error);
}


static void toy_create_vulkan_device_handle (
	VkPhysicalDevice phy_dev,
	const toy_vulkan_queue_family_t* device_families,
	uint32_t family_count,
	const VkPhysicalDeviceFeatures* enabled_features,
	const toy_allocator_t* stack_alc_L,
	const toy_allocator_t* stack_alc_R,
	const VkAllocationCallbacks* vk_alc_cb,
	VkDevice* output,
	toy_error_t* error)
{
	uint32_t queue_ci_cnt = 0;
	VkDeviceQueueCreateInfo queue_ci[4];
	for (uint32_t i = 0; i < family_count; ++i) {
		int duplicate_family = -1;
		for (uint32_t j = 0; j < queue_ci_cnt; ++j) {
			if (queue_ci[j].queueFamilyIndex == device_families[i].family) {
				duplicate_family = j;
				break;
			}
		}
		if (-1 == duplicate_family) {
			queue_ci[queue_ci_cnt].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_ci[queue_ci_cnt].pNext = NULL;
			queue_ci[queue_ci_cnt].flags = 0;
			queue_ci[queue_ci_cnt].queueFamilyIndex = device_families[i].family;
			queue_ci[queue_ci_cnt].queueCount = device_families[i].count;
			++queue_ci_cnt;
		}
		else
			queue_ci[duplicate_family].queueCount += device_families[i].count;
	}

	uint32_t max_queue_count = 1;
	for (uint32_t i = 0; i < queue_ci_cnt; ++i) {
		if (queue_ci[i].queueCount > max_queue_count)
			max_queue_count = queue_ci[i].queueCount;
	}
	float* priorities = (float*)toy_alloc_aligned(stack_alc_R, sizeof(float) * max_queue_count, sizeof(float));
	if (toy_unlikely(NULL == priorities)) {
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "Alloc queue priorities failed", error);
		return;
	}
	for (uint32_t i = 0; i < max_queue_count; ++i)
		priorities[i] = 1.0f; // 0 ~ 1
	for (int i = 0; i < 4; ++i)
		queue_ci[i].pQueuePriorities = priorities;

	// use the same layers of instance
	toy_vk_name_list_t device_layers;
	toy_enumerate_vk_instance_layer_names_R(stack_alc_L, stack_alc_R, &device_layers, error);
	if (toy_unlikely(toy_is_failed(*error))) {
		toy_free_aligned(stack_alc_R, (toy_aligned_p)priorities);
		return;
	}

	toy_vk_name_list_t device_extensions;
	toy_enumerate_vk_device_extension_names_R(phy_dev, stack_alc_L, stack_alc_R, &device_extensions, error);
	if (toy_unlikely(toy_is_failed(*error))) {
		toy_free_aligned(stack_alc_R, (toy_aligned_p)(device_layers.names));
		toy_free_aligned(stack_alc_R, (toy_aligned_p)priorities);
		return;
	}

	VkDeviceCreateInfo device_ci;
	device_ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_ci.pNext = NULL;
	device_ci.flags = 0;
	device_ci.queueCreateInfoCount = queue_ci_cnt;
	device_ci.pQueueCreateInfos = queue_ci;
	device_ci.enabledLayerCount = device_layers.name_count;
	device_ci.ppEnabledLayerNames = device_layers.names;
	device_ci.enabledExtensionCount = device_extensions.name_count;
	device_ci.ppEnabledExtensionNames = device_extensions.names;
	device_ci.pEnabledFeatures = enabled_features;

	VkResult vk_err = vkCreateDevice(phy_dev, &device_ci, vk_alc_cb, output);

	toy_free_aligned(stack_alc_R, (toy_aligned_p)(device_extensions.names));
	toy_free_aligned(stack_alc_R, (toy_aligned_p)(device_layers.names));
	toy_free_aligned(stack_alc_R, (toy_aligned_p)priorities);

	if (toy_unlikely(VK_SUCCESS != vk_err))
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreateDevice failed", error);
	else
		toy_ok(error);
}


void toy_create_vulkan_device (
	toy_window_t* window,
	toy_vulkan_device_setup_info_t* setup_info,
	const toy_memory_allocator_t* alc,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_vulkan_device_t* output,
	toy_error_t* error)
{
	memset(output, 0, sizeof(*output));

	VkDebugUtilsMessengerCreateInfoEXT* debug_messenger_ci = NULL;
#if TOY_DEBUG_VULKAN
	debug_messenger_ci = toy_get_vulkan_debug_messenger_ci();
#endif
	toy_create_vk_instance(setup_info, debug_messenger_ci, &alc->stack_alc_L, &alc->stack_alc_R, vk_alc_cb, &output->instance, error);
	if (toy_unlikely(toy_is_failed(*error)))
		goto FAIL_INSTANCE;

	VkResult vk_err = toy_create_vulkan_debug_messenger(output->instance, vk_alc_cb, &output->debug_messenger);
	if (toy_unlikely(VK_SUCCESS != vk_err)) {
		toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "Create vulkan debug messenger failed", error);
		goto FAIL_DBG_MSG;
	}

	toy_enumerate_vk_physical_device(output->instance, &alc->stack_alc_R, &output->physical_device, error);
	if (toy_unlikely(toy_is_failed(*error)))
		goto FAIL_PHY_DEV;

	toy_create_vk_surface(output->instance, output->physical_device.handle, window, &alc->stack_alc_R, vk_alc_cb, &output->surface, error);
	if (toy_unlikely(toy_is_failed(*error)))
		goto FAIL_SURFACE;

	toy_select_vk_device_queue_families(output->physical_device.handle, output->surface.handle, &alc->stack_alc_R, &output->device_queue_families, error);
	if (toy_unlikely(toy_is_failed(*error)))
		goto FAIL_DEVICE_QUEUE;

	toy_select_enabled_vk_device_features(&output->physical_device.features, &output->enabled_physical_device_features);

	toy_create_vulkan_device_handle(
		output->physical_device.handle,
		output->device_queue_families.union_array,
		sizeof(output->device_queue_families) / sizeof(toy_vulkan_queue_family_t),
		&output->enabled_physical_device_features,
		&alc->stack_alc_L,
		&alc->stack_alc_R,
		vk_alc_cb,
		&output->handle,
		error);
	if (toy_unlikely(toy_is_failed(*error)))
		goto FAIL_DEVICE_HANDLE;

	toy_ok(error);
	return;

FAIL_DEVICE_HANDLE:
FAIL_DEVICE_QUEUE:
	vkDestroySurfaceKHR(output->instance, output->surface.handle, vk_alc_cb);
FAIL_SURFACE:
FAIL_PHY_DEV:
	toy_destroy_vulkan_debug_messenger(output->instance, output->debug_messenger, vk_alc_cb);
FAIL_DBG_MSG:
	vkDestroyInstance(output->instance, vk_alc_cb);
FAIL_INSTANCE:
	toy_log_error(error);
	return;
}


void toy_destroy_vulkan_device (
	toy_vulkan_device_t* device,
	const toy_memory_allocator_t* alc,
	const VkAllocationCallbacks* vk_alc)
{
	vkDestroyDevice(device->handle, vk_alc);
	vkDestroySurfaceKHR(device->instance, device->surface.handle, vk_alc);
	toy_destroy_vulkan_debug_messenger(device->instance, device->debug_messenger, vk_alc);
	vkDestroyInstance(device->instance, vk_alc);
}



VkBool32 toy_check_vulkan_msaa_count_supported (
	VkSampleCountFlagBits msaa_count,
	const VkPhysicalDeviceLimits* limits)
{
	VkSampleCountFlags counts =
		limits->framebufferColorSampleCounts &
		limits->framebufferDepthSampleCounts &
		limits->framebufferStencilSampleCounts;

	return (msaa_count & counts) > 0;
}


// MSAA: Multi-Sample Anti-Aliasing
VkSampleCountFlagBits toy_select_vulkan_msaa (
	VkSampleCountFlagBits setting,
	const VkPhysicalDeviceLimits* limits)
{
	VkSampleCountFlagBits ret = VK_SAMPLE_COUNT_64_BIT;
	while (ret > setting)
		ret >>= 1;
	if (toy_unlikely(0 == ret))
		ret = VK_SAMPLE_COUNT_1_BIT;
	while (!toy_check_vulkan_msaa_count_supported(ret, limits))
		ret >>= 1;
	return ret;
}


// return VK_FORMAT_MAX_ENUM when failed
VkFormat toy_select_vulkan_supported_format (
	VkPhysicalDevice phy_dev,
	const VkFormat* candidates,
	uint32_t candidate_count,
	VkImageTiling tiling,
	VkFormatFeatureFlags required_features)
{
	VkFormat ret = VK_FORMAT_MAX_ENUM;
	// VkImageFormatProperties
	// vkGetPhysicalDeviceImageFormatProperties

	if (VK_IMAGE_TILING_OPTIMAL == tiling) {
		for (uint32_t i = 0; i < candidate_count; ++i) {
			VkFormatProperties fmt_props;
			vkGetPhysicalDeviceFormatProperties(phy_dev, candidates[i], &fmt_props);

			if ((fmt_props.optimalTilingFeatures & required_features) == required_features)
				return candidates[i];
		}
	}
	else if (VK_IMAGE_TILING_LINEAR == tiling) {
		for (uint32_t i = 0; i < candidate_count; ++i) {
			VkFormatProperties fmt_props;
			vkGetPhysicalDeviceFormatProperties(phy_dev, candidates[i], &fmt_props);

			if ((fmt_props.linearTilingFeatures & required_features) == required_features)
				return candidates[i];
		}
	}
	return ret;
}


// return VK_FORMAT_MAX_ENUM when failed
VkFormat toy_select_vulkan_depth_image_format (VkPhysicalDevice phy_dev)
{
	VkFormat candidates[] = {
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_S8_UINT,
	};
	return toy_select_vulkan_supported_format(
		phy_dev,
		candidates,
		sizeof(candidates) / sizeof(*candidates),
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}


// return VK_FORMAT_MAX_ENUM when failed
VkFormat toy_select_vulkan_image_compress_format (VkPhysicalDevice phy_dev)
{
	VkFormat candidates[] = {
		VK_FORMAT_BC7_UNORM_BLOCK,
		VK_FORMAT_BC7_SRGB_BLOCK,
		VK_FORMAT_BC5_UNORM_BLOCK,
		VK_FORMAT_BC5_SNORM_BLOCK,
		VK_FORMAT_BC1_RGB_UNORM_BLOCK,
		VK_FORMAT_BC1_RGB_SRGB_BLOCK,
		VK_FORMAT_ASTC_4x4_UNORM_BLOCK,
		VK_FORMAT_ASTC_4x4_SRGB_BLOCK,
		VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,
		VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,
	};
	VkFormatFeatureFlags required_features =
		VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
		// VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT |
		VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
	return toy_select_vulkan_supported_format(
		phy_dev,
		candidates,
		sizeof(candidates) / sizeof(*candidates),
		VK_IMAGE_TILING_OPTIMAL,
		required_features
	);
}
