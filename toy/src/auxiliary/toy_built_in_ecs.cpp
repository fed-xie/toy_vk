#include "../include/auxiliary/toy_built_in_ecs.h"

#include "../include/toy_math_type.h"
#include "../include/toy_asset.h"
#include "../include/toy_math.hpp"
#include "../toy_assert.h"


struct _toy_mesh_only_entity_t {
	toy_fmat4x4_t location;
	toy_asset_pool_item_ref_t mesh_ref;
	uint32_t object_id;
};
static constexpr uint32_t MAX_MESH_COUNT = ((TOY_MEMORY_CHUNK_SIZE - sizeof(toy_scene_entity_chunk_header_t)) / sizeof(_toy_mesh_only_entity_t));

struct toy_scene_entity_chunk_mesh_only_t {
	toy_scene_entity_chunk_header_t header;
	toy_fmat4x4_t location[MAX_MESH_COUNT];
	uint32_t object_id[MAX_MESH_COUNT];
	toy_asset_pool_item_ref_t mesh_refs[MAX_MESH_COUNT];
};

static_assert(sizeof(struct toy_scene_entity_chunk_mesh_only_t) <= TOY_MEMORY_CHUNK_SIZE, "Size of toy_scene_entity_chunk_mesh_only_t error");

TOY_EXTERN_C_START

static ptrdiff_t toy_get_mesh_only_entity_chunk_component_offset (
	toy_scene_entity_chunk_descriptor_t* chunk_desc,
	enum toy_scene_component_type_t type)
{
	switch (type) {
	case TOY_SCENE_COMPONENT_TYPE_OBJECT_ID:
		return offsetof(struct toy_scene_entity_chunk_mesh_only_t, object_id);
	case TOY_SCENE_COMPONENT_TYPE_MESH:
		return offsetof(struct toy_scene_entity_chunk_mesh_only_t, mesh_refs);
	case TOY_SCENE_COMPONENT_TYPE_LOCATION:
		return offsetof(struct toy_scene_entity_chunk_mesh_only_t, location);
	default:
		return 0;
	}
}


static void toy_create_scene_mesh_only_entity (
	toy_scene_entity_chunk_header_t* chunk,
	uint16_t index,
	uint32_t object_id)
{
	auto mesh_chunk = reinterpret_cast<toy_scene_entity_chunk_mesh_only_t*>(chunk);
	toy_scene_entity_chunk_descriptor_t* chunk_desc = chunk->chunk_desc;
	TOY_ASSERT(chunk_desc->max_entity_count > mesh_chunk->header.entity_count);
	mesh_chunk->object_id[index] = object_id;
}


static uint32_t toy_destroy_mesh_only_entity (
	toy_scene_entity_chunk_header_t* chunk,
	uint16_t index)
{
	auto mesh_chunk = reinterpret_cast<toy_scene_entity_chunk_mesh_only_t*>(chunk);

	toy_asset_pool_p asset_pool = mesh_chunk->mesh_refs[index].pool;
	uint32_t asset_index = mesh_chunk->mesh_refs[index].index;
	toy_sub_asset_ref(asset_pool, asset_index, 1);
	if (0 == toy_get_asset_ref(asset_pool, asset_index))
		toy_free_asset_item(asset_pool, asset_index);
	return mesh_chunk->object_id[index];
}


static uint32_t toy_move_mesh_only_entity (
	toy_scene_entity_chunk_header_t* chunk,
	uint16_t src_index,
	uint16_t dst_index)
{
	TOY_ASSERT(src_index < chunk->chunk_desc->max_entity_count && dst_index < chunk->chunk_desc->max_entity_count);
	auto mesh_chunk = reinterpret_cast<toy_scene_entity_chunk_mesh_only_t*>(chunk);

	mesh_chunk->mesh_refs[dst_index] = mesh_chunk->mesh_refs[src_index];
	mesh_chunk->location[dst_index] = mesh_chunk->location[src_index];
	mesh_chunk->object_id[dst_index] = mesh_chunk->object_id[src_index];
	return mesh_chunk->object_id[dst_index];
}


static toy_scene_entity_chunk_header_t* toy_create_mesh_only_entity_chunk (
	toy_scene_entity_chunk_descriptor_t* chunk_desc,
	const toy_allocator_t* chunk_alc)
{
	auto chunk = (struct toy_scene_entity_chunk_mesh_only_t*)toy_alloc(
		chunk_alc, TOY_MEMORY_CHUNK_SIZE);
	if (NULL == chunk)
		return NULL;

	chunk->header.chunk_desc = chunk_desc;
	chunk->header.entity_count = 0;
	chunk->header.next = chunk_desc->first_chunk;
	chunk->header.prev = NULL;

	for (uint32_t i = 0; i < MAX_MESH_COUNT; ++i) {
		chunk->object_id[i] = UINT32_MAX;
		chunk->location[i] = toy::identity_matrix();
		chunk->mesh_refs[i].pool = NULL;
		chunk->mesh_refs[i].index = UINT32_MAX;
		chunk->mesh_refs[i].next_ref = UINT32_MAX;
	}

	toy_scene_entity_chunk_header_t* header = &chunk->header;
	if (NULL != chunk_desc->first_chunk)
		chunk_desc->first_chunk->prev = header;
	chunk_desc->first_chunk = header;
	++chunk_desc->chunk_count;

	return header;
}


static void toy_destroy_mesh_only_entity_chunk (
	toy_scene_entity_chunk_header_t* chunk,
	const toy_allocator_t* chunk_alc)
{
	auto mesh_chunk = reinterpret_cast<toy_scene_entity_chunk_mesh_only_t*>(chunk);
	toy_scene_entity_chunk_descriptor_t* chunk_desc = chunk->chunk_desc;
	
	for (uint32_t i = 0; i < mesh_chunk->header.entity_count; ++i) {
		toy_sub_asset_ref(mesh_chunk->mesh_refs[i].pool, mesh_chunk->mesh_refs[i].index, 1);
		if (0 == toy_get_asset_ref(mesh_chunk->mesh_refs[i].pool, mesh_chunk->mesh_refs[i].index))
			toy_free_asset_item(mesh_chunk->mesh_refs[i].pool, mesh_chunk->mesh_refs[i].index);
	}

	if (NULL != mesh_chunk->header.prev)
		mesh_chunk->header.prev->next = mesh_chunk->header.next;
	if (NULL != mesh_chunk->header.next)
		mesh_chunk->header.next->prev = mesh_chunk->header.prev;
	if (chunk_desc->first_chunk == &mesh_chunk->header)
		chunk_desc->first_chunk = mesh_chunk->header.next;
	--chunk_desc->chunk_count;

	toy_free(chunk_alc, chunk);
}


toy_scene_entity_chunk_descriptor_t* toy_create_entity_chunk_descriptor_mesh_only (
	const toy_allocator_t* alc)
{
	auto desc = reinterpret_cast<toy_scene_entity_chunk_descriptor_t*>(toy_alloc_aligned(alc, sizeof(toy_scene_entity_chunk_descriptor_t), sizeof(void*)));
	if (NULL == desc)
		return NULL;

	desc->next = NULL;
	desc->context = NULL;
	desc->max_entity_count = MAX_MESH_COUNT;
	desc->get_comp_offset = toy_get_mesh_only_entity_chunk_component_offset;
	desc->create_chunk = toy_create_mesh_only_entity_chunk;
	desc->destroy_chunk = toy_destroy_mesh_only_entity_chunk;
	desc->create_entity = toy_create_scene_mesh_only_entity;
	desc->destroy_entity = toy_destroy_mesh_only_entity;
	desc->move_entity = toy_move_mesh_only_entity;
	desc->first_chunk = NULL;
	return desc;
}

TOY_EXTERN_C_END
