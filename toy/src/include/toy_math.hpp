#pragma once

#include "toy_platform.h"
#include "toy_math_type.h"


#if TOY_CPP
#include <limits>

TOY_NAMESPACE_START

toy_inline toy_fvec2_t fvec2 (float v) {
	return toy_fvec2_t{ v, v };
}

toy_inline toy_fvec3_t fvec3 (float v) {
	return toy_fvec3_t{ v, v, v };
}

toy_inline toy_fvec4_t fvec4 (float v) {
	return toy_fvec4_t{ v, v, v, v };
}

toy_inline toy_fmat4x4_t fmat4x4 (float v) {
	return toy_fmat4x4_t{
		v, 0.0f, 0.0f, 0.0f,
		0.0f, v, 0.0f, 0.0f,
		0.0f, 0.0f, v, 0.0f,
		0.0f, 0.0f, 0.0f, v };
}

toy_inline toy_fvec2_t operator+ (const toy_fvec2_t& v1, const toy_fvec2_t& v2) {
	return toy_fvec2_t{ v1.x + v2.x, v1.y + v2.y };
}

toy_inline toy_fvec2_t operator- (const toy_fvec2_t& v1, const toy_fvec2_t& v2) {
	return toy_fvec2_t{ v1.x - v2.x, v1.y - v2.y };
}

toy_inline toy_fvec2_t operator-(const toy_fvec2_t& v) {
	return toy_fvec2_t{ -v.x, -v.y };
}

toy_inline toy_fvec2_t operator* (const toy_fvec2_t& v1, const toy_fvec2_t& v2) {
	return toy_fvec2_t{ v1.x * v2.x, v1.y * v2.y };
}

toy_inline toy_fvec2_t operator/ (const toy_fvec2_t& v1, const toy_fvec2_t& v2) {
	return toy_fvec2_t{ v1.x / v2.x, v1.y / v2.y };
}


toy_inline toy_fvec3_t operator+ (const toy_fvec3_t& v1, const toy_fvec3_t& v2) {
	return toy_fvec3_t{ v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
}

toy_inline toy_fvec3_t operator- (const toy_fvec3_t& v1, const toy_fvec3_t& v2) {
	return toy_fvec3_t{ v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
}

toy_inline toy_fvec3_t operator- (const toy_fvec3_t& v) {
	return toy_fvec3_t{ -v.x, -v.y, -v.z };
}

toy_inline toy_fvec3_t operator* (const toy_fvec3_t& v1, const toy_fvec3_t& v2) {
	return toy_fvec3_t{ v1.x * v2.x, v1.y * v2.y, v1.z * v2.z };
}

toy_inline toy_fvec3_t operator/ (const toy_fvec3_t& v1, const toy_fvec3_t& v2) {
	return toy_fvec3_t{ v1.x / v2.x, v1.y / v2.y, v1.z / v2.z };
}

toy_inline toy_fvec3_t operator* (const toy_fvec3_t& v, float s) {
	return toy_fvec3_t{ v.x * s, v.y * s, v.z * s };
}

toy_inline toy_fvec3_t operator* (float s, const toy_fvec3_t& v) {
	return toy_fvec3_t{ v.x * s, v.y * s, v.z * s };
}

toy_inline toy_fvec3_t operator/ (const toy_fvec3_t& v, float s) {
	return toy_fvec3_t{ v.x / s, v.y / s, v.z / s };
}

toy_inline toy_fvec3_t operator/ (float s, const toy_fvec3_t& v) {
	return toy_fvec3_t{ v.x / s, v.y / s, v.z / s };
}

toy_fvec3_t operator* (const toy_fmat4x4_t& m, const toy_fvec3_t& v);

toy_inline void identify (toy_fmat4x4_t& m) {
	m = toy_fmat4x4_t{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
	};
}

toy_inline void identify (toy_fquat_t& q) {
	q.w = 1.0f; q.x = 0.0f; q.y = 0.0f; q.z = 0.0f;
}

constexpr toy_fmat4x4_t identity_matrix () {
	return toy_fmat4x4_t{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
	};
}

constexpr toy_fquat_t identity_quaternion () {
	toy_fquat_t ret{ 0 };
	ret.w = 1.0f;
	return ret;
}


template<typename T>
constexpr T epsilon()
{
	static_assert(std::numeric_limits<T>::is_iec559, "'epsilon' only accepts floating-point inputs");
	return std::numeric_limits<T>::epsilon();
}

template<typename T>
constexpr T pi()
{
	static_assert(std::numeric_limits<T>::is_iec559, "'pi' only accepts floating-point inputs");
	return static_cast<T>(3.14159265358979323846264338327950288);
}

constexpr float radian (float degree) {
	constexpr float r = 180.0f / pi<float>();
	return degree / r;
};

toy_fquat_t fquat (float radian, const toy_fvec3_t& axis);

toy_fvec2_t normalize (const toy_fvec2_t& v);
toy_fvec3_t normalize (const toy_fvec3_t& v);
toy_fvec4_t normalize (const toy_fvec4_t& v);
toy_fquat_t normalize (const toy_fquat_t& q);


float dot (const toy_fvec3_t& v1, const toy_fvec3_t& v2);
float dot (const toy_fvec4_t& v1, const toy_fvec4_t& v2);

toy_fvec3_t cross (const toy_fvec3_t& v1, const toy_fvec3_t& v2);

toy_fmat4x4_t operator* (const toy_fmat4x4_t& m1, const toy_fmat4x4_t& m2);

// result is rotate q then rotate p
// result * m == p * q * m
toy_fquat_t operator*(const toy_fquat_t& p, const toy_fquat_t& q);

toy_fvec3_t operator*(const toy_fquat_t& q, const toy_fvec3_t v);

void look_at (const toy_fvec3_t& eye, const toy_fvec3_t& forward, const toy_fvec3_t& up, toy_fmat4x4_t* output);

toy_fmat4x4_t fquat_to_fmat4x4 (const toy_fquat_t& q);

void inverse (const toy_fmat4x4_t& mat, toy_fmat4x4_t* output);

// Translate * Rotation * Scale Matrix
toy_fmat4x4_t TRS (const toy_fvec3_t& translation, const toy_fmat4x4_t& rotation, const toy_fvec3_t& scale);
// Translate * Rotation * Scale Matrix
toy_inline toy_fmat4x4_t TRS (const toy_fvec3_t& translation, const toy_fquat_t& rotation, const toy_fvec3_t& scale) {
	return TRS(translation, fquat_to_fmat4x4(rotation), scale);
}

// left <= right, bottom <= top, z_near <= z_far
void orthographic_vk (float left, float right, float bottom, float top, float z_near, float z_far, toy_fmat4x4_t* output);

void orthographic_vk (float width, float height, float z_near, float z_far, toy_fmat4x4_t* output);

// z_near <= z_far
void perspective_vk (float fovy, float aspect, float z_near, float z_far, toy_fmat4x4_t* output);

TOY_NAMESPACE_END
#endif // TOY_CPP
