#include "include/toy.h"

#include <stdlib.h>
#include <stdio.h>
#include "toy_assert.h"


toy_app_t* toy_create_app (
	const char* config_file,
	toy_main_context_t* main_ctx,
	toy_error_t* error)
{
	toy_allocator_t std_alc = toy_std_alc();
	toy_app_t* app = toy_alloc_aligned(&std_alc, sizeof(toy_app_t), sizeof(void*));
	if (NULL == app)
		goto FAIL_APP;
	memset(app, 0, sizeof(*app));

	toy_memory_config_t mem_cfg;
	mem_cfg.stack_size = 32 * 1024 * 1024; // 32M
	mem_cfg.buddy_size = 64 * 1024 * 1024; // 64M
	mem_cfg.list_size = 64 * 1024 * 1024; // 64M
	mem_cfg.chunk_count = 256;
	app->alc = toy_create_memory_allocator(&mem_cfg);
	if (NULL == app->alc) {
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "Failed to create memory allocator", error);
		goto FAIL_ALC;
	}

	toy_init_timer_env();
	toy_init_hid(&app->hid);

	app->main_vm = toy_lua_new_vm();
	if (NULL == app->main_vm) {
		toy_err(TOY_ERROR_OPERATION_FAILED, "Init main LUA vm failed", error);
		goto FAIL_MAIN_LUA_VM;
	}

	toy_window_create_info_t window_ci;
	window_ci.hInst = main_ctx->hInstance;
	window_ci.width = 800;
	window_ci.height = 600;
	window_ci.hid = &app->hid;
	window_ci.title = "Hello world";
	toy_create_window(&window_ci, &app->window, error);
	if (toy_is_failed(*error))
		goto FAIL_WINDOW;

	toy_vulkan_setup_info_t vk_setup_info;
	vk_setup_info.device_setup_info.app_name = "demo";
	vk_setup_info.device_setup_info.app_version[0] = 0;
	vk_setup_info.device_setup_info.app_version[1] = 0;
	vk_setup_info.device_setup_info.app_version[2] = 1;
	vk_setup_info.msaa_count = 1;
	toy_create_vulkan_driver(
		&vk_setup_info,
		&app->window,
		app->alc,
		app->vk_alc_cb_p,
		&app->vk_driver,
		error);
	if (toy_is_failed(*error))
		goto FAIL_RENDER_DRIVER;

	toy_create_asset_manager(
		30 * 1024 * 1024,
		app->alc,
		&app->vk_driver,
		&app->asset_mgr,
		error);
	if (toy_is_failed(*error))
		goto FAIL_ASSET_MANAGER;

	toy_create_built_in_vulkan_pipeline(
		&app->vk_driver,
		app->alc,
		&app->vk_built_in_pipeline,
		error);
	if (toy_is_failed(*error))
		goto FAIL_PIPELINE;

	app->top_scene = NULL;

	return app;

FAIL_PIPELINE:
	toy_destroy_asset_manager(&app->asset_mgr);
FAIL_ASSET_MANAGER:
	toy_destroy_vulkan_driver(&app->vk_driver, app->alc);
FAIL_RENDER_DRIVER:
	{
		toy_error_t window_err;
		toy_destroy_window(&app->window, &window_err);
		if (toy_is_failed(window_err))
			toy_log_error(&window_err);
	}
FAIL_WINDOW:
	lua_close(app->main_vm);
FAIL_MAIN_LUA_VM:
	toy_destroy_memory_allocator(app->alc);
FAIL_ALC:
	toy_free_aligned(&std_alc, app);
FAIL_APP:
	return NULL;
}


void toy_destroy_app (toy_app_t* app)
{
	toy_allocator_t std_alc = toy_std_alc();
	toy_error_t err;

	toy_scene_t* scene = app->top_scene;
	while (NULL != scene) {
		toy_scene_t* next_scene = scene->next;
		toy_destroy_scene(scene);
		scene = next_scene;
	}

	toy_destroy_built_in_vulkan_pipeline(
		&app->vk_driver, app->alc, &app->vk_built_in_pipeline);

	toy_destroy_asset_manager(&app->asset_mgr);

	toy_destroy_vulkan_driver(&app->vk_driver, app->alc);

	toy_destroy_window(&app->window, &err);
	if (toy_is_failed(err))
		toy_log_error(&err);

	lua_close(app->main_vm);

	toy_destroy_memory_allocator(app->alc);

	toy_free_aligned(&std_alc, app);
}


void toy_init_game_data (
	toy_app_t* app,
	toy_game_init_data_t* game_init_data,
	toy_error_t* error)
{
	int lua_err = LUA_OK;
	if (NULL != game_init_data->lua_init_fp) {
		lua_pushcfunction(app->main_vm, game_init_data->lua_init_fp);
		lua_pushlightuserdata(app->main_vm, app);
		lua_err = lua_pcall(app->main_vm, 1, 0, 0);
		if (LUA_OK != lua_err) {
			toy_err(TOY_ERROR_OPERATION_FAILED, "Game initializing function failed", error);
			return;
		}
	}
}


void toy_main_loop (toy_app_t* app, toy_app_loop_event_t* loop_evt)
{
	toy_timer_t frame_timer;
	toy_reset_timer(&frame_timer);
	uint64_t fps_cnt = 0, frame_ms = 0;
	uint64_t frame_begin = 0, frame_end = 0;

	uint32_t target_fps = 60;
	uint32_t sleep_ms = 1000 / target_fps;

	toy_error_t err;
	while (true) {
		toy_synchronize_hid(&app->hid);

		toy_query_window_messages(&app->window, &err);
		if (toy_is_failed(err))
			break;

		if (app->window.is_quit)
			break;

		frame_begin = toy_get_timer_during_ms(&frame_timer);
		uint64_t delta_time = frame_begin - frame_end;

		loop_evt->on_update(app, loop_evt->user_data, delta_time);

		if (NULL != app->top_scene) {
			toy_swap_vulkan_swapchain(
				app->vk_driver.device.handle,
				&app->vk_driver.swapchain,
				&err);

			toy_draw_scene(&app->vk_driver, app->top_scene, &app->vk_built_in_pipeline, &err);

			toy_present_vulkan_swapchain(
				app->vk_driver.device.handle,
				&app->vk_driver.swapchain,
				app->vk_driver.present_queue,
				&err);
		}

		frame_end = toy_get_timer_during_ms(&frame_timer);
		++fps_cnt;

		if (frame_end - frame_ms >= 1000) {
			frame_ms += 1000;
			char title[64];
			sprintf_s(title, sizeof(title), "fps = %llu, sleep_ms = %u", fps_cnt, sleep_ms);
			toy_set_window_title(&app->window, title, &err);
			TOY_ASSERT(toy_is_ok(err));
			if (fps_cnt > target_fps)
				++sleep_ms;
			else if (fps_cnt < target_fps && sleep_ms > 0)
				--sleep_ms;
			fps_cnt = 0;
		}

		toy_sleep(sleep_ms);
	}

	if (toy_is_failed(err))
		toy_log_error(&err);

	vkDeviceWaitIdle(app->vk_driver.device.handle);
}


void toy_push_scene (toy_app_t* app, toy_scene_t* scene)
{
	TOY_ASSERT(NULL != app && NULL != scene);
	scene->next = app->top_scene;
	if (NULL != app->top_scene)
		app->top_scene->prev = scene;
	app->top_scene = scene;
}


void toy_pop_scene (toy_app_t* app)
{
	TOY_ASSERT(NULL != app && NULL != app->top_scene);
	toy_scene_t* scene = app->top_scene;
	app->top_scene = scene->next;
	if (NULL != scene->next)
		scene->next->prev = NULL;
	toy_destroy_scene(scene);
}
