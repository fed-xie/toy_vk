#include "toy_vulkan_debug.h"

#include "../../include/toy_log.h"

VKAPI_ATTR VkBool32 VKAPI_CALL toy_vulkan_debug_callback (
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	if (pCallbackData != NULL && pCallbackData->pMessage != NULL) {
		if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			toy_log_message((enum toy_log_level)messageSeverity, "%s\n", pCallbackData->pMessage);
		}
	}
	return VK_FALSE;
}


VkDebugUtilsMessengerCreateInfoEXT* toy_get_vulkan_debug_messenger_ci () {
#if TOY_DEBUG_VULKAN
	static VkDebugUtilsMessengerCreateInfoEXT dbgExt;
	dbgExt.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	dbgExt.pNext = NULL;
	dbgExt.flags = 0;
	dbgExt.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	dbgExt.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	dbgExt.pfnUserCallback = toy_vulkan_debug_callback;
	dbgExt.pUserData = NULL;
	return &dbgExt;
#else
	return NULL;
#endif
}


VkResult toy_create_vulkan_debug_messenger (
	VkInstance inst,
	const VkAllocationCallbacks* alloc_cb,
	VkDebugUtilsMessengerEXT* msger_p)
{
#if TOY_DEBUG_VULKAN
	PFN_vkCreateDebugUtilsMessengerEXT pfn_vkCreateDebugUtilsMessengerEXT =
		(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(inst, "vkCreateDebugUtilsMessengerEXT");
	PFN_vkDestroyDebugUtilsMessengerEXT pfn_vkDestroyDebugUtilsMessengerEXT =
		(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(inst, "vkDestroyDebugUtilsMessengerEXT");

	if (NULL == pfn_vkCreateDebugUtilsMessengerEXT || NULL == pfn_vkDestroyDebugUtilsMessengerEXT)
		return VK_ERROR_EXTENSION_NOT_PRESENT;

	return (*pfn_vkCreateDebugUtilsMessengerEXT)(inst, toy_get_vulkan_debug_messenger_ci(), alloc_cb, msger_p);
#else
	return VK_SUCCESS;
#endif
}

void toy_destroy_vulkan_debug_messenger (
	VkInstance inst,
	VkDebugUtilsMessengerEXT msger,
	const VkAllocationCallbacks* alloc_cb)
{
	if (VK_NULL_HANDLE == inst || VK_NULL_HANDLE == msger)
		return;

	PFN_vkDestroyDebugUtilsMessengerEXT pfn_vkDestroyDebugUtilsMessengerEXT =
		(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(inst, "vkDestroyDebugUtilsMessengerEXT");

	if (NULL == pfn_vkDestroyDebugUtilsMessengerEXT)
		return;

	(*pfn_vkDestroyDebugUtilsMessengerEXT)(inst, msger, alloc_cb);
}
