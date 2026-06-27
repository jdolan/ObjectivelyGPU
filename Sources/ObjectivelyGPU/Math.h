/*
 * ObjectivelyGPU: Object oriented graphics framework for SDL3 and GNU C.
 * Copyright (C) 2026 Jay Dolan <jay@jaydolan.com>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 */

#pragma once

#include <stdbool.h>
#include <string.h>

#ifndef M_PI
  #define M_PI 3.14159265358979323846
#endif

#pragma mark - float

/**
 * @return `degrees` converted to radians.
 */
static inline float float_radians(float degrees) {
	return degrees * (float) (M_PI / 180.0);
}

/**
 * @return `radians` converted to degrees.
 */
static inline float float_degrees(float radians) {
	return radians * (float) (180.0 / M_PI);
}

/**
 * @return `f` clamped to `[min, max]`.
 */
static inline float float_clamp(float f, float min, float max) {
	return f < min ? min : f > max ? max : f;
}

/**
 * @return `f` clamped to `[0, 1]`.
 */
static inline float float_saturate(float f) {
	return float_clamp(f, 0.f, 1.f);
}

/**
 * @return The linear interpolation of `a` and `b` by `t`.
 */
static inline float float_lerp(float a, float b, float t) {
	return a + (b - a) * t;
}

/**
 * @return The smooth Hermite interpolation of `f` in `[edge0, edge1]`.
 */
static inline float float_smoothstep(float edge0, float edge1, float f) {
	const float t = float_saturate((f - edge0) / (edge1 - edge0));
	return t * t * (3.f - 2.f * t);
}

/**
 * @return The sign of `f`: `1`, `-1`, or `0`.
 */
static inline float float_sign(float f) {
	return (f > 0.f) - (f < 0.f);
}

#pragma mark - float2

/**
 * @brief Two-component single-precision vector. Component layout matches HLSL `float2`.
 */
typedef union {
	float xy[2];
	struct { float x, y; };
} float2;

/**
 * @return A `float2` with the given components.
 */
static inline float2 float2_new(float x, float y) {
	return (float2) { .x = x + 0.f, .y = y + 0.f };
}

static inline float2 float2_zero(void) { return float2_new(0.f, 0.f); }
static inline float2 float2_one(void)  { return float2_new(1.f, 1.f); }

static inline float2 float2_add(float2 a, float2 b)   { return float2_new(a.x+b.x, a.y+b.y); }
static inline float2 float2_sub(float2 a, float2 b)   { return float2_new(a.x-b.x, a.y-b.y); }
static inline float2 float2_mul(float2 a, float2 b)   { return float2_new(a.x*b.x, a.y*b.y); }
static inline float2 float2_scale(float2 v, float s)  { return float2_new(v.x*s, v.y*s); }
static inline float2 float2_negate(float2 v)           { return float2_scale(v, -1.f); }

static inline float  float2_dot(float2 a, float2 b)          { return a.x*b.x + a.y*b.y; }
static inline float  float2_length_squared(float2 v)          { return float2_dot(v, v); }
static inline float  float2_length(float2 v)                  { return sqrtf(float2_length_squared(v)); }
static inline float  float2_distance(float2 a, float2 b)      { return float2_length(float2_sub(a, b)); }

static inline float2 float2_normalize(float2 v) {
	const float l = float2_length(v);
	return l > 0.f ? float2_scale(v, 1.f / l) : v;
}

static inline float2 float2_min(float2 a, float2 b)  { return float2_new(a.x<b.x?a.x:b.x, a.y<b.y?a.y:b.y); }
static inline float2 float2_max(float2 a, float2 b)  { return float2_new(a.x>b.x?a.x:b.x, a.y>b.y?a.y:b.y); }

static inline float2 float2_lerp(float2 a, float2 b, float t) {
	return float2_add(float2_scale(a, 1.f-t), float2_scale(b, t));
}

static inline bool float2_equal(float2 a, float2 b) { return a.x == b.x && a.y == b.y; }

#pragma mark - float3

/**
 * @brief Three-component single-precision vector. Component layout matches HLSL `float3`.
 * @details `.xy` is a swizzle to `float2`.
 */
typedef union {
	float xyz[3];
	struct { float x, y, z; };
	float2 xy;
} float3;

/**
 * @return A `float3` with the given components.
 */
static inline float3 float3_new(float x, float y, float z) {
	return (float3) { .x = x + 0.f, .y = y + 0.f, .z = z + 0.f };
}

static inline float3 float3_zero(void) { return float3_new(0.f, 0.f, 0.f); }
static inline float3 float3_one(void)  { return float3_new(1.f, 1.f, 1.f); }

static inline float3 float3_add(float3 a, float3 b)  { return float3_new(a.x+b.x, a.y+b.y, a.z+b.z); }
static inline float3 float3_sub(float3 a, float3 b)  { return float3_new(a.x-b.x, a.y-b.y, a.z-b.z); }
static inline float3 float3_mul(float3 a, float3 b)  { return float3_new(a.x*b.x, a.y*b.y, a.z*b.z); }
static inline float3 float3_scale(float3 v, float s) { return float3_new(v.x*s, v.y*s, v.z*s); }
static inline float3 float3_negate(float3 v)          { return float3_scale(v, -1.f); }

static inline float  float3_dot(float3 a, float3 b)   { return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline float3 float3_cross(float3 a, float3 b) { return float3_new(a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x); }
static inline float  float3_length_squared(float3 v)  { return float3_dot(v, v); }
static inline float  float3_length(float3 v)           { return sqrtf(float3_length_squared(v)); }
static inline float  float3_distance(float3 a, float3 b) { return float3_length(float3_sub(a, b)); }

static inline float3 float3_normalize(float3 v) {
	const float l = float3_length(v);
	return l > 0.f ? float3_scale(v, 1.f / l) : v;
}

static inline float3 float3_min(float3 a, float3 b) { return float3_new(a.x<b.x?a.x:b.x, a.y<b.y?a.y:b.y, a.z<b.z?a.z:b.z); }
static inline float3 float3_max(float3 a, float3 b) { return float3_new(a.x>b.x?a.x:b.x, a.y>b.y?a.y:b.y, a.z>b.z?a.z:b.z); }

static inline float3 float3_lerp(float3 a, float3 b, float t) {
	return float3_add(float3_scale(a, 1.f-t), float3_scale(b, t));
}

/**
 * @return `v + (add * s)`. Fused multiply-add — useful for accumulation without a temporary.
 */
static inline float3 float3_fma(float3 v, float3 add, float s) {
	return float3_new(fmaf(add.x,s,v.x), fmaf(add.y,s,v.y), fmaf(add.z,s,v.z));
}

/**
 * @return The reflection of `v` about normal `n`. `n` must be normalized.
 */
static inline float3 float3_reflect(float3 v, float3 n) {
	return float3_sub(v, float3_scale(n, 2.f * float3_dot(v, n)));
}

static inline bool float3_equal(float3 a, float3 b) { return a.x == b.x && a.y == b.y && a.z == b.z; }

#pragma mark - float4

/**
 * @brief Four-component single-precision vector. Component layout matches HLSL `float4`.
 * @details `.xyz` swizzles to `float3`; `.xy` and `.zw` swizzle to `float2`.
 */
typedef union {
	float xyzw[4];
	struct { float x, y, z, w; };
	float3 xyz;
	struct { float2 xy; float2 zw; };
} float4;

/**
 * @return A `float4` with the given components.
 */
static inline float4 float4_new(float x, float y, float z, float w) {
	return (float4) { .x = x + 0.f, .y = y + 0.f, .z = z + 0.f, .w = w + 0.f };
}

static inline float4 float4_zero(void) { return float4_new(0.f, 0.f, 0.f, 0.f); }
static inline float4 float4_one(void)  { return float4_new(1.f, 1.f, 1.f, 1.f); }

static inline float4 float4_add(float4 a, float4 b)  { return float4_new(a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w); }
static inline float4 float4_sub(float4 a, float4 b)  { return float4_new(a.x-b.x, a.y-b.y, a.z-b.z, a.w-b.w); }
static inline float4 float4_mul(float4 a, float4 b)  { return float4_new(a.x*b.x, a.y*b.y, a.z*b.z, a.w*b.w); }
static inline float4 float4_scale(float4 v, float s) { return float4_new(v.x*s, v.y*s, v.z*s, v.w*s); }
static inline float4 float4_negate(float4 v)          { return float4_scale(v, -1.f); }

static inline float  float4_dot(float4 a, float4 b)     { return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }
static inline float  float4_length_squared(float4 v)     { return float4_dot(v, v); }
static inline float  float4_length(float4 v)              { return sqrtf(float4_length_squared(v)); }
static inline float  float4_distance(float4 a, float4 b) { return float4_length(float4_sub(a, b)); }

static inline float4 float4_normalize(float4 v) {
	const float l = float4_length(v);
	return l > 0.f ? float4_scale(v, 1.f / l) : v;
}

static inline float4 float4_min(float4 a, float4 b) { return float4_new(a.x<b.x?a.x:b.x, a.y<b.y?a.y:b.y, a.z<b.z?a.z:b.z, a.w<b.w?a.w:b.w); }
static inline float4 float4_max(float4 a, float4 b) { return float4_new(a.x>b.x?a.x:b.x, a.y>b.y?a.y:b.y, a.z>b.z?a.z:b.z, a.w>b.w?a.w:b.w); }

static inline float4 float4_lerp(float4 a, float4 b, float t) {
	return float4_add(float4_scale(a, 1.f-t), float4_scale(b, t));
}

static inline bool float4_equal(float4 a, float4 b) { return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w; }

#pragma mark - float4x4

/**
 * @brief Column-major 4x4 single-precision matrix. Layout matches HLSL `float4x4`.
 * @details `m[col][row]` — the first index is the column, the second is the row.
 * `cols[i]` is the i-th column as a `float4`. `f[col*4+row]` is the flat accessor.
 */
typedef union {
	float  f[16];
	float  m[4][4];
	float4 cols[4];
} float4x4;

/**
 * @return A `float4x4` from a column-major flat array of 16 floats.
 */
static inline float4x4 float4x4_new(const float src[16]) {
	float4x4 m;
	memcpy(m.f, src, sizeof(m.f));
	return m;
}

/**
 * @return A `float4x4` from four column vectors.
 */
static inline float4x4 float4x4_from_cols(float4 c0, float4 c1, float4 c2, float4 c3) {
	return (float4x4) { .cols = { c0, c1, c2, c3 } };
}

/**
 * @return The identity matrix.
 */
static inline float4x4 float4x4_identity(void) {
	return float4x4_new((const float[]) {
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f,
	});
}

/**
 * @return The product `a * b`.
 */
static inline float4x4 float4x4_mul(float4x4 a, float4x4 b) {
	return float4x4_from_cols(
		float4_new(
			a.m[0][0]*b.m[0][0] + a.m[1][0]*b.m[0][1] + a.m[2][0]*b.m[0][2] + a.m[3][0]*b.m[0][3],
			a.m[0][1]*b.m[0][0] + a.m[1][1]*b.m[0][1] + a.m[2][1]*b.m[0][2] + a.m[3][1]*b.m[0][3],
			a.m[0][2]*b.m[0][0] + a.m[1][2]*b.m[0][1] + a.m[2][2]*b.m[0][2] + a.m[3][2]*b.m[0][3],
			a.m[0][3]*b.m[0][0] + a.m[1][3]*b.m[0][1] + a.m[2][3]*b.m[0][2] + a.m[3][3]*b.m[0][3]
		),
		float4_new(
			a.m[0][0]*b.m[1][0] + a.m[1][0]*b.m[1][1] + a.m[2][0]*b.m[1][2] + a.m[3][0]*b.m[1][3],
			a.m[0][1]*b.m[1][0] + a.m[1][1]*b.m[1][1] + a.m[2][1]*b.m[1][2] + a.m[3][1]*b.m[1][3],
			a.m[0][2]*b.m[1][0] + a.m[1][2]*b.m[1][1] + a.m[2][2]*b.m[1][2] + a.m[3][2]*b.m[1][3],
			a.m[0][3]*b.m[1][0] + a.m[1][3]*b.m[1][1] + a.m[2][3]*b.m[1][2] + a.m[3][3]*b.m[1][3]
		),
		float4_new(
			a.m[0][0]*b.m[2][0] + a.m[1][0]*b.m[2][1] + a.m[2][0]*b.m[2][2] + a.m[3][0]*b.m[2][3],
			a.m[0][1]*b.m[2][0] + a.m[1][1]*b.m[2][1] + a.m[2][1]*b.m[2][2] + a.m[3][1]*b.m[2][3],
			a.m[0][2]*b.m[2][0] + a.m[1][2]*b.m[2][1] + a.m[2][2]*b.m[2][2] + a.m[3][2]*b.m[2][3],
			a.m[0][3]*b.m[2][0] + a.m[1][3]*b.m[2][1] + a.m[2][3]*b.m[2][2] + a.m[3][3]*b.m[2][3]
		),
		float4_new(
			a.m[0][0]*b.m[3][0] + a.m[1][0]*b.m[3][1] + a.m[2][0]*b.m[3][2] + a.m[3][0]*b.m[3][3],
			a.m[0][1]*b.m[3][0] + a.m[1][1]*b.m[3][1] + a.m[2][1]*b.m[3][2] + a.m[3][1]*b.m[3][3],
			a.m[0][2]*b.m[3][0] + a.m[1][2]*b.m[3][1] + a.m[2][2]*b.m[3][2] + a.m[3][2]*b.m[3][3],
			a.m[0][3]*b.m[3][0] + a.m[1][3]*b.m[3][1] + a.m[2][3]*b.m[3][2] + a.m[3][3]*b.m[3][3]
		)
	);
}

/**
 * @return A translation matrix for vector `t`.
 */
static inline float4x4 float4x4_translation(float3 t) {
	return float4x4_new((const float[]) {
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		t.x, t.y, t.z, 1.f,
	});
}

/**
 * @return A rotation matrix for `degrees` around the (normalized) `axis`.
 */
static inline float4x4 float4x4_rotation(float degrees, float3 axis) {
	const float r = float_radians(degrees);
	const float c = cosf(r), s = sinf(r), ic = 1.f - c;
	const float3 u = float3_normalize(axis);
	return float4x4_from_cols(
		float4_new(u.x*u.x*ic + c,     u.x*u.y*ic + u.z*s, u.x*u.z*ic - u.y*s, 0.f),
		float4_new(u.x*u.y*ic - u.z*s, u.y*u.y*ic + c,     u.y*u.z*ic + u.x*s, 0.f),
		float4_new(u.x*u.z*ic + u.y*s, u.y*u.z*ic - u.x*s, u.z*u.z*ic + c,     0.f),
		float4_new(0.f, 0.f, 0.f, 1.f)
	);
}

/**
 * @return A non-uniform scale matrix.
 */
static inline float4x4 float4x4_scale3(float3 s) {
	return float4x4_new((const float[]) {
		s.x, 0.f, 0.f, 0.f,
		0.f, s.y, 0.f, 0.f,
		0.f, 0.f, s.z, 0.f,
		0.f, 0.f, 0.f, 1.f,
	});
}

/**
 * @return A uniform scale matrix.
 */
static inline float4x4 float4x4_scale(float s) {
	return float4x4_scale3(float3_new(s, s, s));
}

/**
 * @return A right-handed perspective projection matrix (depth maps to `[-1, 1]`).
 * @param fovy Vertical field-of-view in degrees.
 * @param aspect Viewport width / height.
 * @param znear Near clip distance (positive).
 * @param zfar Far clip distance (positive).
 */
static inline float4x4 float4x4_perspective(float fovy, float aspect, float znear, float zfar) {
	const float f  = 1.f / tanf(float_radians(fovy) * 0.5f);
	const float nf = 1.f / (znear - zfar);
	return float4x4_new((const float[]) {
		f / aspect, 0.f, 0.f,                        0.f,
		0.f,        f,   0.f,                        0.f,
		0.f,        0.f, (znear + zfar) * nf,       -1.f,
		0.f,        0.f, (2.f * znear * zfar) * nf,  0.f,
	});
}

/**
 * @return A right-handed orthographic projection matrix.
 */
static inline float4x4 float4x4_ortho(float left, float right, float bottom, float top, float znear, float zfar) {
	const float lr = 1.f / (left - right);
	const float bt = 1.f / (bottom - top);
	const float nf = 1.f / (znear - zfar);
	return float4x4_new((const float[]) {
		-2.f * lr,              0.f,              0.f,         0.f,
		 0.f,              -2.f * bt,              0.f,         0.f,
		 0.f,               0.f,              2.f * nf,         0.f,
		(left + right) * lr, (top + bottom) * bt, (zfar + znear) * nf, 1.f,
	});
}

/**
 * @return A right-handed view matrix looking from `eye` toward `center`, with `up` as the up vector.
 */
static inline float4x4 float4x4_look_at(float3 eye, float3 center, float3 up) {
	const float3 z = float3_normalize(float3_sub(eye, center));
	const float3 x = float3_normalize(float3_cross(up, z));
	const float3 y = float3_normalize(float3_cross(z, x));
	return float4x4_new((const float[]) {
		       x.x,          y.x,          z.x, 0.f,
		       x.y,          y.y,          z.y, 0.f,
		       x.z,          y.z,          z.z, 0.f,
		-float3_dot(x, eye), -float3_dot(y, eye), -float3_dot(z, eye), 1.f,
	});
}

/**
 * @return The inverse of `m`. Returns the identity if `m` is singular.
 */
static inline float4x4 float4x4_inverse(float4x4 a) {
	const float b00 = a.m[0][0]*a.m[1][1] - a.m[0][1]*a.m[1][0];
	const float b01 = a.m[0][0]*a.m[1][2] - a.m[0][2]*a.m[1][0];
	const float b02 = a.m[0][0]*a.m[1][3] - a.m[0][3]*a.m[1][0];
	const float b03 = a.m[0][1]*a.m[1][2] - a.m[0][2]*a.m[1][1];
	const float b04 = a.m[0][1]*a.m[1][3] - a.m[0][3]*a.m[1][1];
	const float b05 = a.m[0][2]*a.m[1][3] - a.m[0][3]*a.m[1][2];
	const float b06 = a.m[2][0]*a.m[3][1] - a.m[2][1]*a.m[3][0];
	const float b07 = a.m[2][0]*a.m[3][2] - a.m[2][2]*a.m[3][0];
	const float b08 = a.m[2][0]*a.m[3][3] - a.m[2][3]*a.m[3][0];
	const float b09 = a.m[2][1]*a.m[3][2] - a.m[2][2]*a.m[3][1];
	const float b10 = a.m[2][1]*a.m[3][3] - a.m[2][3]*a.m[3][1];
	const float b11 = a.m[2][2]*a.m[3][3] - a.m[2][3]*a.m[3][2];

	float det = b00*b11 - b01*b10 + b02*b09 + b03*b08 - b04*b07 + b05*b06;
	if (!det) {
		return float4x4_identity();
	}
	det = 1.f / det;

	return float4x4_new((const float[]) {
		( a.m[1][1]*b11 - a.m[1][2]*b10 + a.m[1][3]*b09) * det,
		(-a.m[0][1]*b11 + a.m[0][2]*b10 - a.m[0][3]*b09) * det,
		( a.m[3][1]*b05 - a.m[3][2]*b04 + a.m[3][3]*b03) * det,
		(-a.m[2][1]*b05 + a.m[2][2]*b04 - a.m[2][3]*b03) * det,
		(-a.m[1][0]*b11 + a.m[1][2]*b08 - a.m[1][3]*b07) * det,
		( a.m[0][0]*b11 - a.m[0][2]*b08 + a.m[0][3]*b07) * det,
		(-a.m[3][0]*b05 + a.m[3][2]*b02 - a.m[3][3]*b01) * det,
		( a.m[2][0]*b05 - a.m[2][2]*b02 + a.m[2][3]*b01) * det,
		( a.m[1][0]*b10 - a.m[1][1]*b08 + a.m[1][3]*b06) * det,
		(-a.m[0][0]*b10 + a.m[0][1]*b08 - a.m[0][3]*b06) * det,
		( a.m[3][0]*b04 - a.m[3][1]*b02 + a.m[3][3]*b00) * det,
		(-a.m[2][0]*b04 + a.m[2][1]*b02 - a.m[2][3]*b00) * det,
		(-a.m[1][0]*b09 + a.m[1][1]*b07 - a.m[1][2]*b06) * det,
		( a.m[0][0]*b09 - a.m[0][1]*b07 + a.m[0][2]*b06) * det,
		(-a.m[3][0]*b03 + a.m[3][1]*b01 - a.m[3][2]*b00) * det,
		( a.m[2][0]*b03 - a.m[2][1]*b01 + a.m[2][2]*b00) * det,
	});
}

/**
 * @return The `float3` point `v` transformed by matrix `m` (w=1, perspective divide applied).
 */
static inline float3 float4x4_transform(float4x4 m, float3 v) {
	return float3_new(
		v.x*m.m[0][0] + v.y*m.m[1][0] + v.z*m.m[2][0] + m.m[3][0],
		v.x*m.m[0][1] + v.y*m.m[1][1] + v.z*m.m[2][1] + m.m[3][1],
		v.x*m.m[0][2] + v.y*m.m[1][2] + v.z*m.m[2][2] + m.m[3][2]
	);
}

/**
 * @return The `float4` vector `v` transformed by matrix `m`.
 */
static inline float4 float4x4_transform4(float4x4 m, float4 v) {
	return float4_new(
		v.x*m.m[0][0] + v.y*m.m[1][0] + v.z*m.m[2][0] + v.w*m.m[3][0],
		v.x*m.m[0][1] + v.y*m.m[1][1] + v.z*m.m[2][1] + v.w*m.m[3][1],
		v.x*m.m[0][2] + v.y*m.m[1][2] + v.z*m.m[2][2] + v.w*m.m[3][2],
		v.x*m.m[0][3] + v.y*m.m[1][3] + v.z*m.m[2][3] + v.w*m.m[3][3]
	);
}
