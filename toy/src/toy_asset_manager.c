#include "include/toy_asset_manager.h"

#include "toy_assert.h"
#include <string.h>
#include "include/toy_log.h"
#include "include/platform/vulkan/toy_vulkan_pipeline.h"

#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb_image.h"

static void destroy_vulkan_mesh_primitive (
	toy_asset_pool_t* pool,
	void* asset)
{
	toy_vulkan_mesh_primitive_asset_pool_t* vk_primitive_pool = pool->context;
	toy_vulkan_mesh_primitive_p mesh_primitive = asset;

	toy_free_vulkan_mesh_primitive(vk_primitive_pool, mesh_primitive);
}

static void destroy_mesh (
	toy_asset_pool_t* pool,
	void* asset)
{
	toy_asset_manager_t* asset_mgr = pool->context;
	toy_asset_pool_t* primitive_pool = &asset_mgr->asset_pools.mesh_primitive;
	toy_asset_pool_t* material_pool = &asset_mgr->asset_pools.material;
	toy_mesh_t* mesh = asset;

	toy_sub_asset_ref(primitive_pool, mesh->primitive_index, 1);
	if (0 == toy_get_asset_ref(primitive_pool, mesh->primitive_index))
		toy_free_asset_item(primitive_pool, mesh->primitive_index);

	if (UINT32_MAX != mesh->material_index) {
		toy_sub_asset_ref(material_pool, mesh->material_index, 1);
		if (0 == toy_get_asset_ref(material_pool, mesh->material_index))
			toy_free_asset_item(material_pool, mesh->material_index);
	}
}

static void destroy_image (
	toy_asset_pool_t* pool,
	void* asset)
{
	toy_vulkan_memory_allocator_p vk_alc = pool->context;
	toy_vulkan_image_t* image = asset;
	toy_destroy_vulkan_image(vk_alc->device, image, vk_alc->vk_alc_cb_p);
}

static void destroy_image_sampler (
	toy_asset_pool_t* pool,
	void* asset)
{
	toy_asset_manager_t* asset_mgr = pool->context;
	toy_vulkan_sampler_t* sampler = asset;

	vkDestroySampler(
		asset_mgr->vk_private.vk_driver->device.handle,
		sampler->handle,
		asset_mgr->vk_private.vk_driver->vk_alc_cb_p);
}

static void destroy_material (
	toy_asset_pool_t* pool,
	void* asset)
{
	TOY_ASSERT(NULL != asset);
	toy_asset_manager_t* asset_mgr = pool->context;
	toy_vulkan_descriptor_set_data_header_t* desc_set_data = *(void**)asset;

	uint32_t binding_count = desc_set_data->desc_set_layout->binding_count;
	const VkDescriptorSetLayoutBinding* layout_bindings = desc_set_data->desc_set_layout->layout_bindings;
	const toy_vulkan_descriptor_set_binding_t* bindings = desc_set_data->bindings;

	for (uint32_t i = 0; i < binding_count; ++i) {
		switch (layout_bindings[i].descriptorType) {
		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		{
			toy_asset_pool_item_ref_t* image_ref = (toy_asset_pool_item_ref_t*)((uintptr_t)desc_set_data + bindings[i].offset);
			toy_asset_pool_item_ref_t* sampler_ref = image_ref + 1;
			if (NULL != image_ref->pool && UINT32_MAX != image_ref->index) {
				toy_sub_asset_ref(image_ref->pool, image_ref->index, 1);
				if (0 == toy_get_asset_ref(image_ref->pool, image_ref->index))
					toy_free_asset_item(image_ref->pool, image_ref->index);
			}
			if (NULL != sampler_ref->pool && UINT32_MAX != sampler_ref->index) {
				toy_sub_asset_ref(sampler_ref->pool, sampler_ref->index, 1);
				if (0 == toy_get_asset_ref(sampler_ref->pool, sampler_ref->index))
					toy_free_asset_item(sampler_ref->pool, sampler_ref->index);
			}
			break;
		}
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
		default:
			TOY_ASSERT(0);
		}
	}

	toy_free_aligned(&asset_mgr->alc->list_alc, desc_set_data);
}


void toy_create_asset_manager (
	size_t cache_size,
	toy_memory_allocator_t* alc,
	toy_vulkan_driver_t* vk_driver,
	toy_asset_manager_t* output,
	toy_error_t* error)
{
	TOY_ASSERT(1024 * 1024 <= cache_size && "Too small cache size");
	TOY_ASSERT(NULL != output && NULL != error);

	memset(output, 0, sizeof(*output));

	output->alc = alc;

	output->cache_stack = toy_create_memory_stack(cache_size + sizeof(toy_memory_stack_t), alc, &output->stack_alc_L, &output->stack_alc_R);
	if (NULL == output->cache_stack) {
		toy_err(TOY_ERROR_MEMORY_DEVICE_ALLOCATION_FAILED, "Failed to create asset cache stack", error);
		goto FAIL_MEMORY_STACK;
	}
	output->cache_size = cache_size;

	output->file_api = toy_std_file_interface();

	toy_create_memory_pools(8 * 1024 * 1024, TOY_MEMORY_CHUNK_SIZE, alc, &output->chunk_pools, &output->chunk_alc);
	if (NULL == output->chunk_pools) {
		toy_err(TOY_ERROR_MEMORY_DEVICE_ALLOCATION_FAILED, "Failed to create asset chunk pool", error);
		goto FAIL_MEMORY_POOL;
	}

	toy_create_asset_item_ref_pool(256, &alc->buddy_alc, &output->item_ref_pool, error);
	if (toy_is_failed(*error))
		goto FAIL_ITEM_REF_POOL;

	output->vk_private.vk_driver = vk_driver;

	toy_create_vulkan_asset_loader(
		&vk_driver->device,
		16 * 1024 * 1024,
		&vk_driver->vk_allocator,
		&output->vk_private.vk_asset_loader,
		error);
	if (toy_is_failed(*error))
		goto FAIL_VK_ASSET_LOADER;

	toy_create_vulkan_mesh_primitive_asset_pool(
		2 * 1024 * 1024,
		0,
		1024 * 1024,
		0,
		&vk_driver->vk_allocator,
		&output->vk_private.vk_mesh_primitive_pool,
		error);
	if (toy_is_failed(*error))
		goto FAIL_VK_MESH_PRIMITIVE;

	toy_init_asset_pool(
		sizeof(toy_vulkan_mesh_primitive_t),
		sizeof(void*),
		destroy_vulkan_mesh_primitive,
		&output->chunk_alc,
		&alc->buddy_alc,
		&output->vk_private.vk_mesh_primitive_pool,
		"Vulkan mesh primitive",
		&output->asset_pools.mesh_primitive);

	toy_init_asset_pool(
		sizeof(toy_mesh_t),
		sizeof(void*),
		destroy_mesh,
		&output->chunk_alc,
		&alc->buddy_alc,
		output,
		"Mesh",
		&output->asset_pools.mesh);

	toy_init_asset_pool(
		sizeof(toy_vulkan_image_t),
		sizeof(void*),
		destroy_image,
		&output->chunk_alc,
		&alc->buddy_alc,
		&output->vk_private.vk_driver->vk_allocator,
		"Vulkan image",
		&output->asset_pools.image);

	toy_init_asset_pool(
		sizeof(void*),
		sizeof(void*),
		destroy_material,
		&output->chunk_alc,
		&alc->buddy_alc,
		output,
		"Material Pointer",
		&output->asset_pools.material);

	toy_init_asset_pool(
		sizeof(toy_vulkan_sampler_t),
		sizeof(uint32_t),
		destroy_image_sampler,
		&output->chunk_alc,
		&alc->buddy_alc,
		output,
		"Vulkan image sampler",
		&output->asset_pools.image_sampler);
	
	toy_ok(error);
	return;

FAIL_VK_MESH_PRIMITIVE:
	toy_destroy_vulkan_asset_loader(
		output->vk_private.vk_driver->device.handle,
		&output->vk_private.vk_asset_loader);
FAIL_VK_ASSET_LOADER:
	toy_destroy_asset_ref_pool(&output->item_ref_pool);
FAIL_ITEM_REF_POOL:
	toy_destroy_memory_pools(output->chunk_pools, alc);
FAIL_MEMORY_POOL:
	toy_destroy_memory_stack(output->cache_stack, alc);
FAIL_MEMORY_STACK:
	return;
}


void toy_destroy_asset_manager (
	toy_asset_manager_t* asset_mgr)
{
	toy_memory_allocator_t* alc = asset_mgr->alc;

	toy_destroy_asset_pool(&alc->buddy_alc, &asset_mgr->asset_pools.material);
	toy_destroy_asset_pool(&alc->buddy_alc, &asset_mgr->asset_pools.image);
	toy_destroy_asset_pool(&alc->buddy_alc, &asset_mgr->asset_pools.mesh);
	toy_destroy_asset_pool(&alc->buddy_alc, &asset_mgr->asset_pools.mesh_primitive);

	toy_destroy_vulkan_mesh_primitive_asset_pool(&asset_mgr->vk_private.vk_mesh_primitive_pool);
	toy_destroy_vulkan_asset_loader(
		asset_mgr->vk_private.vk_driver->device.handle,
		&asset_mgr->vk_private.vk_asset_loader);

	toy_destroy_asset_ref_pool(&asset_mgr->item_ref_pool);
	toy_destroy_memory_pools(asset_mgr->chunk_pools, asset_mgr->alc);
	toy_destroy_memory_stack(asset_mgr->cache_stack, asset_mgr->alc);
}



static uint32_t alloc_mesh_primitive_item (
	toy_asset_manager_t* asset_mgr,
	const toy_host_mesh_primitive_t* primitive_data,
	toy_error_t* error)
{
	// Alloc asset item
	uint32_t primitive_index = toy_alloc_asset_item(
		&asset_mgr->asset_pools.mesh_primitive,
		error);
	if (toy_is_failed(*error))
		goto FAIL_ALLOC_REF;

	toy_vulkan_mesh_primitive_t* vk_primitive = toy_get_asset_item(
		&asset_mgr->asset_pools.mesh_primitive,
		primitive_index);

	toy_alloc_vulkan_mesh_primitive(
		&asset_mgr->vk_private.vk_mesh_primitive_pool,
		primitive_data->attribute_size / primitive_data->vertex_count,
		primitive_data->vertex_count,
		primitive_data->index_count,
		vk_primitive,
		error);
	if (toy_is_failed(*error))
		goto FAIL_ALLOC_MESH_PRIMITIVE;

	toy_ok(error);
	return primitive_index;

FAIL_ALLOC_MESH_PRIMITIVE:
	toy_raw_free_asset_item(&asset_mgr->asset_pools.mesh_primitive, primitive_index);
FAIL_ALLOC_REF:
	return UINT32_MAX;
}


uint32_t toy_load_mesh_primitive (
	toy_asset_manager_t* asset_mgr,
	const toy_host_mesh_primitive_t* primitive_data,
	toy_error_t* error)
{
	toy_asset_manager_vulkan_private_t* vk_private = &asset_mgr->vk_private;
	VkResult vk_err;

	// Alloc asset item
	uint32_t primitive_index = alloc_mesh_primitive_item(asset_mgr, primitive_data, error);
	if (toy_is_failed(*error))
		goto FAIL_ALLOC_ITEM;

	toy_vulkan_mesh_primitive_t* vk_primitive = toy_get_asset_item(&asset_mgr->asset_pools.mesh_primitive, primitive_index);
	TOY_ASSERT(NULL != vk_primitive);

	toy_reset_vulkan_asset_loader(
		vk_private->vk_driver->device.handle,
		&vk_private->vk_asset_loader,
		error);
	if (toy_is_failed(*error))
		goto FAIL_RESET_LOADER;

	// Copy data to stage memory
	toy_stage_data_block_t data_blocks[2];
	toy_vulkan_sub_buffer_t stage_sub_buffers[2];
	data_blocks[0].data = primitive_data->attributes;
	data_blocks[0].size = primitive_data->attribute_size;
	data_blocks[0].alignment = primitive_data->attribute_size / primitive_data->vertex_count;
	data_blocks[1].data = primitive_data->indices;
	data_blocks[1].size = primitive_data->index_size;
	data_blocks[1].alignment = primitive_data->index_count > UINT16_MAX ? sizeof(uint32_t) : sizeof(uint16_t);

	toy_map_vulkan_stage_memory(
		vk_private->vk_driver->device.handle,
		&vk_private->vk_asset_loader,
		error);
	if (toy_is_failed(*error))
		goto FAIL_MAP_STAGE_MEMORY;

	toy_copy_data_to_vulkan_stage_memory(
		data_blocks,
		NULL != primitive_data->indices ? 2 : 1,
		&vk_private->vk_asset_loader,
		stage_sub_buffers,
		error);
	if (toy_is_failed(*error))
		goto FAIL_COPY_TO_STAGE_MEMORY;

	toy_unmap_vulkan_stage_memory(
		vk_private->vk_driver->device.handle,
		&vk_private->vk_asset_loader,
		error);
	if (toy_is_failed(*error))
		goto FAIL_UNMAP_STAGE_MEMORY;

	// Copy stage memory to gpu local memory
	vk_err = toy_start_vkcmd_stage_mesh_primitive(
		&vk_private->vk_asset_loader);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "Failed to record vkcmd of loading mesh primitive", error);
		goto FAIL_START_CMD;
	}

	toy_vkcmd_copy_mesh_primitive_data(
		vk_private->vk_asset_loader.transfer_cmd,
		&vk_private->vk_mesh_primitive_pool,
		&stage_sub_buffers[0],
		NULL != primitive_data->indices ? &stage_sub_buffers[1] : NULL,
		vk_primitive);

	// Post process commands, for mesh primitive, it's nothing

	toy_submit_vkcmd_stage_mesh_primitive(
		vk_private->vk_driver->device.handle,
		&vk_private->vk_asset_loader,
		error);
	if (toy_is_failed(*error))
		goto FAIL_SUBMIT_CMD;

	vk_err = vkWaitForFences(
		vk_private->vk_driver->device.handle,
		1,
		&vk_private->vk_asset_loader.fence,
		VK_TRUE,
		UINT64_MAX);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "Wait for asset loading failed", error);
		goto FAIL_WAIT_SUBMIT;
	}

	toy_ok(error);
	return primitive_index;

FAIL_WAIT_SUBMIT:
FAIL_SUBMIT_CMD:
FAIL_START_CMD:
FAIL_UNMAP_STAGE_MEMORY:
	toy_clear_vulkan_stage_memory(&vk_private->vk_asset_loader);
FAIL_COPY_TO_STAGE_MEMORY:
FAIL_MAP_STAGE_MEMORY:
FAIL_RESET_LOADER:
FAIL_ALLOC_ITEM:
	toy_free_asset_item(&asset_mgr->asset_pools.mesh_primitive, primitive_index);
	return UINT32_MAX;
}


void toy_load_texture2d (
	toy_asset_manager_t* asset_mgr,
	const char* utf8_path,
	toy_asset_pool_item_ref_t* output,
	toy_error_t* error)
{
	toy_vulkan_asset_loader_t* vk_asset_loader = &asset_mgr->vk_private.vk_asset_loader;

	size_t size_read;
	void* file_content = toy_load_whole_file(utf8_path, &asset_mgr->file_api, &asset_mgr->stack_alc_L, &asset_mgr->stack_alc_R, &size_read, error);
	if (toy_is_failed(*error))
		goto FAIL_LOAD_FILE;

	int image_width, image_height, image_component_num;
	stbi_uc* pixels = stbi_load_from_memory((stbi_uc*)file_content, (int)size_read, &image_width, &image_height, &image_component_num, STBI_rgb_alpha);
	toy_free_aligned(&asset_mgr->stack_alc_L, file_content);
	if (NULL == pixels) {
		toy_err(TOY_ERROR_OPERATION_FAILED, "Decode texture failed", error);
		goto FAIL_DECODE;
	}

	size_t image_size = image_width * image_height * sizeof(uint32_t);

	uint32_t image_index = toy_alloc_asset_item(&asset_mgr->asset_pools.image, error);
	if (toy_is_failed(*error))
		goto FAIL_ALLOC_ITEM;

	toy_vulkan_image_t* vk_image = toy_get_asset_item(&asset_mgr->asset_pools.image, image_index);
	TOY_ASSERT(NULL != vk_image);

	output->pool = &asset_mgr->asset_pools.image;
	output->index = image_index;
	output->next_ref = UINT32_MAX;

	toy_create_vulkan_image_texture2d(
		asset_mgr->vk_private.vk_asset_loader.vk_alc,
		VK_FORMAT_R8G8B8A8_UNORM,
		image_width, image_height, 1,
		vk_image,
		error);
	if (toy_is_failed(*error))
		goto FAIL_CREATE_IMAGE;

	toy_reset_vulkan_asset_loader(
		vk_asset_loader->vk_alc->device,
		vk_asset_loader,
		error);
	if (toy_is_failed(*error))
		goto FAIL_RESET_LOADER;

	// Copy data to stage memory
	toy_stage_data_block_t data_block;
	toy_vulkan_sub_buffer_t stage_sub_buffer;
	data_block.data = pixels;
	data_block.size = image_size;
	data_block.alignment = sizeof(uint32_t);

	toy_map_vulkan_stage_memory(
		vk_asset_loader->vk_alc->device,
		vk_asset_loader,
		error);
	if (toy_is_failed(*error))
		goto FAIL_MAP_STAGE_MEMORY;

	toy_copy_data_to_vulkan_stage_memory(
		&data_block, 1,
		vk_asset_loader,
		&stage_sub_buffer,
		error);
	if (toy_is_failed(*error))
		goto FAIL_COPY_TO_STAGE_MEMORY;

	toy_unmap_vulkan_stage_memory(
		vk_asset_loader->vk_alc->device,
		vk_asset_loader,
		error);
	if (toy_is_failed(*error))
		goto FAIL_UNMAP_STAGE_MEMORY;

	stbi_image_free(pixels);
	pixels = NULL;

	// Copy stage memory to gpu local memory
	VkResult vk_err = toy_start_vkcmd_stage_image(vk_asset_loader);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "Failed to record vkcmd of staging image", error);
		goto FAIL_START_CMD;
	}

	toy_vkcmd_stage_texture_image(
		vk_asset_loader, &stage_sub_buffer, vk_image, image_width, image_height, 1);

	toy_submit_vkcmd_stage_image(
		vk_asset_loader->vk_alc->device, vk_asset_loader, error);
	if (toy_is_failed(*error))
		goto FAIL_SUBMIT_CMD;

	vk_err = vkWaitForFences(
		vk_asset_loader->vk_alc->device,
		1,
		&vk_asset_loader->fence,
		VK_TRUE,
		UINT64_MAX);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "Wait for asset loading failed", error);
		goto FAIL_WAIT_SUBMIT;
	}

	toy_ok(error);
	return;

FAIL_WAIT_SUBMIT:
FAIL_SUBMIT_CMD:
FAIL_START_CMD:
	toy_clear_vulkan_stage_memory(vk_asset_loader);
FAIL_UNMAP_STAGE_MEMORY:
FAIL_COPY_TO_STAGE_MEMORY:
FAIL_MAP_STAGE_MEMORY:
FAIL_RESET_LOADER:
	toy_destroy_vulkan_image(vk_asset_loader->vk_alc->device, vk_image, vk_asset_loader->vk_alc->vk_alc_cb_p);
FAIL_CREATE_IMAGE:
	toy_raw_free_asset_item(&asset_mgr->asset_pools.image, image_index);
FAIL_ALLOC_ITEM:
	if (NULL != pixels)
		stbi_image_free(pixels);
FAIL_DECODE:
FAIL_LOAD_FILE:
	toy_log_error(error);
	return;
}


uint32_t toy_alloc_material (
	toy_asset_manager_t* asset_mgr,
	size_t size,
	toy_error_t* error)
{
	uint32_t index = toy_alloc_asset_item(&asset_mgr->asset_pools.material, error);
	if (toy_is_failed(*error))
		return UINT32_MAX;

	void* data = toy_alloc_aligned(&asset_mgr->alc->list_alc, size, sizeof(void*));
	if (NULL == data) {
		toy_raw_free_asset_item(&asset_mgr->asset_pools.material, index);
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "Alloc material failed", error);
		return UINT32_MAX;
	}

	void** ptr = toy_get_asset_item(&asset_mgr->asset_pools.material, index);
	*ptr = data;
	toy_ok(error);
	return index;
}


toy_asset_pool_item_ref_t toy_create_image_sampler (
	toy_asset_manager_t* asset_mgr,
	toy_image_sampler_t* sampler_params,
	toy_error_t* error)
{
	toy_asset_pool_item_ref_t ref;
	ref.pool = NULL;
	ref.index = UINT32_MAX;
	ref.next_ref = UINT32_MAX;

	uint32_t index = toy_alloc_asset_item(&asset_mgr->asset_pools.image_sampler, error);
	if (toy_is_failed(*error))
		return ref;
	
	toy_vulkan_sampler_t* vk_sampler = toy_get_asset_item(&asset_mgr->asset_pools.image_sampler, index);
	TOY_ASSERT(NULL != vk_sampler);

	vk_sampler->params = *sampler_params;
	VkResult vk_err = toy_create_vulkan_image_sampler(
		asset_mgr->vk_private.vk_driver->device.handle,
		sampler_params,
		asset_mgr->vk_private.vk_driver->vk_alc_cb_p,
		&vk_sampler->handle);
	if (VK_SUCCESS != vk_err) {
		toy_err(TOY_ERROR_CREATE_OBJECT_FAILED, "toy_create_vulkan_image_sampler failed", error);
		return ref;
	}

	ref.pool = &asset_mgr->asset_pools.image_sampler;
	ref.index = index;

	toy_ok(error);
	return ref;
}
