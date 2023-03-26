#include "../include/scene/toy_scene_camera.h"

#include "../include/toy_math.hpp"
#include <cmath>
#include "../toy_assert.h"

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

toy_fmat4x4_t toy_camera_model_to_view_matrix (const toy_fmat4x4_t* model_matrix)
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


void toy_calc_camera_zone (
	toy_scene_camera_t* cam,
	toy_fvec3_t output_zone[8])
{
	if (TOY_CAMERA_TYPE_PERSPECTIVE == cam->type) {
		toy_fvec3_t forward = toy::normalize(cam->target - cam->eye);
		toy_fvec3_t right = toy::normalize(toy::cross(forward, cam->up));
		toy_fvec3_t up = toy::cross(right, forward);

		float z_near = cam->perspective.z_near;
		float z_far = cam->perspective.z_far;
		float tan_asp = std::tanf(toy::radian(cam->perspective.fovy / 2.0f));
		toy_fvec3_t tan_y = up * tan_asp;
		toy_fvec3_t tan_x = right * tan_asp * cam->perspective.aspect;
		output_zone[0] = cam->eye + z_near * (forward + tan_x + tan_y);
		output_zone[1] = cam->eye + z_near * (forward + tan_x - tan_y);
		output_zone[2] = cam->eye + z_near * (forward - tan_x + tan_y);
		output_zone[3] = cam->eye + z_near * (forward - tan_x - tan_y);
		output_zone[4] = cam->eye + z_far * (forward + tan_x + tan_y);
		output_zone[5] = cam->eye + z_far * (forward + tan_x - tan_y);
		output_zone[6] = cam->eye + z_far * (forward - tan_x + tan_y);
		output_zone[7] = cam->eye + z_far * (forward - tan_x - tan_y);
	}
	else if (TOY_CAMERA_TYPE_ORTHOGRAPHIC == cam->type) {
		toy_fvec3_t forward = toy::normalize(cam->target - cam->eye);
		toy_fvec3_t right = toy::normalize(toy::cross(forward, cam->up));
		toy_fvec3_t up = toy::cross(right, forward);

		float z_near = cam->perspective.z_near;
		float z_far = cam->perspective.z_far;
		toy_fvec3_t x = right * cam->orthographic.width / 2.0f;
		toy_fvec3_t y = up * cam->orthographic.height / 2.0f;
		toy_fvec3_t z_n = forward * z_near;
		toy_fvec3_t z_f = forward * z_far;
		output_zone[0] = cam->eye + z_n + x + y;
		output_zone[1] = cam->eye + z_n + x - y;
		output_zone[2] = cam->eye + z_n - x + y;
		output_zone[3] = cam->eye + z_n - x - y;
		output_zone[4] = cam->eye + z_f + x + y;
		output_zone[5] = cam->eye + z_f + x - y;
		output_zone[6] = cam->eye + z_f - x + y;
		output_zone[7] = cam->eye + z_f - x - y;
	}
	else {
		toy_fmat4x4_t project;
		toy_calc_camera_project_matrix(cam, &project);
		toy_fmat4x4_t view;
		toy_calc_camera_view_matrix(cam, &view);

		toy_fmat4x4_t proj_inv, view_inv;
		toy::inverse(project, &proj_inv);
		toy::inverse(view, &view_inv);

		// Vulkan NDC (Normalized Device Coordinate):
		// x: {-1, 1}, from left to right
		// y: {1, -1}, from up to down
		// z: {0, 1}, into the screen
		toy_fmat4x4_t view_proj_inv = view_inv * proj_inv;
		output_zone[0] = view_proj_inv * toy_fvec3_t{ 1, -1, 0 };
		output_zone[1] = view_proj_inv * toy_fvec3_t{ 1, 1, 0 };
		output_zone[2] = view_proj_inv * toy_fvec3_t{ -1, -1, 0 };
		output_zone[3] = view_proj_inv * toy_fvec3_t{ -1, 1, 0 };
		output_zone[4] = view_proj_inv * toy_fvec3_t{ 1, -1, 1 };
		output_zone[5] = view_proj_inv * toy_fvec3_t{ 1, 1, 1 };
		output_zone[6] = view_proj_inv * toy_fvec3_t{ -1, -1, 1 };
		output_zone[7] = view_proj_inv * toy_fvec3_t{ -1, 1, 1 };
	}
}


void toy_calc_camera_view_matrix (
	toy_scene_camera_t* cam,
	toy_fmat4x4_t* output)
{
	toy::look_at(
		cam->eye,
		cam->target - cam->eye,
		cam->up,
		output);
}


void toy_calc_camera_project_matrix (
	toy_scene_camera_t* cam,
	toy_fmat4x4_t* output)
{
	if (TOY_CAMERA_TYPE_PERSPECTIVE == cam->type) {
		toy::perspective_vk(
			cam->perspective.fovy,
			cam->perspective.aspect,
			cam->perspective.z_near,
			cam->perspective.z_far,
			output);
	}
	else if (TOY_CAMERA_TYPE_ORTHOGRAPHIC == cam->type) {
		toy::orthographic_vk(
			cam->orthographic.width,
			cam->orthographic.height,
			cam->orthographic.z_near,
			cam->orthographic.z_far,
			output);
	}
	else {
		TOY_ASSERT(0);
	}
}

TOY_EXTERN_C_END
