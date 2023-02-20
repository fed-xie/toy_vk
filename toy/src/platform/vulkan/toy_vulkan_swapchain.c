#include "../../include/platform/vulkan/toy_vulkan_swapchain.h"

#include "../../include/platform/vulkan/toy_vulkan_device.h"
#include "../../toy_assert.h"
#include "../../include/toy_log.h"


static void toy_adjust_vulkan_swapchain_extent (
	const VkSurfaceCapabilitiesKHR* cap,
	VkExtent2D* in_out)
{
	if (cap->currentExtent.width != UINT32_MAX) {
		*in_out = cap->currentExtent;
		return;
	}

	uint32_t width = in_out->width, height = in_out->height;
	if (width > cap->maxImageExtent.width)
		width = cap->maxImageExtent.width;
	if (width < cap->minImageExtent.width)
		width = cap->minImageExtent.width;

	if (height > cap->maxImageExtent.height)
		height = cap->maxImageExtent.height;
	if (height < cap->minImageExtent.height)
		height = cap->minImageExtent.height;

	in_out->width = width;
	in_out->height = height;
}


static VkResult toy_create_vulkan_swapchain_handle (
	VkDevice dev,
	toy_vulkan_surface_t* surface,
	VkExtent2D extent,
	const uint32_t* queue_families,
	uint32_t queue_family_count,
	VkSwapchainKHR old,
	const VkAllocationCallbacks* vk_alc_cb,
	VkSwapchainKHR* output)
{
	uint32_t min_image_count = 3;
	if (min_image_count < surface->capabilities.minImageCount)
		min_image_count = surface->capabilities.minImageCount; // at least one -- by specification
	if (surface->capabilities.maxImageCount > 0 && min_image_count > surface->capabilities.maxImageCount)
		min_image_count = surface->capabilities.maxImageCount;

	if (VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR == surface->present_mode ||
		VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR == surface->present_mode)
		min_image_count = 1;

	VkSwapchainCreateInfoKHR swapchain_ci;
	swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_ci.pNext = NULL;
	swapchain_ci.flags = 0;
	swapchain_ci.surface = surface->handle;
	swapchain_ci.minImageCount = min_image_count;
	swapchain_ci.imageFormat = surface->format.format;
	swapchain_ci.imageColorSpace = surface->format.colorSpace;
	swapchain_ci.imageExtent = extent;
	swapchain_ci.imageArrayLayers = 1; // Always 1 except stereoscopic 3D application
	swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_ci.imageSharingMode = queue_family_count > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
	swapchain_ci.queueFamilyIndexCount = queue_family_count;
	swapchain_ci.pQueueFamilyIndices = queue_families;
	swapchain_ci.preTransform = surface->capabilities.currentTransform;
	swapchain_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_ci.presentMode = surface->present_mode;
	swapchain_ci.clipped = VK_TRUE;
	swapchain_ci.oldSwapchain = old;
	return vkCreateSwapchainKHR(dev, &swapchain_ci, vk_alc_cb, output);
}


static VkResult toy_create_vulkan_swapchain_present_image_views (
	VkDevice dev,
	const VkImage* swapchain_images,
	uint32_t image_count,
	VkFormat image_format,
	const VkAllocationCallbacks* vk_alc_cb,
	VkImageView* output)
{
	for (uint32_t i = 0; i < image_count; ++i) {
		VkImageViewCreateInfo img_view_ci;
		img_view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		img_view_ci.pNext = NULL;
		img_view_ci.flags = 0;
		img_view_ci.image = swapchain_images[i];
		img_view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
		img_view_ci.format = image_format;
		img_view_ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		img_view_ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		img_view_ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		img_view_ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		img_view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		img_view_ci.subresourceRange.baseMipLevel = 0;
		img_view_ci.subresourceRange.levelCount = 1;
		img_view_ci.subresourceRange.baseArrayLayer = 0;
		img_view_ci.subresourceRange.layerCount = 1;

		VkResult vk_err = vkCreateImageView(dev, &img_view_ci, vk_alc_cb, &output[i]);
		if (VK_SUCCESS != vk_err) {
			for (uint32_t j = 0; j < i; ++j)
				vkDestroyImageView(dev, output[j], vk_alc_cb);
			return vk_err;
		}
	}

	return VK_SUCCESS;
}


static void toy_create_vulkan_swapchain_fences_and_semaphores (
	VkDevice dev,
	toy_vulkan_swapchain_t* swapchain,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_error_t* error)
{
	VkResult vk_err;

	VkFenceCreateInfo fence_ci;
	fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_ci.pNext = NULL;
	fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	for (uint32_t i = 0; i < TOY_CONCURRENT_FRAME_MAX; ++i) {
		vk_err = vkCreateFence(dev, &fence_ci, vk_alc_cb, &swapchain->start_fences[i]);
		if (toy_unlikely(VK_SUCCESS != vk_err)) {
			toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreateFence for swapchain failed", error);
			for (uint32_t j = i + 1; j > 0; --j)
				vkDestroyFence(dev, swapchain->start_fences[j - 1], vk_alc_cb);
			goto FAIL_START_FENCE;
		}
	}

	VkSemaphoreCreateInfo sem_ci;
	sem_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	sem_ci.pNext = NULL;
	sem_ci.flags = 0;
	for (uint32_t i = 0; i < TOY_CONCURRENT_FRAME_MAX; ++i) {
		vk_err = vkCreateSemaphore(dev, &sem_ci, vk_alc_cb, &swapchain->finish_semaphores[i]);
		if (toy_unlikely(VK_SUCCESS != vk_err)) {
			toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreateSemaphore for swapchain failed", error);
			for (uint32_t j = i + 1; j > 0; --j)
				vkDestroySemaphore(dev, swapchain->finish_semaphores[j - 1], vk_alc_cb);
			goto FAIL_FINISH_SEM;
		}
	}

	fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	for (uint32_t i = 0; i < TOY_CONCURRENT_FRAME_MAX; ++i) {
		vk_err = vkCreateFence(dev, &fence_ci, vk_alc_cb, &swapchain->finish_fences[i]);
		if (toy_unlikely(VK_SUCCESS != vk_err)) {
			toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreateFence for swapchain failed", error);
			for (uint32_t j = i + 1; j > 0; --j)
				vkDestroyFence(dev, swapchain->finish_fences[j - 1], vk_alc_cb);
			goto FAIL_FINISH_FENCE;
		}
	}

	toy_ok(error);
	return;

FAIL_FINISH_FENCE:
	for (uint32_t j = TOY_CONCURRENT_FRAME_MAX; j > 0; --j)
		vkDestroySemaphore(dev, swapchain->finish_semaphores[j - 1], vk_alc_cb);
FAIL_FINISH_SEM:
	for (uint32_t j = TOY_CONCURRENT_FRAME_MAX; j > 0; --j)
		vkDestroyFence(dev, swapchain->start_fences[j - 1], vk_alc_cb);
FAIL_START_FENCE:
	return;
}


static void toy_create_vulkan_swapchain_impl (
	VkDevice dev,
	VkExtent2D extent,
	toy_vulkan_surface_t* surface,
	toy_vulkan_device_queue_families_t* queue_families,
	const toy_allocator_t* alc,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_vulkan_swapchain_t* output,
	toy_error_t* error)
{
	memset(output, 0, sizeof(toy_vulkan_swapchain_t));
	VkResult vk_err;

	if (NULL == surface || VK_NULL_HANDLE == surface->handle) {
		output->handle = VK_NULL_HANDLE;
		toy_ok(error);
		return;
	}

	output->extent = extent;
	toy_adjust_vulkan_swapchain_extent(&surface->capabilities, &output->extent);

	uint32_t families[2];
	uint32_t family_count = 0;
	if (VK_QUEUE_FAMILY_IGNORED != queue_families->graphic.family)
		families[family_count++] = queue_families->graphic.family;
	if (VK_QUEUE_FAMILY_IGNORED != queue_families->present.family &&
		queue_families->present.family != queue_families->graphic.family)
		families[family_count++] = queue_families->present.family;

	vk_err = toy_create_vulkan_swapchain_handle(
		dev,
		surface,
		output->extent,
		families,
		family_count,
		VK_NULL_HANDLE,
		vk_alc_cb,
		&output->handle);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "toy_create_vulkan_swapchain failed", error);
		goto FAIL_SWAPCHAIN;
	}

	uint32_t image_count;
	vk_err = vkGetSwapchainImagesKHR(dev, output->handle, &image_count, NULL);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_ASSERT_FAILED, vk_err, "vkGetSwapchainImagesKHR failed", error);
		goto FAIL_COUNT;
	}
	TOY_ASSERT(0 < image_count);
	output->image_count = image_count;

	size_t ptr_size = output->image_count * (sizeof(VkImage) + sizeof(VkImageView));
	toy_aligned_p ptr = toy_alloc_aligned(alc, ptr_size, sizeof(VkImage));
	if (toy_unlikely(NULL == ptr)) {
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "Malloc for vulkan swapchain images failed", error);
		goto FAIL_CREATE_ARRAY;
	}
	output->present_images = (VkImage*)ptr;
	output->present_views = (VkImageView*)(&output->present_images[image_count]);

	vk_err = vkGetSwapchainImagesKHR(dev, output->handle, &image_count, output->present_images);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_ASSERT_FAILED, vk_err, "Second vkGetSwapchainImagesKHR failed", error);
		goto FAIL_PRESENT_IMAGE;
	}
	TOY_ASSERT(image_count == output->image_count);

	vk_err = toy_create_vulkan_swapchain_present_image_views(
		dev,
		output->present_images,
		output->image_count,
		surface->format.format,
		vk_alc_cb,
		output->present_views);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreateImageView for swapchain present image failed", error);
		goto FAIL_PRESENT_VIEW;
	}

	output->current_image = 0;

	toy_create_vulkan_swapchain_fences_and_semaphores(
		dev, output, vk_alc_cb, error);
	if (toy_unlikely(toy_is_failed(*error)))
		goto FAIL_FENCE_AND_SEMAPHORE;

	output->frame_count = TOY_CONCURRENT_FRAME_MAX;
	output->current_frame = TOY_CONCURRENT_FRAME_MAX - 1;

	toy_ok(error);
	return;

FAIL_FENCE_AND_SEMAPHORE:
	for (uint32_t i = output->image_count; i > 0; --i)
		vkDestroyImageView(dev, output->present_views[i - 1], vk_alc_cb);
FAIL_PRESENT_VIEW:
FAIL_PRESENT_IMAGE:
	toy_free_aligned(alc, ptr);
FAIL_CREATE_ARRAY:
FAIL_COUNT:
	vkDestroySwapchainKHR(dev, output->handle, vk_alc_cb);
FAIL_SWAPCHAIN:
	toy_log_error(error);
	memset(output, 0, sizeof(toy_vulkan_swapchain_t));
	return;
}


void toy_create_vulkan_swapchain (
	toy_vulkan_device_t* device,
	toy_window_t* window,
	const toy_allocator_t* alc,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_vulkan_swapchain_t* output,
	toy_error_t* error)
{
	VkExtent2D swapchain_extent = { .width = window->width, .height = window->height };

	toy_create_vulkan_swapchain_impl(
		device->handle,
		swapchain_extent,
		&device->surface,
		&device->device_queue_families,
		alc,
		vk_alc_cb,
		output,
		error);
}


void toy_destroy_vulkan_swapchain (
	VkDevice dev,
	toy_vulkan_swapchain_t* swapchain,
	const toy_allocator_t* alc,
	const VkAllocationCallbacks* vk_alc_cb)
{
	for (uint32_t i = TOY_CONCURRENT_FRAME_MAX; i > 0; --i)
		vkDestroyFence(dev, swapchain->finish_fences[i - 1], vk_alc_cb);
	for (uint32_t j = TOY_CONCURRENT_FRAME_MAX; j > 0; --j)
		vkDestroySemaphore(dev, swapchain->finish_semaphores[j - 1], vk_alc_cb);
	for (uint32_t i = TOY_CONCURRENT_FRAME_MAX; i > 0; --i)
		vkDestroyFence(dev, swapchain->start_fences[i - 1], vk_alc_cb);

	for (uint32_t i = swapchain->image_count; i > 0; --i)
		vkDestroyImageView(dev, swapchain->present_views[i - 1], vk_alc_cb);

	toy_free_aligned(alc, (toy_aligned_p)swapchain->present_images);

	vkDestroySwapchainKHR(dev, swapchain->handle, vk_alc_cb);
	memset(swapchain, 0, sizeof(*swapchain));
}


void toy_swap_vulkan_swapchain (
	VkDevice dev,
	toy_vulkan_swapchain_t* swapchain,
	toy_error_t* error)
{
	swapchain->current_frame = (swapchain->current_frame + 1) % swapchain->frame_count;
	VkResult vk_err;

	uint64_t timeout = 50 * 1000 * 1000; // 50ms
	vk_err = vkWaitForFences(dev, 1, &swapchain->finish_fences[swapchain->current_frame], VK_TRUE, timeout);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "vkWaitForFences to wait render finish failed", error);
		return;
	}

	VkFence frame_fences[] = {
		swapchain->finish_fences[swapchain->current_frame],
		swapchain->start_fences[swapchain->current_frame],
	};
	const uint32_t fence_count = sizeof(frame_fences) / sizeof(VkFence);
	vk_err = vkResetFences(dev, fence_count, frame_fences);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "vkResetFence for swapchain failed", error);
		return;
	}

	vk_err = vkAcquireNextImageKHR(
		dev,
		swapchain->handle,
		timeout,
		VK_NULL_HANDLE,
		swapchain->start_fences[swapchain->current_frame],
		&swapchain->current_image);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "vkAcquireNextImageKHR failed, need to recreate swapchain", error);
		return;
	}

	vk_err = vkWaitForFences(dev, 1, &swapchain->start_fences[swapchain->current_frame], VK_TRUE, timeout);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "vkWaitForFences to vkAcquireNextImageKHR failed", error);
		return;
	}

	toy_ok(error);
}


void toy_present_vulkan_swapchain (
	VkDevice dev,
	toy_vulkan_swapchain_t* swapchain,
	VkQueue present_queue,
	toy_error_t* error)
{
	VkResult vk_err;

	VkSwapchainKHR swapchains[] = { swapchain->handle };
	const uint32_t swapchain_count = sizeof(swapchains) / sizeof(VkSwapchainKHR);

	VkSemaphore frame_semaphores[] = { swapchain->finish_semaphores[swapchain->current_frame] };

	VkPresentInfoKHR present_info;
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pNext = NULL;
	present_info.waitSemaphoreCount = sizeof(frame_semaphores) / sizeof(VkSemaphore);
	present_info.pWaitSemaphores = frame_semaphores;
	present_info.swapchainCount = swapchain_count;
	present_info.pSwapchains = swapchains;
	present_info.pImageIndices = &(swapchain->current_image);
	present_info.pResults = NULL; // Since we use only 1 swapchain now, the function return value is enough

	vk_err = vkQueuePresentKHR(present_queue, &present_info);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "vkQueuePresentKHR failed", error);
		return;
	}

	toy_ok(error);
}
