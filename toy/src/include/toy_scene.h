#pragma once

#include "toy_platform.h"
#include "toy_asset.h"
#include "toy_memory.h"
#include "scene/toy_scene_component.h"
#include "scene/toy_scene_camera.h"

#include <stdint.h>



typedef struct toy_scene_t {
	struct toy_scene_t* prev;
	struct toy_scene_t* next;
	uint32_t scene_id;

	toy_fmat4x4_t inst_matrices[128];
	uint32_t mesh_primitives[128];
	uint32_t object_ids[128];
	uint32_t object_count;

	toy_scene_camera_t main_camera;

	toy_memory_allocator_t* alc;
}toy_scene_t;


typedef toy_scene_entity_chunk_descriptor_t* (*toy_create_scene_entity_chunk_descriptor_fp) (const toy_allocator_t* alc);


TOY_EXTERN_C_START

toy_scene_t* toy_create_scene (
	toy_memory_allocator_t* alc,
	toy_error_t* error
);

void toy_destroy_scene (
	toy_scene_t* scene
);

TOY_EXTERN_C_END
