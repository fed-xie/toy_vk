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

	memset(scene->object_ids, 0, sizeof(scene->object_ids));
	memset(scene->meshes, 0, sizeof(scene->meshes));
	memset(scene->inst_matrices, 0, sizeof(scene->inst_matrices));

	scene->alc = alc;

	return scene;

FAIL_ALLOC:
	return NULL;
}


void toy_destroy_scene (
	toy_scene_t* scene)
{
	toy_free_aligned(&scene->alc->list_alc, (toy_aligned_p)scene);
}

TOY_EXTERN_C_END
