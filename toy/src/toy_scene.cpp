#include "include/toy_scene.h"

#include "toy_assert.h"
#include <cstring>
#include <cerrno>

TOY_EXTERN_C_START

toy_scene_t* toy_create_scene (
	toy_memory_allocator_t* alc,
	toy_error_t* error)
{
	toy_scene_t* scene = reinterpret_cast<toy_scene_t*>(toy_alloc_aligned(&alc->list_alc, sizeof(toy_scene_t), sizeof(void*)));
	if (NULL == scene) {
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "Alloc scene failed", error);
		goto FAIL_ALLOC;
	}

	scene->prev = NULL;
	scene->next = NULL;
	scene->scene_id = 0;

	toy_init_asset_pool(
		sizeof(toy_entity_ref_t),
		sizeof(void*),
		NULL,
		&alc->chunk_pool_alc,
		&alc->list_alc,
		NULL,
		"Scene entity refs",
		&scene->object_refs);

	scene->chunk_descs = NULL;
	scene->cameras = NULL;
	scene->camera_count = 0;
	//scene->meshes;
	scene->alc = alc;

	return scene;

FAIL_ALLOC:
	return NULL;
}


void toy_destroy_scene (
	toy_scene_t* scene)
{
	toy_scene_entity_chunk_descriptor_t* chunk_desc = scene->chunk_descs;
	while (NULL != chunk_desc) {
		toy_scene_entity_chunk_descriptor_t* next_desc = chunk_desc->next;
		while (NULL != chunk_desc->first_chunk)
			chunk_desc->destroy_chunk(chunk_desc->first_chunk, &scene->alc->chunk_pool_alc);
		toy_free_aligned(&scene->alc->list_alc, chunk_desc);
		chunk_desc = next_desc;
	}

	toy_free_aligned(&scene->alc->list_alc, scene->cameras);

	toy_destroy_asset_pool(&scene->alc->list_alc, &scene->object_refs);

	toy_free_aligned(&scene->alc->list_alc, (toy_aligned_p)scene);
}


toy_scene_entity_chunk_descriptor_t* toy_add_scene_entity_descriptor (
	toy_scene_t* scene,
	toy_create_scene_entity_chunk_descriptor_fp create_desc_fp)
{
	TOY_ASSERT(NULL != scene && NULL != create_desc_fp);

	auto desc = create_desc_fp(&scene->alc->list_alc);
	desc->next = scene->chunk_descs;
	scene->chunk_descs = desc;
	return desc;
}


uint32_t toy_create_scene_entity (
	toy_scene_t* scene,
	toy_scene_entity_chunk_descriptor_t* chunk_desc)
{
	toy_error_t err;
	uint32_t object_id = toy_alloc_asset_item(&scene->object_refs, &err);
	if (toy_is_failed(err))
		return UINT32_MAX;

	toy_entity_ref_t* ref = reinterpret_cast<toy_entity_ref_t*>(toy_get_asset_item(&scene->object_refs, object_id));
	TOY_ASSERT(NULL != ref);
	*ref = toy_create_scene_chunk_entity(chunk_desc, &scene->alc->chunk_pool_alc, object_id);
	return object_id;
}


void toy_destroy_scene_entity (toy_scene_t* scene, toy_entity_ref_t* ref)
{
	toy_scene_entity_chunk_header_t* chunk = ref->chunk;
	uint16_t index = ref->index;
	TOY_ASSERT(index < chunk->entity_count);

	uint32_t object_id = chunk->chunk_desc->destroy_entity(chunk, index);
	toy_free_asset_item(&scene->object_refs, object_id);

	// Move last entity in chunk to fill the hole where the entity destroyed
	if (index != chunk->entity_count - 1) {
		object_id = chunk->chunk_desc->move_entity(chunk, chunk->entity_count - 1, index);
		toy_entity_ref_t* move_ref = reinterpret_cast<toy_entity_ref_t*>(toy_get_asset_item(&scene->object_refs, object_id));
		move_ref->index = index;
	}

	--chunk->entity_count;
}


uint32_t toy_new_scene_camera (toy_scene_t* scene)
{
	if (NULL == scene->cameras) {
		TOY_ASSERT(0 == scene->camera_count);
		scene->cameras = reinterpret_cast<toy_scene_camera_t*>(toy_alloc_aligned(
			&scene->alc->list_alc, sizeof(toy_scene_camera_t), sizeof(toy_fmat4x4_t)));
		if (NULL == scene->cameras) {
			return UINT32_MAX;
		}
		scene->camera_count = 1;
		return 0;
	}

	size_t new_arr_size = sizeof(toy_scene_camera_t) * (scene->camera_count + 1);
	toy_scene_camera_t* cameras = reinterpret_cast<toy_scene_camera_t*>(toy_alloc_aligned(
		&scene->alc->list_alc, new_arr_size, sizeof(toy_fmat4x4_t)));
	if (NULL == cameras)
		return UINT32_MAX;

	errno_t err_no = memcpy_s(cameras, new_arr_size, scene->cameras, sizeof(toy_scene_camera_t) * scene->camera_count);
	if (0 != err_no)
		return UINT32_MAX;

	return scene->camera_count++;
}


TOY_EXTERN_C_END
