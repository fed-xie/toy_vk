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
	uint32_t mesh_id;
	toy_asset_pool_item_ref_t mesh_primitive_ref;
};

void load_scene (toy_app_t* app, game_data_t* gd)
{
	toy_error_t err;

	toy_scene_t* scene = toy_create_scene(app->alc, &err);
	assert(NULL != scene);
	toy_push_scene(app, scene);

	toy_asset_pool_item_ref_t mesh_primitive_ref;
	toy_load_mesh_primitive(
		&app->asset_mgr,
		toy_get_built_in_mesh_rectangle(),
		&mesh_primitive_ref,
		&err);
	assert(toy_is_ok(err));

	for (uint32_t i = 0; i < sizeof(scene->object_ids) / sizeof(scene->object_ids[0]); ++i) {
		scene->object_ids[i] = i + 1;
		scene->mesh_primitives[i] = mesh_primitive_ref.index;
		float offset = (i & 1) ? (float)i : -(float)i;
		scene->inst_matrices[i] = toy::TRS(toy_fvec3_t{ offset / 2.0f, offset / 4.0f, -static_cast<float>(i) }, toy::identity_quaternion(), { 1, 1, 1 });
	}
	scene->object_count = 128;

	toy_init_perspective_camera(45.0f, 4.0f / 3.0f, 0.1f, 1000.0f, &scene->main_camera);
	toy_scene_camera_t& cam = scene->main_camera;
	cam.eye = toy_fvec3_t{ 0.0f, 1.0f, 4.0f };
	cam.target = toy_fvec3_t{ 0.0f, 0.0f, 0.0f };
	cam.up = toy_fvec3_t{ 0.0f, 1.0f, 0.0f };
	toy::look_at(cam.eye, cam.target - cam.eye, cam.up, &cam.view_matrix);

	gd->mesh_id = 3;
	gd->mesh_primitive_ref = mesh_primitive_ref;
}


void destroy_scene (toy_app_t* app, game_data_t* gd)
{
	toy_free_asset_item(gd->mesh_primitive_ref.pool, gd->mesh_primitive_ref.index);
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

	toy_lua_run(vm, &app->alc->list_alc, "src/asset/script/init.lua", &err);
	if (toy_is_failed(err))
		toy_log_error(&err);

	return 0;
}


void update_scene (toy_app_t* app, void* user_data, uint64_t delta_ms)
{
	toy_keyboard_t* keybd = &app->hid.keyboard;
	game_data_t* gd = reinterpret_cast<game_data_t*>(user_data);

	auto camera = &app->top_scene->main_camera;

	float angle = (float)delta_ms / 1000.0f * (2 * toy::pi<float>());
	app->top_scene->inst_matrices[gd->mesh_id] = app->top_scene->inst_matrices[gd->mesh_id] * toy::fquat_to_fmat4x4(toy::fquat(angle, toy_fvec3_t{ 0, 0, 1 }));

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

	destroy_scene(app, &gd);

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
