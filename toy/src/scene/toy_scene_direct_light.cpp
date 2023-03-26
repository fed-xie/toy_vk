#include "../include/scene/toy_scene_direct_light.h"

#include "../include/toy_math.hpp"
#include "../toy_assert.h"
#include <cmath>


TOY_EXTERN_C_START

using toy::operator+;
using toy::operator-;
using toy::operator*;
using toy::operator/;


void toy_calc_shadow_view (
	toy_direct_light_t* light,
	toy_fvec3_t camera_zone[8],
	toy_fmat4x4_t* output_view,
	toy_fmat4x4_t* output_project)
{
	toy_fvec3_t forward = toy::normalize(light->direction);
	toy_fvec3_t right = toy::normalize(toy::cross(light->direction, light->up));
	toy_fvec3_t up = toy::normalize(toy::cross(right, forward));

	toy_fvec3_t origin = { 0, 0, 0 };
	toy_fmat4x4_t look_at;
	toy::look_at(origin, forward, up, &look_at);
	toy_fvec3_t light_zone[8];
	for (int i = 0; i < 8; ++i)
		light_zone[i] = look_at * camera_zone[i];

	toy_fvec3_t aabb_min = light_zone[0];
	toy_fvec3_t aabb_max = light_zone[0];
#define toy_min(a, b) ((a) < (b) ? (a) : (b))
#define toy_max(a, b) ((a) > (b) ? (a) : (b))
	for (int i = 1; i < 8; ++i) {
		aabb_min.x = toy_min(light_zone[i].x, aabb_min.x);
		aabb_min.y = toy_min(light_zone[i].y, aabb_min.y);
		aabb_min.z = toy_min(light_zone[i].z, aabb_min.z);
		aabb_max.x = toy_max(light_zone[i].x, aabb_max.x);
		aabb_max.y = toy_max(light_zone[i].y, aabb_max.y);
		aabb_max.z = toy_max(light_zone[i].z, aabb_max.z);
	}
#undef toy_min
#undef toy_max

	toy_fmat4x4_t orth;
	toy::orthographic_vk(aabb_min.x, aabb_max.x, aabb_min.y, aabb_max.y, aabb_min.z, aabb_max.z, &orth);

	*output_view = look_at;
	*output_project = orth;
}

TOY_EXTERN_C_END
