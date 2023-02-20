#include "../include/toy.h"

#include "../include/auxiliary/toy_built_in_meshes.h"
#include "../include/auxiliary/toy_built_in_ecs.h"
#include "../include/auxiliary/toy_built_in_pipeline.h"

#include <cassert>

using toy::operator*;
using toy::operator+;
using toy::operator-;
using toy::operator/;

struct game_data_t {
	uint32_t camera_id;
	uint32_t mesh_ids[2];
};

void load_scene (toy_app_t* app, game_data_t* gd)
{
	toy_error_t err;

	toy_scene_t* scene = toy_create_scene(app->alc, &err);
	assert(NULL != scene);
	toy_push_scene(app, scene);

	toy_asset_pool_item_ref_t mesh_ref;
	toy_load_built_in_mesh(
		&app->asset_mgr,
		toy_get_built_in_mesh_rectangle(),
		&mesh_ref,
		&err);
	auto mesh = reinterpret_cast<toy_mesh_t*>(toy_get_asset_item2(&mesh_ref));

	auto mesh_only_chunk_desc = toy_add_scene_entity_descriptor(
		scene,
		toy_create_entity_chunk_descriptor_mesh_only);
	assert(NULL != mesh_only_chunk_desc);

	{
		uint32_t object_id = toy_create_scene_entity(
			scene, mesh_only_chunk_desc);
		assert(UINT32_MAX != object_id);
		auto entity_ref = reinterpret_cast<toy_entity_ref_t*>(toy_get_asset_item(&scene->object_refs, object_id));
		assert(NULL != entity_ref);
		auto mesh_ref_arr = reinterpret_cast<toy_asset_pool_item_ref_t*>(toy_get_scene_chunk_offset2(entity_ref, TOY_SCENE_COMPONENT_TYPE_MESH));
		auto id_arr = reinterpret_cast<uint32_t*>(toy_get_scene_chunk_offset2(entity_ref, TOY_SCENE_COMPONENT_TYPE_OBJECT_ID));
		auto location_arr = reinterpret_cast<toy_fmat4x4_t*>(toy_get_scene_chunk_offset2(entity_ref, TOY_SCENE_COMPONENT_TYPE_LOCATION));

		mesh_ref_arr[entity_ref->index] = mesh_ref;
		toy_add_asset_ref(mesh_ref.pool, mesh_ref.index, 1);
		id_arr[entity_ref->index] = object_id;
		location_arr[entity_ref->index] = toy::TRS({ 0, 0, 1 }, toy::identity_quaternion(), { 1, 1, 1 });
		gd->mesh_ids[0] = object_id;
	}
	{
		uint32_t object_id = toy_create_scene_entity(
			scene, mesh_only_chunk_desc);
		assert(UINT32_MAX != object_id);
		auto entity_ref = reinterpret_cast<toy_entity_ref_t*>(toy_get_asset_item(&scene->object_refs, object_id));
		assert(NULL != entity_ref);
		auto mesh_ref_arr = reinterpret_cast<toy_asset_pool_item_ref_t*>(toy_get_scene_chunk_offset2(entity_ref, TOY_SCENE_COMPONENT_TYPE_MESH));
		auto id_arr = reinterpret_cast<uint32_t*>(toy_get_scene_chunk_offset2(entity_ref, TOY_SCENE_COMPONENT_TYPE_OBJECT_ID));
		auto location_arr = reinterpret_cast<toy_fmat4x4_t*>(toy_get_scene_chunk_offset2(entity_ref, TOY_SCENE_COMPONENT_TYPE_LOCATION));

		mesh_ref_arr[entity_ref->index] = mesh_ref;
		toy_add_asset_ref(mesh_ref.pool, mesh_ref.index, 1);
		id_arr[entity_ref->index] = object_id;
		location_arr[entity_ref->index] = toy::TRS({ 0, 0, 0 }, toy::identity_quaternion(), { 1, 1, 1 });
		gd->mesh_ids[1] = object_id;
	}
	{
		uint32_t object_id = toy_create_scene_entity(
			scene, mesh_only_chunk_desc);
		assert(UINT32_MAX != object_id);
		auto entity_ref = reinterpret_cast<toy_entity_ref_t*>(toy_get_asset_item(&scene->object_refs, object_id));
		assert(NULL != entity_ref);
		auto mesh_ref_arr = reinterpret_cast<toy_asset_pool_item_ref_t*>(toy_get_scene_chunk_offset2(entity_ref, TOY_SCENE_COMPONENT_TYPE_MESH));
		auto id_arr = reinterpret_cast<uint32_t*>(toy_get_scene_chunk_offset2(entity_ref, TOY_SCENE_COMPONENT_TYPE_OBJECT_ID));
		auto location_arr = reinterpret_cast<toy_fmat4x4_t*>(toy_get_scene_chunk_offset2(entity_ref, TOY_SCENE_COMPONENT_TYPE_LOCATION));

		mesh_ref_arr[entity_ref->index] = mesh_ref;
		toy_add_asset_ref(mesh_ref.pool, mesh_ref.index, 1);
		id_arr[entity_ref->index] = object_id;
		location_arr[entity_ref->index] = toy::TRS({ 0, -1.0f, 0 }, toy::fquat(toy::radian(-90.0f), toy_fvec3_t{ 1, 0, 0 }), { 4, 4, 4 });
	}

	uint32_t cam_index = toy_new_scene_camera(scene);
	assert(UINT32_MAX != cam_index);
	toy_init_perspective_camera(45.0f, 4.0f / 3.0f, 0.1f, 1000.0f, &scene->cameras[cam_index]);
	toy_scene_camera_t& cam = scene->cameras[cam_index];
	cam.eye = toy_fvec3_t{ 0.0f, 2.0f, 4.0f };
	cam.target = toy_fvec3_t{ 0.0f, 0.0f, 0.0f };
	cam.up = toy_fvec3_t{ 0.0f, 1.0f, 0.0f };
	toy::look_at(cam.eye, cam.target - cam.eye, cam.up, &cam.view_matrix);
	gd->camera_id = cam_index;
}


int demo_script_init (lua_State* vm)
{
	toy_app_t* app = (toy_app_t*)lua_touserdata(vm, 1);

	toy_error_t err;

	char path[MAX_PATH];
	size_t path_size;
	toy_get_cwd(path, sizeof(path), &path_size, &err);
	if (toy_is_failed(err)) {
		toy_log_error(&err);
	}
	else {
		toy_log_i("CWD = %s", path);
	}

	toy_lua_run(vm, &app->alc->list_alc, "D:/Projects/toy/toy/src/asset/script/init.lua", &err);
	if (toy_is_failed(err))
		toy_log_error(&err);

	return 0;
}


void update_scene (toy_app_t* app, void* user_data, uint64_t delta_ms)
{
	toy_keyboard_t* keybd = &app->hid.keyboard;
	game_data_t* gd = reinterpret_cast<game_data_t*>(user_data);

	auto camera = &app->top_scene->cameras[gd->camera_id];

	auto entity_ref = reinterpret_cast<toy_entity_ref_t*>(toy_get_asset_item(&app->top_scene->object_refs, gd->mesh_ids[1]));
	auto location_arr = reinterpret_cast<toy_fmat4x4_t*>(toy_get_scene_chunk_offset2(entity_ref, TOY_SCENE_COMPONENT_TYPE_LOCATION));
	float angle = (float)delta_ms / 1000.0f * (2 * toy::pi<float>());
	location_arr[entity_ref->index] = location_arr[entity_ref->index] * toy::fquat_to_fmat4x4(toy::fquat(angle, toy_fvec3_t{ 0, 0, 1 }));

	float speed = 1.0f;
	{
		float distance = speed * ((float)delta_ms / 1000.0f);
		float radian = toy::radian(90 * (delta_ms / 1000.0f));
		toy_fvec3_t forward = toy::normalize(camera->target - camera->eye);
		toy_fvec3_t right = toy::normalize(toy::cross(forward, camera->up));

		toy_fvec3_t translation = toy_fvec3_t{ 0, 0, 0 };
		toy_fquat_t rotation = toy::identity_quaternion();

		if (TOY_KEY_STATE_DOWN == toy_get_keyboard_key(keybd, 'A'))
			translation = translation - distance * right;
		if (TOY_KEY_STATE_DOWN == toy_get_keyboard_key(keybd, 'D'))
			translation = translation + distance * right;
		if (TOY_KEY_STATE_DOWN == toy_get_keyboard_key(keybd, 'W'))
			translation = translation + distance * forward;
		if (TOY_KEY_STATE_DOWN == toy_get_keyboard_key(keybd, 'S'))
			translation = translation - distance * forward;
		if (TOY_KEY_STATE_DOWN == toy_get_keyboard_key(keybd, VK_UP))
			rotation = toy::fquat(radian, toy_fvec3_t{ 1, 0, 0 }) * rotation;
		if (TOY_KEY_STATE_DOWN == toy_get_keyboard_key(keybd, VK_DOWN))
			rotation = toy::fquat(-radian, toy_fvec3_t{ 1, 0, 0 }) * rotation;
		if (TOY_KEY_STATE_DOWN == toy_get_keyboard_key(keybd, VK_RIGHT))
			rotation = toy::fquat(radian, toy_fvec3_t{ 0, -1, 0 }) * rotation;
		if (TOY_KEY_STATE_DOWN == toy_get_keyboard_key(keybd, VK_LEFT))
			rotation = toy::fquat(-radian, toy_fvec3_t{ 0, -1, 0 }) * rotation;

		camera->eye = camera->eye + translation;
		camera->target = camera->eye + rotation * forward;
		camera->up = rotation * camera->up;
		toy::look_at(camera->eye, camera->target - camera->eye, camera->up, &camera->view_matrix);
	}

	if (TOY_KEY_STATE_DOWN == toy_get_keyboard_key(keybd, VK_ESCAPE))
		app->window.is_quit = 1;
}


int demo_main (HINSTANCE hInstance)
{
	toy_error_t err;
	toy_main_context_t main_ctx;
	main_ctx.hInstance = hInstance;
	toy_app_t* app = toy_create_app("", &main_ctx, &err);
	if (toy_is_failed(err))
		return EXIT_FAILURE;

	toy_game_init_data_t init_data;
	init_data.lua_init_fp = demo_script_init;
	toy_init_game_data(app, &init_data, &err);
	if (toy_is_failed(err)) {
		toy_log_error(&err);
		goto FAIL_INIT;
	}

	game_data_t gd;
	toy_app_loop_event_t loop_evt;
	loop_evt.user_data = &gd;
	loop_evt.on_update = update_scene;
	load_scene(app, &gd);

	toy_main_loop(app, &loop_evt);

	toy_destroy_app(app);

	return 0;

FAIL_INIT:
	toy_destroy_app(app);
	return EXIT_FAILURE;
}


// Switch entry function in Project Property->Linker->System->SubSystem
int main (int argc, const char* argv[])
{
	//test();
	return demo_main(NULL);
}


// Switch entry function in Project Property->Linker->System->SubSystem
int WINAPI wWinMain (
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nShowCmd)
{
	return demo_main(hInstance);
}
