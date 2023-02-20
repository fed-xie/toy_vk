#include "../../include/platform/vulkan/toy_vulkan_image.h"


void toy_destroy_vulkan_image (
	VkDevice dev,
	toy_vulkan_image_t* image,
	const VkAllocationCallbacks* vk_alc_cb)
{
	vkDestroyImageView(dev, image->view, vk_alc_cb);
	toy_free_vulkan_memory_binding(&image->binding);
	vkDestroyImage(dev, image->handle, vk_alc_cb);
}
