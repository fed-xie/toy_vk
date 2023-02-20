#pragma once

#include "../toy_platform.h"
#include "../scene/toy_scene_component.h"


TOY_EXTERN_C_START

toy_scene_entity_chunk_descriptor_t* toy_create_entity_chunk_descriptor_mesh_only (
	const toy_allocator_t* alc
);

TOY_EXTERN_C_END
