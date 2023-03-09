#include "../include/auxiliary/toy_built_in_meshes.h"

#include "../toy_assert.h"
#include <stddef.h>

// Vulkan depth is 0.0~1.0, less value cover bigger value by default

// Aligned as vec4 to compatible with GLSL declaration
struct toy_built_in_mesh_vertex_t {
	float position[3];
	float normal[3];
	float texcoord[2];
};
static const struct toy_vertex_attribute_slot_descriptor_t s_built_in_mesh_attr_slots[] = {
	{
		.slot = TOY_VERTEX_ATTRIBUTE_SLOT_POSITION,
		.stride = sizeof(float) * 3,
		.offset = offsetof(struct toy_built_in_mesh_vertex_t, position),
	},{
		.slot = TOY_VERTEX_ATTRIBUTE_SLOT_NORMAL,
		.stride = sizeof(float) * 3,
		.offset = offsetof(struct toy_built_in_mesh_vertex_t, normal),
	},{
		.slot = TOY_VERTEX_ATTRIBUTE_SLOT_TEXCOORD,
		.stride = sizeof(float) * 2,
		.offset = offsetof(struct toy_built_in_mesh_vertex_t, texcoord),
	},
};
static const toy_vertex_attribute_descriptor_t s_built_in_meh_attr_desc = {
	.slot_count = sizeof(s_built_in_mesh_attr_slots) / sizeof(*s_built_in_mesh_attr_slots),
	.slot_descs = s_built_in_mesh_attr_slots,
};

static const struct toy_built_in_mesh_vertex_t s_attr_rectangle[] = {
	{
		.position = {-0.5f, 0.5f, 0.0f,},
		.normal = {0.0f, 0.0f, 1.0f,},
		.texcoord = {0.0f, 0.0f,},
	}, {
		.position = {-0.5f, -0.5f, 0.0f,},
		.normal = {0.0f, 0.0f, 1.0f,},
		.texcoord = {0.0f, 1.0f,},
	}, {
		.position = {0.5f, 0.5f, 0.0f,},
		.normal = {0.0f, 0.0f, 1.0f,},
		.texcoord = {1.0f, 0.0f,},
	}, {
		.position = {0.5f, -0.5f, 0.0f,},
		.normal = {0.0f, 0.0f, 1.0f,},
		.texcoord = {1.0f, 1.0f,},
	},
};
static const uint16_t s_index_rectangle[] = {
	0, 1, 2, 2, 1, 3,
};
static const toy_host_mesh_primitive_t s_mesh_primitive_rectangle = {
	.attributes = s_attr_rectangle,
	.attribute_size = sizeof(s_attr_rectangle),
	.attr_desc = &s_built_in_meh_attr_desc,
	.indices = s_index_rectangle,
	.index_size = sizeof(s_index_rectangle),
	.vertex_count = sizeof(s_attr_rectangle) / sizeof(*s_attr_rectangle),
	.index_count = sizeof(s_index_rectangle) / sizeof(*s_index_rectangle),
};

const toy_host_mesh_primitive_t* toy_get_built_in_mesh_rectangle()
{
	return &s_mesh_primitive_rectangle;
}


uint32_t toy_load_built_in_mesh (
	toy_asset_manager_t* asset_mgr,
	const toy_host_mesh_primitive_t* primitive,
	toy_error_t* error)
{
	uint32_t mesh_index = toy_alloc_asset_item(&asset_mgr->asset_pools.mesh, error);
	if (UINT32_MAX == mesh_index)
		goto FAIL_ALLOC_MESH;

	toy_mesh_t* mesh = toy_get_asset_item(&asset_mgr->asset_pools.mesh, mesh_index);
	TOY_ASSERT(NULL != mesh);

	mesh->primitive_index = toy_load_mesh_primitive(
		asset_mgr, primitive, error);
	if (toy_is_failed(*error))
		goto FAIL_LOAD_PRIMITIVE;
	
	toy_add_asset_ref(&asset_mgr->asset_pools.mesh_primitive, mesh->primitive_index, 1);
	mesh->material_index = UINT32_MAX;

	toy_ok(error);
	return mesh_index;

FAIL_LOAD_PRIMITIVE:
	toy_raw_free_asset_item(&asset_mgr->asset_pools.mesh, mesh_index);
FAIL_ALLOC_MESH:
	return UINT32_MAX;
}
