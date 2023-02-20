#pragma once

#include "../../toy_platform.h"
#include "../../toy_error.h"
#include "../../toy_window.h"
#include "../../toy_allocator.h"
#include "toy_vulkan_device.h"

#if __cplusplus
#include <vulkan/vulkan.hpp>
#else
#include <vulkan/vulkan.h>
#endif


typedef struct toy_vulkan_swapchain_t {
	VkSwapchainKHR handle;
	VkExtent2D extent;

	VkFence start_fences[TOY_CONCURRENT_FRAME_MAX]; // signaled after aquire swapchain image
	VkSemaphore finish_semaphores[TOY_CONCURRENT_FRAME_MAX]; // signaled by render command
	VkFence finish_fences[TOY_CONCURRENT_FRAME_MAX]; // signaled by render command
	uint32_t frame_count;
	uint32_t current_frame;

	VkImage* present_images;
	VkImageView* present_views;
	uint32_t image_count;
	uint32_t current_image; // index of images and views
}toy_vulkan_swapchain_t;


TOY_EXTERN_C_START

void toy_create_vulkan_swapchain (
	toy_vulkan_device_t* device,
	toy_window_t* window,
	const toy_allocator_t* alc,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_vulkan_swapchain_t* output,
	toy_error_t* error
);

void toy_destroy_vulkan_swapchain (
	VkDevice dev,
	toy_vulkan_swapchain_t* swapchain,
	const toy_allocator_t* alc,
	const VkAllocationCallbacks* vk_alc_cb
);

void toy_swap_vulkan_swapchain (
	VkDevice dev,
	toy_vulkan_swapchain_t* swapchain,
	toy_error_t* error
);

void toy_present_vulkan_swapchain (
	VkDevice dev,
	toy_vulkan_swapchain_t* swapchain,
	VkQueue present_queue,
	toy_error_t* error
);

TOY_EXTERN_C_END
