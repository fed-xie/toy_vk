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

	toy_asset_pool_t object_refs;

	toy_scene_entity_chunk_descriptor_t* chunk_descs;

	toy_scene_camera_t* cameras;
	uint32_t camera_count;

	// private
	//toy_asset_item_ref_pool_t meshes;

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

toy_scene_entity_chunk_descriptor_t* toy_add_scene_entity_descriptor (
	toy_scene_t* scene,
	toy_create_scene_entity_chunk_descriptor_fp create_desc_fp
);

uint32_t toy_create_scene_entity (
	toy_scene_t* scene,
	toy_scene_entity_chunk_descriptor_t* chunk_desc
);

void toy_destroy_scene_entity (toy_scene_t* scene, toy_entity_ref_t* ref);

toy_inline void toy_destroy_scene_entity2 (toy_scene_t* scene, uint32_t object_id) {
	toy_destroy_scene_entity(scene, (toy_entity_ref_t*)toy_get_asset_item(&scene->object_refs, object_id));
}

uint32_t toy_new_scene_camera (toy_scene_t* scene);

TOY_EXTERN_C_END
