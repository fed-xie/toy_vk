#pragma once

#include "../../include/auxiliary/render_pass/pass_base.h"

TOY_EXTERN_C_START

uint32_t toy_alloc_vulkan_descriptor_set_single_texture (
	toy_asset_manager_t* asset_mgr,
	const toy_vulkan_descriptor_set_layout_t* desc_set_layout,
	toy_error_t* error
);

toy_built_in_shader_module_t toy_get_shader_module_mesh();

TOY_EXTERN_C_END
