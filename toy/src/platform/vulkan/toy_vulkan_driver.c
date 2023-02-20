#include "../../include/platform/vulkan/toy_vulkan_driver.h"

#include "../../toy_assert.h"


void toy_create_vulkan_driver (
	toy_vulkan_setup_info_t* setup_info,
	toy_window_t* window,
	toy_memory_allocator_t* alc,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_vulkan_driver_t* output,
	toy_error_t* error)
{
	memset(output, 0, sizeof(toy_vulkan_driver_t));

	toy_create_vulkan_device(window, &setup_info->device_setup_info, alc, vk_alc_cb, &output->device, error);
	if (toy_unlikely(toy_is_failed(*error)))
		return;

	toy_create_vulkan_memory_allocator(
		output->device.handle,
		output->device.physical_device.handle,
		vk_alc_cb,
		alc,
		&output->vk_allocator,
		error);
	if (toy_unlikely(toy_is_failed(*error))) {
		toy_destroy_vulkan_device(&output->device, alc, vk_alc_cb);
		return;
	}

	toy_create_vulkan_swapchain(
		&output->device,
		window,
		&alc->buddy_alc,
		vk_alc_cb,
		&output->swapchain,
		error);
	if (toy_unlikely(toy_is_failed(*error))) {
		toy_destroy_vulkan_memory_allocator(&output->vk_allocator);
		toy_destroy_vulkan_device(&output->device, alc, vk_alc_cb);
		return;
	}

	output->render_config.msaa_count = toy_select_vulkan_msaa(
		setup_info->msaa_count, &output->device.physical_device.properties.limits);
	output->render_config.depth_format = toy_select_vulkan_depth_image_format(output->device.physical_device.handle);
	output->render_config.compress_format = toy_select_vulkan_image_compress_format(output->device.physical_device.handle);

	output->vk_alc_cb_p = vk_alc_cb;

	vkGetDeviceQueue(
		output->device.handle,
		output->device.device_queue_families.transfer.family,
		output->device.device_queue_families.transfer.offset,
		&output->transfer_queue);
	vkGetDeviceQueue(
		output->device.handle,
		output->device.device_queue_families.graphic.family,
		output->device.device_queue_families.graphic.offset,
		&output->graphic_queue);
	if (VK_NULL_HANDLE != output->device.surface.handle) {
		vkGetDeviceQueue(
			output->device.handle,
			output->device.device_queue_families.present.family,
			output->device.device_queue_families.present.offset,
			&output->present_queue);
	}
	else {
		output->present_queue = VK_NULL_HANDLE;
	}

	toy_ok(error);
}


void toy_destroy_vulkan_driver (toy_vulkan_driver_t* vk_driver, const toy_memory_allocator_t* alc)
{
	toy_destroy_vulkan_swapchain(vk_driver->device.handle, &vk_driver->swapchain, &alc->buddy_alc, vk_driver->vk_alc_cb_p);

	toy_destroy_vulkan_memory_allocator(&vk_driver->vk_allocator);

	toy_destroy_vulkan_device(&vk_driver->device, alc, vk_driver->vk_alc_cb_p);
}
