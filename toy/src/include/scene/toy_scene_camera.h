#pragma once

#include "../toy_platform.h"
#include "../toy_math_type.h"

enum toy_camera_type_t {
	TOY_CAMERA_TYPE_PERSPECTIVE = 0,
	TOY_CAMERA_TYPE_ORTHOGRAPHIC
};

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

	enum toy_camera_type_t type;
	union {
		struct {
			float fovy;
			float aspect;
			float z_near;
			float z_far;
		} perspective;
		struct {
			float width;
			float height;
			float z_near;
			float z_far;
		} orthographic;
	};
}toy_scene_camera_t;


TOY_EXTERN_C_START


void toy_calc_camera_zone (
	toy_scene_camera_t* cam,
	toy_fvec3_t output_zone[8]
);

toy_fmat4x4_t toy_calc_camera_model_matrix (toy_fvec3_t eye, toy_fvec3_t target, toy_fvec3_t up);

toy_fmat4x4_t toy_camera_model_to_view_matrix (const toy_fmat4x4_t* model_matrix);

void toy_calc_camera_view_matrix (
	toy_scene_camera_t* cam,
	toy_fmat4x4_t* output
);

void toy_calc_camera_project_matrix (
	toy_scene_camera_t* cam,
	toy_fmat4x4_t* output
);

TOY_EXTERN_C_END
