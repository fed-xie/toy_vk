#include "descriptor_set_layout.h"

#include "../../toy_assert.h"

static VkResult create_single_texture (
	VkDevice dev,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_vulkan_descriptor_set_layout_t* output)
{
	static const VkDescriptorSetLayoutBinding bindings[] = {
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = NULL,
		},
	};
	static const binding_count = sizeof(bindings) / sizeof(bindings[0]);
	return toy_create_vulkan_descriptor_set_layout(dev, bindings, binding_count, vk_alc_cb, output);
}


static const toy_vulkan_descriptor_set_binding_t s_single_texture[] = {
	{
		.desc_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.binding = 0,
		.offset = offsetof(struct toy_built_in_descriptor_set_single_texture_t, image_ref),
		.size = sizeof(toy_asset_pool_item_ref_t) * 2,
	},
};


uint32_t toy_alloc_vulkan_descriptor_set_single_texture (
	toy_asset_manager_t* asset_mgr,
	const toy_vulkan_descriptor_set_layout_t* desc_set_layout,
	toy_error_t* error)
{
	uint32_t material_index = toy_alloc_material(asset_mgr, sizeof(toy_built_in_descriptor_set_single_texture_t), error);
	if (toy_is_failed(*error))
		return UINT32_MAX;

	toy_built_in_descriptor_set_single_texture_t** desc_set_data_p = (toy_built_in_descriptor_set_single_texture_t**)toy_get_asset_item(
		&asset_mgr->asset_pools.material, material_index);
	TOY_ASSERT(NULL != desc_set_data_p);
	toy_built_in_descriptor_set_single_texture_t* desc_set_data = *desc_set_data_p;
	desc_set_data->header.desc_set_layout = desc_set_layout;
	desc_set_data->header.bindings = s_single_texture;

	toy_asset_pool_item_ref_t empty_ref;
	empty_ref.pool = NULL;
	empty_ref.index = UINT32_MAX;
	empty_ref.next_ref = UINT32_MAX;

	desc_set_data->image_ref = empty_ref;
	desc_set_data->sampler_ref = empty_ref;
	toy_ok(error);
	return material_index;
}



static VkResult create_main_camera (
	VkDevice dev,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_vulkan_descriptor_set_layout_t* output)
{
	static const VkDescriptorSetLayoutBinding layout_bindings[] = {
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.pImmutableSamplers = NULL,
		}, {
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.pImmutableSamplers = NULL,
		}, {
			.binding = 2,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.pImmutableSamplers = NULL,
		},{
			.binding = 3,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.pImmutableSamplers = NULL,
		},
	};
	static const binding_count = sizeof(layout_bindings) / sizeof(layout_bindings[0]);
	return toy_create_vulkan_descriptor_set_layout(dev, layout_bindings, binding_count, vk_alc_cb, output);
}


void toy_creaet_built_in_vulkan_descriptor_set_layouts (
	VkDevice dev,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_built_in_vulkan_descriptor_set_layout_t* output,
	toy_error_t* error)
{
	VkResult vk_err = create_main_camera(dev, vk_alc_cb, &output->main_camera);
	if (VK_SUCCESS != vk_err)
		goto FAIL;

	vk_err = create_single_texture(dev, vk_alc_cb, &output->single_texture);
	if (VK_SUCCESS != vk_err)
		goto FAIL;

	toy_ok(error);
	return;
FAIL:
	toy_destroy_built_in_vulkan_descriptor_set_layouts(dev, vk_alc_cb, output);
	toy_err_vkerr(TOY_ERROR_CREATE_OBJECT_FAILED, vk_err, "Create descriptor set layout failed", error);
	return;
}


void toy_destroy_built_in_vulkan_descriptor_set_layouts (
	VkDevice dev,
	const VkAllocationCallbacks* vk_alc_cb,
	toy_built_in_vulkan_descriptor_set_layout_t* desc_set_layouts)
{
	toy_vulkan_descriptor_set_layout_t* layouts = (toy_vulkan_descriptor_set_layout_t*)desc_set_layouts;
	const int layout_count = sizeof(*desc_set_layouts) / sizeof(toy_vulkan_descriptor_set_layout_t);

	for (int i = 0; i < layout_count; ++i) {
		vkDestroyDescriptorSetLayout(dev, layouts[i].handle, vk_alc_cb);
		layouts[i].handle = VK_NULL_HANDLE;
	}
}
