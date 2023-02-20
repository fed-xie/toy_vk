#include "../include/scene/toy_scene_camera.h"
#include "../include/toy_math.hpp"

using toy::operator+;
using toy::operator-;
using toy::operator*;
using toy::operator/;

TOY_EXTERN_C_START

toy_fmat4x4_t toy_calc_camera_model_matrix (toy_fvec3_t eye, toy_fvec3_t target, toy_fvec3_t up)
{
	toy_fvec3_t forward = toy::normalize(target - eye);
	toy_fvec3_t right = toy::normalize(toy::cross(forward, up));
	toy_fvec3_t normal_up = toy::cross(right, forward);
#if TOY_MATRIX_ROW_MAJOR
	toy_fmat4x4_t matrix = {
		right.x, normal_up.x, -forward.x, eye.x,
		right.y, normal_up.y, -forward.y, eye.y,
		right.z, normal_up.z, -forward.z, eye.z,
		0, 0, 0, 1
	};
#else
	toy_fmat4x4_t matrix = {
		right.x, right.y, right.z, 0,
		normal_up.x, normal_up.y, normal_up.z, 0,
		-forward.x, -forward.y, -forward.z, 0,
		eye.x, eye.y, eye.z, 1
	};
#endif
	return matrix;
}

toy_fmat4x4_t toy_calc_camera_view_matrix (const toy_fmat4x4_t* model_matrix)
{
#if TOY_MATRIX_ROW_MAJOR
	toy_fvec3_t eye = toy_fvec3_t{ model_matrix->v[3], model_matrix->v[7], model_matrix->v[15] };
#else
	toy_fvec3_t eye = toy_fvec3_t{ model_matrix->v[12], model_matrix->v[13], model_matrix->v[14] };
#endif

	toy_fvec3_t forward = (*model_matrix) * toy_fvec3_t{ 0, 0, -1 };
	toy_fvec3_t up = (*model_matrix) * toy_fvec3_t{ 0, 1, 0 };

	toy_fmat4x4_t view_matrix;
	toy::look_at(eye, forward - eye, up - eye, &view_matrix);
	return view_matrix;
}


void toy_init_perspective_camera (
	float fovy,
	float aspect,
	float z_near,
	float z_far,
	toy_scene_camera_t* output)
{
	output->eye = toy_fvec3_t{ 0, 0, 0 };
	output->target = toy_fvec3_t{ 0, 0, -1 };
	output->up = toy_fvec3_t{ 0, 1, 0 };
	toy::look_at(
		output->eye,
		output->target - output->eye,
		output->up,
		&output->view_matrix);
	toy::perspective_vk(
		fovy,
		aspect,
		z_near,
		z_far,
		&output->project_matrix);
}


void toy_init_orthographic_camera (
	float width,
	float height,
	float z_near,
	float z_far,
	toy_scene_camera_t* output)
{
	output->eye = toy_fvec3_t{ 0, 0, 0 };
	output->target = toy_fvec3_t{ 0, 0, -1 };
	output->up = toy_fvec3_t{ 0, 1, 0 };
	toy::look_at(
		output->eye,
		output->target - output->eye,
		output->up,
		&output->view_matrix);
	toy::orthographic_vk(
		width,
		height,
		z_near,
		z_far,
		&output->project_matrix);
}

TOY_EXTERN_C_END
