#include "pipeline_layout.h"


static void create_mesh (
	VkDevice dev,
	const toy_built_in_vulkan_descriptor_set_layout_t* built_in_layouts,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_vulkan_pipeline_layout_t* output,
	toy_error_t* error)
{
	VkResult vk_err;

	VkDescriptorSetLayout desc_set_layouts[] = {
		built_in_layouts->main_camera.handle,
		built_in_layouts->single_texture.handle
	};

	VkPipelineLayout layout;
	VkPipelineLayoutCreateInfo layout_ci;
	layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_ci.pNext = NULL;
	layout_ci.flags = 0;
	layout_ci.setLayoutCount = sizeof(desc_set_layouts) / sizeof(*desc_set_layouts);
	layout_ci.pSetLayouts = desc_set_layouts;
	layout_ci.pushConstantRangeCount = 0;
	layout_ci.pPushConstantRanges = NULL;
	vk_err = vkCreatePipelineLayout(dev, &layout_ci, vk_alc_cb, &layout);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreatePipelineLayout failed", error);
		return;
	}

	output->desc_set_layouts[0] = built_in_layouts->main_camera.handle;
	output->desc_set_layouts[1] = built_in_layouts->single_texture.handle;
	output->desc_set_layouts[2] = VK_NULL_HANDLE;
	output->desc_set_layouts[3] = VK_NULL_HANDLE;
	output->handle = layout;
	toy_ok(error);
}


static void create_shadow (
	VkDevice dev,
	const toy_built_in_vulkan_descriptor_set_layout_t* built_in_layouts,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_vulkan_pipeline_layout_t* output,
	toy_error_t* error)
{
	VkResult vk_err;

	VkDescriptorSetLayout desc_set_layouts[] = {
		built_in_layouts->main_camera.handle,
	};

	VkPipelineLayout layout;
	VkPipelineLayoutCreateInfo layout_ci;
	layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_ci.pNext = NULL;
	layout_ci.flags = 0;
	layout_ci.setLayoutCount = sizeof(desc_set_layouts) / sizeof(*desc_set_layouts);
	layout_ci.pSetLayouts = desc_set_layouts;
	layout_ci.pushConstantRangeCount = 0;
	layout_ci.pPushConstantRanges = NULL;
	vk_err = vkCreatePipelineLayout(dev, &layout_ci, vk_alc_cb, &layout);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "vkCreatePipelineLayout failed", error);
		return;
	}

	output->desc_set_layouts[0] = built_in_layouts->main_camera.handle;
	output->desc_set_layouts[1] = VK_NULL_HANDLE;
	output->desc_set_layouts[2] = VK_NULL_HANDLE;
	output->desc_set_layouts[3] = VK_NULL_HANDLE;
	output->handle = layout;
	toy_ok(error);
}


void toy_create_built_in_vulkan_pipeline_layouts (
	VkDevice dev,
	const toy_built_in_vulkan_descriptor_set_layout_t* built_in_layouts,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_built_in_vulkan_pipeline_layouts_t* output,
	toy_error_t* error)
{
	memset(output, 0, sizeof(*output));

	create_mesh(dev, built_in_layouts, vk_alc_cb, &output->mesh, error);
	if (toy_is_failed(*error))
		goto FAIL;

	create_shadow(dev, built_in_layouts, vk_alc_cb, &output->shadow, error);
	if (toy_is_failed(*error))
		goto FAIL;

	toy_ok(error);
	return;
FAIL:
	toy_destroy_built_in_vulkan_pipeine_layouts(dev, vk_alc_cb, output);
}


void toy_destroy_built_in_vulkan_pipeine_layouts (
	VkDevice dev,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_built_in_vulkan_pipeline_layouts_t* pipeline_layouts)
{
	toy_vulkan_pipeline_layout_t* layouts = (toy_vulkan_pipeline_layout_t*)pipeline_layouts;
	const int layout_count = sizeof(*pipeline_layouts) / sizeof(toy_vulkan_pipeline_layout_t);

	for (int i = 0; i < layout_count; ++i) {
		if (VK_NULL_HANDLE != layouts[i].handle)
			vkDestroyPipelineLayout(dev, layouts[i].handle, vk_alc_cb);
	}
}
