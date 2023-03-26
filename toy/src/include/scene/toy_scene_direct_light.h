#pragma once

#include "../toy_platform.h"
#include "../toy_math_type.h"
#include "../scene/toy_scene_camera.h"

typedef struct toy_direct_light_t {
	toy_fvec3_t direction;
	toy_fvec3_t position;
	toy_fvec3_t up;
}toy_direct_light_t;


TOY_EXTERN_C_START

void toy_calc_shadow_view (
	toy_direct_light_t* light,
	toy_fvec3_t camera_zone[8],
	toy_fmat4x4_t* output_view,
	toy_fmat4x4_t* output_project
);

TOY_EXTERN_C_END
