#pragma once

#include "../../toy_platform.h"

#if TOY_OS_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#if __cplusplus
#include <vulkan/vulkan.hpp>
#else
#include <vulkan/vulkan.h>
#endif


typedef struct toy_vulkan_physical_device_t {
	VkPhysicalDevice handle;
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceMemoryProperties memory_properties;
	VkPhysicalDeviceFeatures features;
}toy_vulkan_physical_device_t;


typedef struct toy_vulkan_surface_t {
	VkSurfaceKHR handle;
	VkSurfaceCapabilitiesKHR capabilities;
	VkSurfaceFormatKHR format;
	VkPresentModeKHR present_mode;
}toy_vulkan_surface_t;


typedef struct toy_vulkan_queue_family_t {
	uint32_t family;
	uint32_t count;
	uint32_t offset;
}toy_vulkan_queue_family_t;


typedef struct toy_vulkan_device_queue_families_t {
	union {
		struct {
			toy_vulkan_queue_family_t graphic;
			toy_vulkan_queue_family_t compute;
			toy_vulkan_queue_family_t transfer;
			toy_vulkan_queue_family_t present;
		};
		struct toy_vulkan_queue_family_t union_array[4];
	};
}toy_vulkan_device_queue_families_t;



#include "../../toy_memory.h"
#include "../../toy_allocator.h"
#include "../../toy_window.h"

typedef struct toy_vulkan_device_setup_info_t {
	const char* app_name;
	uint32_t app_version[3];
}toy_vulkan_device_setup_info_t;

typedef struct toy_vulkan_device_t {
	VkDevice handle;
	toy_vulkan_device_queue_families_t device_queue_families;
	VkPhysicalDeviceFeatures enabled_physical_device_features;

	toy_vulkan_physical_device_t physical_device;

	toy_vulkan_surface_t surface;

	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_messenger;
}toy_vulkan_device_t;


TOY_EXTERN_C_START

void toy_create_vulkan_device (
	toy_window_t* window,
	toy_vulkan_device_setup_info_t* setup_info,
	const toy_memory_allocator_t* alc,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_vulkan_device_t* output,
	toy_error_t* error
);


void toy_destroy_vulkan_device (
	toy_vulkan_device_t* device,
	const toy_memory_allocator_t* alc,
	const VkAllocationCallbacks* vk_alc_cb
);



VkBool32 toy_check_vulkan_msaa_count_supported (
	VkSampleCountFlagBits msaa_count,
	const VkPhysicalDeviceLimits* limits
);

VkSampleCountFlagBits toy_select_vulkan_msaa (
	VkSampleCountFlagBits setting,
	const VkPhysicalDeviceLimits* limits
);

// return VK_FORMAT_MAX_ENUM when failed
VkFormat toy_select_vulkan_depth_image_format (VkPhysicalDevice phy_dev);

// return VK_FORMAT_MAX_ENUM when failed
VkFormat toy_select_vulkan_image_compress_format (VkPhysicalDevice phy_dev);

TOY_EXTERN_C_END
