#pragma once

#include "../toy_platform.h"

#include "../platform/vulkan/toy_vulkan.h"
#include "../toy_memory.h"
#include "../toy_scene.h"
#include "../toy_asset_manager.h"

typedef struct toy_built_in_pipeline_t* toy_built_in_pipeline_p;

TOY_EXTERN_C_START

toy_built_in_pipeline_p toy_create_built_in_vulkan_pipeline (
	toy_vulkan_driver_t* vk_driver,
	toy_memory_allocator_t* alc,
	toy_error_t* error
);

void toy_destroy_built_in_vulkan_pipeline (
	toy_vulkan_driver_t* vk_driver,
	toy_memory_allocator_t* alc,
	toy_built_in_pipeline_p pipeline
);

void toy_draw_scene (
	toy_vulkan_driver_t* vk_driver,
	toy_scene_t* scene,
	toy_asset_manager_t* asset_mgr,
	toy_built_in_pipeline_p pipeline,
	toy_error_t* error
);

TOY_EXTERN_C_END
