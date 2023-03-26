#pragma once

#include "toy_platform.h"
#include "toy_error.h"
#include "toy_log.h"
#include "toy_allocator.h"
#include "toy_memory.h"
#include "toy_window.h"
#include "toy_timer.h"
#if TOY_DRIVER_VULKAN
#include "platform/vulkan/toy_vulkan.h"

#include "auxiliary/toy_built_in_pipeline.h"
#endif
#include "toy_file.h"
#include "toy_lua.h"
#include "toy_asset_manager.h"
#include "toy_scene.h"
#if __cplusplus
#include "toy_math.hpp"
#endif


#if TOY_OS_WINDOWS
typedef struct toy_main_context_t {
	HINSTANCE hInstance;
}toy_main_context_t;
#endif

typedef struct toy_app_t toy_app_t;

typedef void (*toy_app_on_frame_update_fp) (toy_app_t* app, void* user_data, uint64_t delta_ms);

typedef struct toy_app_loop_event_t {
	void* user_data;
	toy_app_on_frame_update_fp on_update;
}toy_app_loop_event_t;

struct toy_app_t {
	lua_State* main_vm;
	toy_hid_t hid;
	toy_window_t window;
	toy_memory_allocator_t* alc;
	toy_asset_manager_t asset_mgr;
	toy_scene_t* top_scene;
	toy_app_loop_event_t* loop_event;
#if TOY_DRIVER_VULKAN
	toy_vulkan_driver_t vk_driver;
	toy_built_in_pipeline_p vk_built_in_pipeline;
	VkAllocationCallbacks vk_alc_cb;
	VkAllocationCallbacks *vk_alc_cb_p;
#endif
};


typedef struct toy_game_init_data_t {
	lua_CFunction lua_init_fp; // Will be run with toy_app_t* as lightuserdata as 1st parameter
}toy_game_init_data_t;


TOY_EXTERN_C_START

toy_app_t* toy_create_app (
	const char* config_file,
	toy_main_context_t* main_ctx,
	toy_error_t* error
);

void toy_destroy_app (toy_app_t* app);

void toy_init_game_data (
	toy_app_t* app,
	toy_game_init_data_t* game_init_data,
	toy_error_t* error
);

void toy_main_loop (toy_app_t* app, toy_app_loop_event_t* loop_evt);

void toy_push_scene (toy_app_t* app, toy_scene_t* scene);

void toy_pop_scene (toy_app_t* app);

TOY_EXTERN_C_END
