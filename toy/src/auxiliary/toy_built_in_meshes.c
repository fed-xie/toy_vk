#include "../include/auxiliary/toy_built_in_meshes.h"

#include "../toy_assert.h"

// Vulkan depth is 0.0~1.0, less value cover bigger value by default
static toy_built_in_vertex_t s_rectangle_vertices[] = {
		{-0.5f,  0.5f, 0.0f,  0.0f, 0.0f,  0.0f, 0.0f, 1.0f, },
		{-0.5f, -0.5f, 0.0f,  0.0f, 1.0f,  0.0f, 0.0f, 1.0f, },
		{ 0.5f,  0.5f, 0.0f,  1.0f, 0.0f,  0.0f, 0.0f, 1.0f, },
		{ 0.5f, -0.5f, 0.0f,  1.0f, 1.0f,  0.0f, 0.0f, 1.0f, },
};
static uint16_t s_rectangle_vertex_indices[] = {
	0, 1, 2, 2, 1, 3,
};
const static toy_host_mesh_primitive_t s_rectangle_host_mesh_primitive = {
	.attributes = s_rectangle_vertices,
	.attribute_size = sizeof(s_rectangle_vertices),
	.indices = s_rectangle_vertex_indices,
	.index_size = sizeof(s_rectangle_vertex_indices),
	.vertex_count = sizeof(s_rectangle_vertices) / sizeof(*s_rectangle_vertices),
	.index_count = sizeof(s_rectangle_vertex_indices) / sizeof(*s_rectangle_vertex_indices),
};

const toy_host_mesh_primitive_t* toy_get_built_in_mesh_rectangle()
{
	return &s_rectangle_host_mesh_primitive;
}


void toy_load_built_in_mesh (
	toy_asset_manager_t* asset_mgr,
	const toy_host_mesh_primitive_t* primitive,
	toy_asset_pool_item_ref_t* output,
	toy_error_t* error)
{
	uint32_t mesh_index = toy_alloc_asset_item(&asset_mgr->asset_pools.mesh, error);
	if (UINT32_MAX == mesh_index)
		goto FAIL_ALLOC_MESH;

	toy_mesh_t* mesh = toy_get_asset_item(&asset_mgr->asset_pools.mesh, mesh_index);
	TOY_ASSERT(NULL != mesh);

	toy_load_mesh_primitive(
		asset_mgr,
		primitive,
		&mesh->first_primitive,
		error);
	if (toy_is_failed(*error))
		goto FAIL_LOAD_PRIMITIVE;
	
	mesh->ref_pool = &asset_mgr->item_ref_pool;
	toy_add_asset_ref(mesh->first_primitive.pool, mesh->first_primitive.index, 1);
	mesh->primitive_count = 1;

	output->pool = &asset_mgr->asset_pools.mesh;
	output->index = mesh_index;
	output->next_ref = UINT32_MAX;
	toy_ok(error);
	return;

FAIL_LOAD_PRIMITIVE:
	toy_raw_free_asset_item(&asset_mgr->asset_pools.mesh, mesh_index);
FAIL_ALLOC_MESH:
	return;
}
