#include "include/toy_math.hpp"

#include <cmath>

#define GLM_FORCE_MESSAGES
#define GLM_FORCE_SIZE_T_LENGTH
#if TOY_DRIVER_VULKAN
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#endif
#define GLM_FORCE_INTRINSICS
#define GLM_FORCE_EXPLICIT_CTOR
#include "third_party/glm/glm.hpp"
#include "third_party/glm/gtc/matrix_transform.hpp"
#include "third_party/glm/gtc/type_ptr.hpp"
#include "third_party/glm/gtx/quaternion.hpp"

TOY_NAMESPACE_START

// Treat point vec3 as vec4(vec3, 1), treat vector vec3 as vec4(vec3, 0)
toy_fvec3_t operator* (const toy_fmat4x4_t& m, const toy_fvec3_t& v) {
#if TOY_MATRIX_ROW_MAJOR
	float x = m.v[0] * v.x + m.v[1] * v.y + m.v[2] * v.z + m.v[3];
	float y = m.v[4] * v.x + m.v[5] * v.y + m.v[6] * v.z + m.v[7];
	float z = m.v[8] * v.x + m.v[9] * v.y + m.v[10] * v.z + m.v[11];
	float w = m.v[12] * v.x + m.v[13] * v.y + m.v[14] * v.z + m.v[15];
	return toy_fvec3_t{ x, y, z } / w;
#else
	float x = m.v[0] * v.x + m.v[4] * v.y + m.v[8] * v.z + m.v[12];
	float y = m.v[1] * v.x + m.v[5] * v.y + m.v[9] * v.z + m.v[13];
	float z = m.v[2] * v.x + m.v[6] * v.y + m.v[10] * v.z + m.v[14];
	float w = m.v[3] * v.x + m.v[7] * v.y + m.v[11] * v.z + m.v[15];
	return toy_fvec3_t{ x, y, z } / w;
#endif
}


toy_fquat_t fquat (float radian, const toy_fvec3_t& axis)
{
	radian *= 0.5f;
	toy_fvec3_t a = toy::normalize(axis);
	auto s = std::sin(radian);
	toy_fquat_t r;
	r.w = std::cos(radian);
	r.x = a.x * s;
	r.y = a.y * s;
	r.z = a.z * s;
	return r;
}


toy_fvec2_t normalize (const toy_fvec2_t& v) {
	float length = std::sqrtf(v.x * v.x + v.y * v.y);
	return toy_fvec2_t{ v.x / length, v.y / length };
}


toy_fvec3_t normalize (const toy_fvec3_t& v) {
	float length = std::sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
	return toy_fvec3_t{ v.x / length, v.y / length, v.z / length };
}


toy_fvec4_t normalize (const toy_fvec4_t& v) {
	float length = std::sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
	return toy_fvec4_t{ v.x / length, v.y / length, v.z / length, v.w / length };
}


toy_fquat_t normalize (const toy_fquat_t& q) {
	auto gq = glm::normalize(glm::quat(q.w, q.x, q.y, q.z));
	toy_fquat_t r;
	r.w = gq.w; r.x = gq.x; r.y = gq.y; r.z = gq.z;
	return r;
}


float dot (const toy_fvec3_t& v1, const toy_fvec3_t& v2) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}


float dot (const toy_fvec4_t& v1, const toy_fvec4_t& v2) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
}


toy_fvec3_t cross (const toy_fvec3_t& v1, const toy_fvec3_t& v2) {
	return toy_fvec3_t{
		v1.y * v2.z - v1.z * v2.y,
		v1.z * v2.x - v1.x * v2.z,
		v1.x * v2.y - v1.y * v2.x
	};
}


toy_fmat4x4_t operator* (const toy_fmat4x4_t& m1, const toy_fmat4x4_t& m2) {
#if TOY_MATRIX_ROW_MAJOR
	toy_fvec4_t r0{ m2.row[0].x, m2.row[1].x, m2.row[2].x, m2.row[3].x };
	toy_fvec4_t r1{ m2.row[0].y, m2.row[1].y, m2.row[2].y, m2.row[3].y };
	toy_fvec4_t r2{ m2.row[0].z, m2.row[1].z, m2.row[2].z, m2.row[3].z };
	toy_fvec4_t r3{ m2.row[0].w, m2.row[1].w, m2.row[2].w, m2.row[3].w };
	return toy_fmat4x4_t{
		dot(m1.row[0], r0), dot(m1.row[0], r1), dot(m1.row[0], r2), dot(m1.row[0], r3),
		dot(m1.row[1], r0), dot(m1.row[1], r1), dot(m1.row[1], r2), dot(m1.row[1], r3),
		dot(m1.row[2], r0), dot(m1.row[2], r1), dot(m1.row[2], r2), dot(m1.row[2], r3),
		dot(m1.row[3], r0), dot(m1.row[3], r1), dot(m1.row[3], r2), dot(m1.row[3], r3),
	};
#else
	toy_fvec4_t r0{ m1.row[0].x, m1.row[1].x, m1.row[2].x, m1.row[3].x };
	toy_fvec4_t r1{ m1.row[0].y, m1.row[1].y, m1.row[2].y, m1.row[3].y };
	toy_fvec4_t r2{ m1.row[0].z, m1.row[1].z, m1.row[2].z, m1.row[3].z };
	toy_fvec4_t r3{ m1.row[0].w, m1.row[1].w, m1.row[2].w, m1.row[3].w };
	return toy_fmat4x4_t{
		dot(m2.row[0], r0), dot(m2.row[0], r1), dot(m2.row[0], r2), dot(m2.row[0], r3),
		dot(m2.row[1], r0), dot(m2.row[1], r1), dot(m2.row[1], r2), dot(m2.row[1], r3),
		dot(m2.row[2], r0), dot(m2.row[2], r1), dot(m2.row[2], r2), dot(m2.row[2], r3),
		dot(m2.row[3], r0), dot(m2.row[3], r1), dot(m2.row[3], r2), dot(m2.row[3], r3),
	};
#endif
}


// result is rotate q then rotate p
// result * m == p * q * m
toy_fquat_t operator*(const toy_fquat_t& p, const toy_fquat_t& q)
{
	toy_fquat_t r;
	r.x = q.w * p.x + q.x * p.w + q.y * p.z - q.z * p.y;
	r.y = q.w * p.y + q.y * p.w + q.z * p.x - q.x * p.z;
	r.z = q.w * p.z + q.z * p.w + q.x * p.y - q.y * p.x;
	r.w = q.w * p.w - q.x * p.x - q.y * p.y - q.z * p.z;
	return r;
}


toy_fvec3_t operator*(const toy_fquat_t& q, const toy_fvec3_t v)
{
	const toy_fvec3_t qv{ q.x, q.y, q.z };
	const toy_fvec3_t uv(cross(qv, v));

	return v + (cross(qv, uv) + uv * q.w) * 2.0f;
}


toy_fmat4x4_t fquat_to_fmat4x4 (const toy_fquat_t& q)
{
	auto xx = q.x * q.x;
	auto yy = q.y * q.y;
	auto zz = q.z * q.z;
	auto xy = q.x * q.y;
	auto xz = q.x * q.z;
	auto yz = q.y * q.z;
	auto xw = q.x * q.w;
	auto yw = q.y * q.w;
	auto zw = q.z * q.w;

	toy_fmat4x4_t r;
#if TOY_MATRIX_ROW_MAJOR
	r.v[0] = 1.0f - 2.0f * (yy + zz);
	r.v[1] = 2.0f * (xy + zw);
	r.v[2] = 2.0f * (xz - yw);
	r.v[3] = 0.0f;

	r.v[4] = 2.0f * (xy - zw);
	r.v[5] = 1.0f - 2.0f * (xx + zz);
	r.v[6] = 2.0f * (yz + xw);
	r.v[7] = 0.0f;

	r.v[8] = 2.0f * (xz + yw);
	r.v[9] = 2.0f * (yz - xw);
	r.v[10] = 1.0f - 2.0f * (xx + yy);
	r.v[11] = 0.0f;

	r.row[3] = { 0.0f, 0.0f, 0.0f, 1.0f };
#else
	r.v[0] = 1.0f - 2.0f * (yy + zz);
	r.v[1] = 2.0f * (xy - zw);
	r.v[2] = 2.0f * (xz + yw);
	r.v[3] = 0.0f;

	r.v[4] = 2.0f * (xy + zw);
	r.v[5] = 1.0f - 2.0f * (xx + zz);
	r.v[6] = 2.0f * (yz - xw);
	r.v[7] = 0.0f;

	r.v[8] = 2.0f * (xz - yw);
	r.v[9] = 2.0f * (yz + xw);
	r.v[10] = 1.0f - 2.0f * (xx + yy);
	r.v[11] = 0.0f;

	r.row[3] = { 0.0f, 0.0f, 0.0f, 1.0f };
#endif
	return r;
}



toy_fmat4x4_t TRS (const toy_fvec3_t& translation, const toy_fmat4x4_t& rotation, const toy_fvec3_t& scale) {
#if TOY_MATRIX_ROW_MAJOR
	return toy_fmat4x4_t{
		scale.x * rotation.row[0].x, scale.y * rotation.row[0].y, scale.z * rotation.row[0].z, translation.x,
		scale.x * rotation.row[1].x, scale.y * rotation.row[1].y, scale.z * rotation.row[1].z, translation.y,
		scale.x * rotation.row[2].x, scale.y * rotation.row[2].y, scale.z * rotation.row[2].z, translation.z,
		0, 0, 0, 1
	};
#else
	return toy_fmat4x4_t{
		scale.x * rotation.row[0].x, scale.x * rotation.row[1].x, scale.x * rotation.row[2].x, 0,
		scale.y * rotation.row[0].y, scale.y * rotation.row[1].y, scale.y * rotation.row[2].y, 0,
		scale.z * rotation.row[0].z, scale.z * rotation.row[1].z, scale.z * rotation.row[2].z, 0,
		translation.x, translation.y, translation.z, 1
	};
#endif
}



void look_at (const toy_fvec3_t& eye, const toy_fvec3_t& forward, const toy_fvec3_t& up, toy_fmat4x4_t* output)
{
	// Cartesian coordinates is RH(right hand)
	toy_fvec3_t z = normalize(-forward);
	toy_fvec3_t x = normalize(cross(up, z));
	toy_fvec3_t y = cross(z, x);

#if TOY_MATRIX_ROW_MAJOR
	output->row[0] = toy_fvec4_t{ x.x, x.y, x.z, -dot(x, eye) };
	output->row[1] = toy_fvec4_t{ y.x, y.y, y.z, -dot(y, eye) };
	output->row[2] = toy_fvec4_t{ z.x, z.y, z.z, -dot(z, eye) };
	output->row[3] = toy_fvec4_t{ 0.0f, 0.0f, 0.0f, 1.0f };
#else
	output->row[0] = toy_fvec4_t{ x.x, y.x, z.x, 0.0f };
	output->row[1] = toy_fvec4_t{ x.y, y.y, z.y, 0.0f };
	output->row[2] = toy_fvec4_t{ x.z, y.z, z.z, 0.0f };
	output->row[3] = toy_fvec4_t{ -dot(x, eye), -dot(y, eye), -dot(z, eye), 1.0f };
#endif
}


// left <= right, bottom <= top, z_near <= z_far
void orthographic_vk (float left, float right, float bottom, float top, float z_near, float z_far, toy_fmat4x4_t* output)
{
	//const toy_fvec3_t t{ -(left + right) * 0.5f, -(bottom + top) * 0.5f, z_near }; // translate
	const toy_fvec3_t s{ 2.0f / (right - left), 2.0f / (bottom - top), 1.0f / (z_near - z_far) }; // scale
#if TOY_MATRIX_ROW_MAJOR
	//output->row[0] = toy_fvec4_t{ s.x, 0.0f, 0.0f, t.x * s.x };
	//output->row[1] = toy_fvec4_t{ 0.0f, s.y, 0.0f, t.y * s.y };
	//output->row[2] = toy_fvec4_t{ 0.0f, 0.0f, s.z, t.z * s.z };
	output->row[0] = toy_fvec4_t{ s.x, 0.0f, 0.0f, (left + right) / (left - right) };
	output->row[1] = toy_fvec4_t{ 0.0f, s.y, 0.0f, (bottom + top) / (top - bottom) };
	output->row[2] = toy_fvec4_t{ 0.0f, 0.0f, s.z, z_near / (z_near - z_far) };
	output->row[3] = toy_fvec4_t{ 0.0f, 0.0f, 0.0f, 1.0f };
#else
	output->row[0] = toy_fvec4_t{ s.x, 0.0f, 0.0f, 0.0f };
	output->row[1] = toy_fvec4_t{ 0.0f, s.y, 0.0f, 0.0f };
	output->row[2] = toy_fvec4_t{ 0.0f, 0.0f, s.z, 0.0f };
	output->row[3] = toy_fvec4_t{ (left + right) / (left - right), (bottom + top) / (top - bottom), z_near / (z_near - z_far), 1.0f };
#endif
}


void orthographic_vk (float width, float height, float z_near, float z_far, toy_fmat4x4_t* output)
{
	//const toy_fvec3_t t{ 0.0f, 0.0f, z_near }; // translate
	const toy_fvec3_t s{ 2.0f / width, -2.0f / height, 1.0f / (z_near - z_far) }; // scale
#if TOY_MATRIX_ROW_MAJOR
	//output->row[0] = toy_fvec4_t{ s.x, 0.0f, 0.0f, t.x * s.x };
	//output->row[1] = toy_fvec4_t{ 0.0f, s.y, 0.0f, t.y * s.y };
	//output->row[2] = toy_fvec4_t{ 0.0f, 0.0f, s.z, t.z * s.z };
	output->row[0] = toy_fvec4_t{ s.x, 0.0f, 0.0f, 0.0f };
	output->row[1] = toy_fvec4_t{ 0.0f, s.y, 0.0f, 0.0f };
	output->row[2] = toy_fvec4_t{ 0.0f, 0.0f, s.z, z_near / (z_near - z_far) };
	output->row[3] = toy_fvec4_t{ 0.0f, 0.0f, 0.0f, 1.0f };
#else
	output->row[0] = toy_fvec4_t{ s.x, 0.0f, 0.0f, 0.0f };
	output->row[1] = toy_fvec4_t{ 0.0f, s.y, 0.0f, 0.0f };
	output->row[2] = toy_fvec4_t{ 0.0f, 0.0f, s.z, 0.0f };
	output->row[3] = toy_fvec4_t{ 0.0f, 0.0f, z_near / (z_near - z_far), 1.0f };
#endif
}


void perspective_vk (float fovy, float aspect, float z_near, float z_far, toy_fmat4x4_t* output)
{
	const float tan_fov = std::tanf(fovy / 2.0f);
	const float y_near = z_near * tan_fov;
	const float y_far = z_far * tan_fov;
	const float x_near = y_near * aspect;
	const float x_far = y_far * aspect;

#if TOY_MATRIX_ROW_MAJOR
	/*
	toy_fmat4x4_t persp2ortho{
		n, 0, 0, 0,
		0, n, 0, 0,
		0, 0, n+f, -nf,
		0, 0, 1, 0
	};
	toy_fmat4x4_t ortho;
	toy_fvec3_t t{ 0.0f, 0.0f, -z_near }; // translate
	toy_fvec3_t s{ 2.0f / width, 2.0f / height, 1.0f / (z_far - z_near }; // scale
	orthographic_vk(2 * x_near, 2 * y_near, z_near, z_far, &ortho);
	{
		1/xn, 0, 0, 0
		0, 1/yn, 0, 0
		0, 0, 1/(f-n), -n/(f-n)
		0, 0, 0, 1
	}
	perspective = ortho * persp2ortho;
	{
		n/xn, 0, 0, 0
		0, n/yn, 0, 0
		0, 0, f/(f-n), -nf/(f-n)
		0, 0, 1, 0
	}
	*output = perspective * scale(1, -1, -1)

	*/
	* output = toy_fmat4x4_t{
		1.0f / (tan_fov * aspect), 0, 0, 0,
		0, -1.0f / tan_fov, 0, 0,
		0, 0, z_far / (z_near - z_far), z_near * z_far / (z_near - z_far),
		0, 0, -1.0f, 0
	};
#else
	* output = toy_fmat4x4_t{
		1.0f / (tan_fov * aspect), 0, 0, 0,
		0, -1.0f / tan_fov, 0, 0,
		0, 0, z_far / (z_near - z_far), -1.0f,
		0, 0, z_near * z_far / (z_near - z_far), 0
	};
#endif
}


TOY_NAMESPACE_END
