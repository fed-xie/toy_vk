#include "../include/scene/toy_scene_component.h"

#include "../toy_assert.h"

toy_entity_ref_t toy_create_scene_chunk_entity (
	toy_scene_entity_chunk_descriptor_t* chunk_desc,
	const toy_allocator_t* chunk_alc,
	uint32_t object_id)
{
	toy_entity_ref_t ref;
	ref.chunk = NULL;
	ref.index = UINT16_MAX;

	toy_scene_entity_chunk_header_t* chunk = chunk_desc->first_chunk;
	while (NULL != chunk) {
		if (chunk->entity_count < chunk_desc->max_entity_count)
			break;
		chunk = chunk->next;
	}
	
	if (NULL == chunk) {
		chunk = chunk_desc->create_chunk(chunk_desc, chunk_alc);
		if (NULL == chunk)
			return ref;
	}

	chunk_desc->create_entity(chunk, chunk->entity_count, object_id);
	uint16_t entity_index = chunk->entity_count++;
	TOY_ASSERT(entity_index < chunk_desc->max_entity_count);

	ref.chunk = chunk;
	ref.index = entity_index;
	return ref;
}
