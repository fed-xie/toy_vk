#pragma once

#include "toy_platform.h"


typedef union toy_fvec2_t {
	toy_alignas(8) float v[2];
	struct {
		float x, y;
	};
}toy_fvec2_t;


typedef union toy_fvec3_t {
	float v[3];
	struct {
		float x, y, z;
	};
}toy_fvec3_t;


typedef union toy_fvec4_t {
	toy_alignas(16) float v[4];
	struct {
		float x, y, z, w;
	};
}toy_fvec4_t;


// Quaternion
typedef union toy_fquat_t {
	toy_alignas(16) float v[4];
	struct {
		float x, y, z, w;
	};
}toy_fquat_t;


#define TOY_MATRIX_ROW_MAJOR 0

typedef union toy_fmat4x4_t {
	struct {
		toy_alignas(64) toy_fvec4_t row[4];
	};
	toy_alignas(64) float v[16];
}toy_fmat4x4_t;
