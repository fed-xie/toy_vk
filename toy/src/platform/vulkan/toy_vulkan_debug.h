#pragma once

#include "../../include/toy_platform.h"
#if __cplusplus
#include <vulkan/vulkan.hpp>
#else
#include <vulkan/vulkan.h>
#endif



TOY_EXTERN_C_START

VkDebugUtilsMessengerCreateInfoEXT* toy_get_vulkan_debug_messenger_ci ();

VkResult toy_create_vulkan_debug_messenger (
	VkInstance inst,
	const VkAllocationCallbacks* alloc_cb,
	VkDebugUtilsMessengerEXT* msger_p
);

void toy_destroy_vulkan_debug_messenger (
	VkInstance inst,
	VkDebugUtilsMessengerEXT msger,
	const VkAllocationCallbacks* alloc_cb
);

TOY_EXTERN_C_END
