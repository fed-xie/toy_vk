#pragma once

#include "../../toy_platform.h"

#include "toy_vulkan_device.h"
#include "toy_vulkan_memory.h"
#include "toy_vulkan_swapchain.h"


typedef struct toy_vulkan_driver_t {
	toy_vulkan_device_t device;

	toy_vulkan_memory_allocator_t vk_allocator;

	toy_vulkan_swapchain_t swapchain;

	struct {
		VkSampleCountFlagBits msaa_count;
		VkFormat depth_format;
		VkFormat compress_format;
	} render_config;

	VkQueue transfer_queue;
	VkQueue graphic_queue;
	VkQueue present_queue;

	const VkAllocationCallbacks* vk_alc_cb_p;
}toy_vulkan_driver_t;


TOY_EXTERN_C_START

typedef struct toy_vulkan_setup_info_t {
	toy_vulkan_device_setup_info_t device_setup_info;
	VkSampleCountFlagBits msaa_count;
}toy_vulkan_setup_info_t;

void toy_create_vulkan_driver (
	toy_vulkan_setup_info_t* setup_info,
	toy_window_t* window,
	toy_memory_allocator_t* alc,
	const VkAllocationCallbacks* vk_alc,
	toy_vulkan_driver_t* output,
	toy_error_t* error
);


void toy_destroy_vulkan_driver (toy_vulkan_driver_t* vk_driver, const toy_memory_allocator_t* alc);

TOY_EXTERN_C_END
