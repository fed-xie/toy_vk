#include "../include/toy.h"

#include "../include/auxiliary/toy_built_in_meshes.h"
#include "../include/auxiliary/toy_built_in_ecs.h"
#include "../include/auxiliary/toy_built_in_pipeline.h"

#include "../auxiliary/vulkan_pipeline/base.h"

#include <cassert>

using toy::operator*;
using toy::operator+;
using toy::operator-;
using toy::operator/;

struct game_data_t {
	uint32_t mesh_id;
	uint32_t mesh_index;
	uint32_t mesh2_index;
};

void load_scene (toy_app_t* app, game_data_t* gd)
{
	toy_error_t err;

	toy_scene_t* scene = toy_create_scene(app->alc, &err);
	assert(NULL != scene);
	toy_push_scene(app, scene);
	
	uint32_t mesh_index = toy_load_built_in_mesh(
		&app->asset_mgr,
		toy_get_built_in_mesh_rectangle(),
		&err);
	assert(toy_is_ok(err));

	toy_asset_pool_item_ref_t tex_ref;
	toy_load_texture2d(
		&app->asset_mgr,
		"assets/textures/test.jpg",
		&tex_ref,
		&err);
	assert(toy_is_ok(err));

	toy_asset_pool_item_ref_t tex2_ref;
	toy_load_texture2d(
		&app->asset_mgr,
		"assets/textures/test2.jpg",
		&tex2_ref,
		&err);
	assert(toy_is_ok(err));

	toy_image_sampler_t img_sampler;
	img_sampler.mag_filter = TOY_IMAGE_SAMPLER_FILTER_LINEAR;
	img_sampler.min_filter = TOY_IMAGE_SAMPLER_FILTER_LINEAR;
	img_sampler.wrap_u = TOY_IMAGE_SAMPLER_WRAP_REPEAT;
	img_sampler.wrap_v = TOY_IMAGE_SAMPLER_WRAP_REPEAT;
	img_sampler.wrap_w = TOY_IMAGE_SAMPLER_WRAP_REPEAT;
	toy_asset_pool_item_ref_t sampler_ref = toy_create_image_sampler(
		&app->asset_mgr, &img_sampler, &err);
	assert(toy_is_ok(err));

	uint32_t material_index = toy_alloc_vulkan_descriptor_set_single_texture(
		&app->asset_mgr, &app->vk_built_in_pipeline->desc_set_layouts.single_texture, &err);
	assert(toy_is_ok(err));
	toy_built_in_descriptor_set_single_texture_t** desc_set_data_p = (toy_built_in_descriptor_set_single_texture_t**)toy_get_asset_item(
		&app->asset_mgr.asset_pools.material, material_index);
	assert(NULL != desc_set_data_p);
	toy_built_in_descriptor_set_single_texture_t* desc_set_data = *desc_set_data_p;
	desc_set_data->image_ref = tex_ref;
	toy_add_asset_ref(tex_ref.pool, tex_ref.index, 1);
	desc_set_data->sampler_ref = sampler_ref;
	toy_add_asset_ref(sampler_ref.pool, sampler_ref.index, 1);

	toy_mesh_t* mesh = (toy_mesh_t*)toy_get_asset_item(&app->asset_mgr.asset_pools.mesh, mesh_index);
	mesh->material_index = material_index;
	toy_add_asset_ref(&app->asset_mgr.asset_pools.material, material_index, 1);

	uint32_t mesh2_index = toy_alloc_asset_item(&app->asset_mgr.asset_pools.mesh, &err);
	toy_mesh_t* mesh2 = (toy_mesh_t*)toy_get_asset_item(&app->asset_mgr.asset_pools.mesh, mesh2_index);
	mesh2->primitive_index = mesh->primitive_index;
	toy_add_asset_ref(&app->asset_mgr.asset_pools.mesh_primitive, mesh->primitive_index, 1);
	uint32_t material2_index = toy_alloc_vulkan_descriptor_set_single_texture(
		&app->asset_mgr, &app->vk_built_in_pipeline->desc_set_layouts.single_texture, &err);
	assert(toy_is_ok(err));
	toy_built_in_descriptor_set_single_texture_t** desc_set2_data_p = (toy_built_in_descriptor_set_single_texture_t**)toy_get_asset_item(
		&app->asset_mgr.asset_pools.material, material2_index);
	assert(NULL != desc_set2_data_p);
	toy_built_in_descriptor_set_single_texture_t* desc_set2_data = *desc_set2_data_p;
	desc_set2_data->image_ref = tex2_ref;
	toy_add_asset_ref(tex2_ref.pool, tex2_ref.index, 1);
	desc_set2_data->sampler_ref = sampler_ref;
	toy_add_asset_ref(sampler_ref.pool, sampler_ref.index, 1);
	mesh2->material_index = material2_index;
	toy_add_asset_ref(&app->asset_mgr.asset_pools.material, material2_index, 1);
	

	for (uint32_t i = 0; i < sizeof(scene->object_ids) / sizeof(scene->object_ids[0]); ++i) {
		scene->object_ids[i] = i + 1;
		scene->meshes[i] = (i % 10) ? mesh_index : mesh2_index;
		float offset = (i & 1) ? (float)i : -(float)i;
		float scale = i * 1.1f;
		scene->inst_matrices[i] = toy::TRS(toy_fvec3_t{ offset / 2.0f, offset / 4.0f, -static_cast<float>(i) }, toy::identity_quaternion(), { scale, scale, scale });
	}
	scene->meshes[3] = mesh2_index;
	scene->meshes[126] = mesh2_index;
	scene->inst_matrices[127] = toy::TRS({ 0, -0.5f, 0 }, toy::fquat(toy::radian(-90.0f), toy_fvec3_t{ 1, 0, 0 }), { 20, 20, 1 });
	scene->meshes[127] = mesh2_index;
	scene->object_count = 128;

	toy_scene_camera_t& cam = scene->main_camera;
	cam.eye = toy_fvec3_t{ 0.5f, 0.5f, 3.0f };
	cam.target = toy_fvec3_t{ 0.0f, 0.0f, -7.0f };
	cam.up = toy_fvec3_t{ 0.0f, 1.0f, 0.0f };
	cam.type = TOY_CAMERA_TYPE_PERSPECTIVE;
	cam.perspective.fovy = 45.0f;
	cam.perspective.aspect = 4.0f / 3.0f;
	cam.perspective.z_near = 0.1f;
	cam.perspective.z_far = 200.0f;

	gd->mesh_id = 3;
	gd->mesh_index = mesh_index;
	gd->mesh2_index = mesh2_index;
}


void destroy_scene (toy_app_t* app, game_data_t* gd)
{
	toy_free_asset_item(&app->asset_mgr.asset_pools.mesh, gd->mesh2_index);
	toy_free_asset_item(&app->asset_mgr.asset_pools.mesh, gd->mesh_index);
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

	float speed = 5.0f;
	{
		float distance = speed * ((float)delta_ms / 1000.0f);
		float radian = toy::radian(90 * (delta_ms / 1000.0f));
		toy_fvec3_t forward = toy::normalize(camera->target - camera->eye);
		toy_fvec3_t right = toy::normalize(toy::cross(forward, camera->up));
		toy_fvec3_t up = toy::normalize(camera->up);

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
		if (TOY_KEY_STATE_DOWN == toy_get_keyboard_key(keybd, 'Q'))
			translation = translation + distance * up;
		if (TOY_KEY_STATE_DOWN == toy_get_keyboard_key(keybd, 'E'))
			translation = translation - distance * up;
		if (TOY_KEY_STATE_DOWN == toy_get_keyboard_key(keybd, VK_UP))
			rotation = toy::fquat(radian, toy_fvec3_t{ 1, 0, 0 }) * rotation;
		if (TOY_KEY_STATE_DOWN == toy_get_keyboard_key(keybd, VK_DOWN))
			rotation = toy::fquat(-radian, toy_fvec3_t{ 1, 0, 0 }) * rotation;
		if (TOY_KEY_STATE_DOWN == toy_get_keyboard_key(keybd, VK_RIGHT))
			rotation = toy::fquat(radian, toy_fvec3_t{ 0, -1, 0 }) * rotation;
		if (TOY_KEY_STATE_DOWN == toy_get_keyboard_key(keybd, VK_LEFT))
			rotation = toy::fquat(-radian, toy_fvec3_t{ 0, -1, 0 }) * rotation;

		if (TOY_KEY_STATE_DOWN == toy_get_keyboard_key(keybd, 'U')) {
			camera->perspective.fovy += 1.0f;
			if (camera->perspective.fovy >= 170.0f)
				camera->perspective.fovy = 170.0f;
		}
		if (TOY_KEY_STATE_DOWN == toy_get_keyboard_key(keybd, 'J')) {
			camera->perspective.fovy -= 1.0f;
			if (camera->perspective.fovy <= 5.0f)
				camera->perspective.fovy = 5.0f;
		}

		camera->eye = camera->eye + translation;
		camera->target = camera->eye + rotation * forward;
		camera->up = rotation * camera->up;
	}

	if (TOY_KEY_STATE_DOWN == toy_get_keyboard_key(keybd, VK_ESCAPE))
		app->window.is_quit = 1;

	if (toy_is_keyboard_key_pressed(keybd, 'H')) {
		if (gd->mesh_index == app->top_scene->meshes[3])
			app->top_scene->meshes[3] = gd->mesh2_index;
		else
			app->top_scene->meshes[3] = gd->mesh_index;
	}
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
