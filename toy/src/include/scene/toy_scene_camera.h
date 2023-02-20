#pragma once

#include "../toy_platform.h"
#include "../toy_math_type.h"

/*
	Orientation is changed by game object
	Default value is:
		eye:		{ 0, 0, 0 }
		target:	{ 0, 0, -1 }
		up:		{ 0, 1, 0 }
*/
typedef struct toy_scene_camera_t {
	toy_fvec3_t eye;
	toy_fvec3_t target;
	toy_fvec3_t up;

	toy_fmat4x4_t view_matrix;
	toy_fmat4x4_t project_matrix;
}toy_scene_camera_t;


TOY_EXTERN_C_START

toy_fmat4x4_t toy_calc_camera_model_matrix (toy_fvec3_t eye, toy_fvec3_t target, toy_fvec3_t up);

toy_fmat4x4_t toy_calc_camera_view_matrix (const toy_fmat4x4_t* model_matrix);

void toy_init_perspective_camera (
	float fovy,
	float aspect,
	float z_near,
	float z_far,
	toy_scene_camera_t* output
);

void toy_init_orthographic_camera (
	float width,
	float height,
	float z_near,
	float z_far,
	toy_scene_camera_t* output
);

TOY_EXTERN_C_END
