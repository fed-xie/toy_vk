#pragma once

#include "../toy_platform.h"
#include "../toy_asset.h"
#include "../toy_asset_manager.h"


TOY_EXTERN_C_START

const toy_host_mesh_primitive_t* toy_get_built_in_mesh_rectangle();

uint32_t toy_load_built_in_mesh (
	toy_asset_manager_t* asset_mgr,
	const toy_host_mesh_primitive_t* primitive,
	toy_error_t* error
);

TOY_EXTERN_C_END
