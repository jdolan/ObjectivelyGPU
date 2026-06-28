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

#include <math.h>
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

#pragma mark - vec2

/**
 * @brief Two-component single-precision vector. Component layout matches HLSL `vec2`.
 */
typedef union {
	float xy[2];
	struct { float x, y; };
} vec2;

/**
 * @return A `vec2` with the given components.
 */
static inline vec2 vec2_new(float x, float y) {
	return (vec2) { .x = x + 0.f, .y = y + 0.f };
}

static inline vec2 vec2_zero(void) { return vec2_new(0.f, 0.f); }
static inline vec2 vec2_one(void)  { return vec2_new(1.f, 1.f); }

static inline vec2 vec2_add(vec2 a, vec2 b)   { return vec2_new(a.x+b.x, a.y+b.y); }
static inline vec2 vec2_sub(vec2 a, vec2 b)   { return vec2_new(a.x-b.x, a.y-b.y); }
static inline vec2 vec2_mul(vec2 a, vec2 b)   { return vec2_new(a.x*b.x, a.y*b.y); }
static inline vec2 vec2_scale(vec2 v, float s)  { return vec2_new(v.x*s, v.y*s); }
static inline vec2 vec2_negate(vec2 v)           { return vec2_scale(v, -1.f); }

static inline float  vec2_dot(vec2 a, vec2 b)          { return a.x*b.x + a.y*b.y; }
static inline float  vec2_length_squared(vec2 v)          { return vec2_dot(v, v); }
static inline float  vec2_length(vec2 v)                  { return sqrtf(vec2_length_squared(v)); }
static inline float  vec2_distance(vec2 a, vec2 b)      { return vec2_length(vec2_sub(a, b)); }

static inline vec2 vec2_normalize(vec2 v) {
	const float l = vec2_length(v);
	return l > 0.f ? vec2_scale(v, 1.f / l) : v;
}

static inline vec2 vec2_min(vec2 a, vec2 b)  { return vec2_new(a.x<b.x?a.x:b.x, a.y<b.y?a.y:b.y); }
static inline vec2 vec2_max(vec2 a, vec2 b)  { return vec2_new(a.x>b.x?a.x:b.x, a.y>b.y?a.y:b.y); }

static inline vec2 vec2_lerp(vec2 a, vec2 b, float t) {
	return vec2_add(vec2_scale(a, 1.f-t), vec2_scale(b, t));
}

static inline bool vec2_equal(vec2 a, vec2 b) { return a.x == b.x && a.y == b.y; }

#pragma mark - vec3

/**
 * @brief Three-component single-precision vector. Component layout matches HLSL `vec3`.
 * @details `.xy` is a swizzle to `vec2`.
 */
typedef union {
	float xyz[3];
	struct { float x, y, z; };
	vec2 xy;
} vec3;

/**
 * @return A `vec3` with the given components.
 */
static inline vec3 vec3_new(float x, float y, float z) {
	return (vec3) { .x = x + 0.f, .y = y + 0.f, .z = z + 0.f };
}

static inline vec3 vec3_zero(void) { return vec3_new(0.f, 0.f, 0.f); }
static inline vec3 vec3_one(void)  { return vec3_new(1.f, 1.f, 1.f); }

static inline vec3 vec3_add(vec3 a, vec3 b)  { return vec3_new(a.x+b.x, a.y+b.y, a.z+b.z); }
static inline vec3 vec3_sub(vec3 a, vec3 b)  { return vec3_new(a.x-b.x, a.y-b.y, a.z-b.z); }
static inline vec3 vec3_mul(vec3 a, vec3 b)  { return vec3_new(a.x*b.x, a.y*b.y, a.z*b.z); }
static inline vec3 vec3_scale(vec3 v, float s) { return vec3_new(v.x*s, v.y*s, v.z*s); }
static inline vec3 vec3_negate(vec3 v)          { return vec3_scale(v, -1.f); }

static inline float  vec3_dot(vec3 a, vec3 b)   { return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline vec3 vec3_cross(vec3 a, vec3 b) { return vec3_new(a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x); }
static inline float  vec3_length_squared(vec3 v)  { return vec3_dot(v, v); }
static inline float  vec3_length(vec3 v)           { return sqrtf(vec3_length_squared(v)); }
static inline float  vec3_distance(vec3 a, vec3 b) { return vec3_length(vec3_sub(a, b)); }

static inline vec3 vec3_normalize(vec3 v) {
	const float l = vec3_length(v);
	return l > 0.f ? vec3_scale(v, 1.f / l) : v;
}

static inline vec3 vec3_min(vec3 a, vec3 b) { return vec3_new(a.x<b.x?a.x:b.x, a.y<b.y?a.y:b.y, a.z<b.z?a.z:b.z); }
static inline vec3 vec3_max(vec3 a, vec3 b) { return vec3_new(a.x>b.x?a.x:b.x, a.y>b.y?a.y:b.y, a.z>b.z?a.z:b.z); }

static inline vec3 vec3_lerp(vec3 a, vec3 b, float t) {
	return vec3_add(vec3_scale(a, 1.f-t), vec3_scale(b, t));
}

/**
 * @return `v + (add * s)`. Fused multiply-add — useful for accumulation without a temporary.
 */
static inline vec3 vec3_fma(vec3 v, vec3 add, float s) {
	return vec3_new(fmaf(add.x,s,v.x), fmaf(add.y,s,v.y), fmaf(add.z,s,v.z));
}

/**
 * @return The reflection of `v` about normal `n`. `n` must be normalized.
 */
static inline vec3 vec3_reflect(vec3 v, vec3 n) {
	return vec3_sub(v, vec3_scale(n, 2.f * vec3_dot(v, n)));
}

static inline bool vec3_equal(vec3 a, vec3 b) { return a.x == b.x && a.y == b.y && a.z == b.z; }

#pragma mark - vec4

/**
 * @brief Four-component single-precision vector. Component layout matches HLSL `vec4`.
 * @details `.xyz` swizzles to `vec3`; `.xy` and `.zw` swizzle to `vec2`.
 */
typedef union {
	float xyzw[4];
	struct { float x, y, z, w; };
	vec3 xyz;
	struct { vec2 xy; vec2 zw; };
} vec4;

/**
 * @return A `vec4` with the given components.
 */
static inline vec4 vec4_new(float x, float y, float z, float w) {
	return (vec4) { .x = x + 0.f, .y = y + 0.f, .z = z + 0.f, .w = w + 0.f };
}

static inline vec4 vec4_zero(void) { return vec4_new(0.f, 0.f, 0.f, 0.f); }
static inline vec4 vec4_one(void)  { return vec4_new(1.f, 1.f, 1.f, 1.f); }

static inline vec4 vec4_add(vec4 a, vec4 b)  { return vec4_new(a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w); }
static inline vec4 vec4_sub(vec4 a, vec4 b)  { return vec4_new(a.x-b.x, a.y-b.y, a.z-b.z, a.w-b.w); }
static inline vec4 vec4_mul(vec4 a, vec4 b)  { return vec4_new(a.x*b.x, a.y*b.y, a.z*b.z, a.w*b.w); }
static inline vec4 vec4_scale(vec4 v, float s) { return vec4_new(v.x*s, v.y*s, v.z*s, v.w*s); }
static inline vec4 vec4_negate(vec4 v)          { return vec4_scale(v, -1.f); }

static inline float  vec4_dot(vec4 a, vec4 b)     { return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }
static inline float  vec4_length_squared(vec4 v)     { return vec4_dot(v, v); }
static inline float  vec4_length(vec4 v)              { return sqrtf(vec4_length_squared(v)); }
static inline float  vec4_distance(vec4 a, vec4 b) { return vec4_length(vec4_sub(a, b)); }

static inline vec4 vec4_normalize(vec4 v) {
	const float l = vec4_length(v);
	return l > 0.f ? vec4_scale(v, 1.f / l) : v;
}

static inline vec4 vec4_min(vec4 a, vec4 b) { return vec4_new(a.x<b.x?a.x:b.x, a.y<b.y?a.y:b.y, a.z<b.z?a.z:b.z, a.w<b.w?a.w:b.w); }
static inline vec4 vec4_max(vec4 a, vec4 b) { return vec4_new(a.x>b.x?a.x:b.x, a.y>b.y?a.y:b.y, a.z>b.z?a.z:b.z, a.w>b.w?a.w:b.w); }

static inline vec4 vec4_lerp(vec4 a, vec4 b, float t) {
	return vec4_add(vec4_scale(a, 1.f-t), vec4_scale(b, t));
}

static inline bool vec4_equal(vec4 a, vec4 b) { return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w; }

#pragma mark - mat4

/**
 * @brief Column-major 4x4 single-precision matrix. Layout matches HLSL `mat4`.
 * @details `m[col][row]` — the first index is the column, the second is the row.
 * `cols[i]` is the i-th column as a `vec4`. `f[col*4+row]` is the flat accessor.
 */
typedef union {
	float  f[16];
	float  m[4][4];
	vec4 cols[4];
} mat4;

/**
 * @return A `mat4` from a column-major flat array of 16 floats.
 */
static inline mat4 mat4_new(const float src[16]) {
	mat4 m;
	memcpy(m.f, src, sizeof(m.f));
	return m;
}

/**
 * @return A `mat4` from four column vectors.
 */
static inline mat4 mat4_from_cols(vec4 c0, vec4 c1, vec4 c2, vec4 c3) {
	return (mat4) { .cols = { c0, c1, c2, c3 } };
}

/**
 * @return The identity matrix.
 */
static inline mat4 mat4_identity(void) {
	return mat4_new((const float[]) {
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f,
	});
}

/**
 * @return The product `a * b`.
 */
static inline mat4 mat4_mul(mat4 a, mat4 b) {
	return mat4_from_cols(
		vec4_new(
			a.m[0][0]*b.m[0][0] + a.m[1][0]*b.m[0][1] + a.m[2][0]*b.m[0][2] + a.m[3][0]*b.m[0][3],
			a.m[0][1]*b.m[0][0] + a.m[1][1]*b.m[0][1] + a.m[2][1]*b.m[0][2] + a.m[3][1]*b.m[0][3],
			a.m[0][2]*b.m[0][0] + a.m[1][2]*b.m[0][1] + a.m[2][2]*b.m[0][2] + a.m[3][2]*b.m[0][3],
			a.m[0][3]*b.m[0][0] + a.m[1][3]*b.m[0][1] + a.m[2][3]*b.m[0][2] + a.m[3][3]*b.m[0][3]
		),
		vec4_new(
			a.m[0][0]*b.m[1][0] + a.m[1][0]*b.m[1][1] + a.m[2][0]*b.m[1][2] + a.m[3][0]*b.m[1][3],
			a.m[0][1]*b.m[1][0] + a.m[1][1]*b.m[1][1] + a.m[2][1]*b.m[1][2] + a.m[3][1]*b.m[1][3],
			a.m[0][2]*b.m[1][0] + a.m[1][2]*b.m[1][1] + a.m[2][2]*b.m[1][2] + a.m[3][2]*b.m[1][3],
			a.m[0][3]*b.m[1][0] + a.m[1][3]*b.m[1][1] + a.m[2][3]*b.m[1][2] + a.m[3][3]*b.m[1][3]
		),
		vec4_new(
			a.m[0][0]*b.m[2][0] + a.m[1][0]*b.m[2][1] + a.m[2][0]*b.m[2][2] + a.m[3][0]*b.m[2][3],
			a.m[0][1]*b.m[2][0] + a.m[1][1]*b.m[2][1] + a.m[2][1]*b.m[2][2] + a.m[3][1]*b.m[2][3],
			a.m[0][2]*b.m[2][0] + a.m[1][2]*b.m[2][1] + a.m[2][2]*b.m[2][2] + a.m[3][2]*b.m[2][3],
			a.m[0][3]*b.m[2][0] + a.m[1][3]*b.m[2][1] + a.m[2][3]*b.m[2][2] + a.m[3][3]*b.m[2][3]
		),
		vec4_new(
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
static inline mat4 mat4_translation(vec3 t) {
	return mat4_new((const float[]) {
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		t.x, t.y, t.z, 1.f,
	});
}

/**
 * @return A rotation matrix for `degrees` around the (normalized) `axis`.
 */
static inline mat4 mat4_rotation(float degrees, vec3 axis) {
	const float r = float_radians(degrees);
	const float c = cosf(r), s = sinf(r), ic = 1.f - c;
	const vec3 u = vec3_normalize(axis);
	return mat4_from_cols(
		vec4_new(u.x*u.x*ic + c,     u.x*u.y*ic + u.z*s, u.x*u.z*ic - u.y*s, 0.f),
		vec4_new(u.x*u.y*ic - u.z*s, u.y*u.y*ic + c,     u.y*u.z*ic + u.x*s, 0.f),
		vec4_new(u.x*u.z*ic + u.y*s, u.y*u.z*ic - u.x*s, u.z*u.z*ic + c,     0.f),
		vec4_new(0.f, 0.f, 0.f, 1.f)
	);
}

/**
 * @return A non-uniform scale matrix.
 */
static inline mat4 mat4_scale3(vec3 s) {
	return mat4_new((const float[]) {
		s.x, 0.f, 0.f, 0.f,
		0.f, s.y, 0.f, 0.f,
		0.f, 0.f, s.z, 0.f,
		0.f, 0.f, 0.f, 1.f,
	});
}

/**
 * @return A uniform scale matrix.
 */
static inline mat4 mat4_scale(float s) {
	return mat4_scale3(vec3_new(s, s, s));
}

/**
 * @return A right-handed perspective projection matrix (depth maps to `[-1, 1]`).
 * @param fovy Vertical field-of-view in degrees.
 * @param aspect Viewport width / height.
 * @param znear Near clip distance (positive).
 * @param zfar Far clip distance (positive).
 */
static inline mat4 mat4_perspective(float fovy, float aspect, float znear, float zfar) {
	const float f  = 1.f / tanf(float_radians(fovy) * 0.5f);
	const float nf = 1.f / (znear - zfar);
	return mat4_new((const float[]) {
		f / aspect, 0.f, 0.f,                        0.f,
		0.f,        f,   0.f,                        0.f,
		0.f,        0.f, (znear + zfar) * nf,       -1.f,
		0.f,        0.f, (2.f * znear * zfar) * nf,  0.f,
	});
}

/**
 * @return A right-handed orthographic projection matrix.
 */
static inline mat4 mat4_ortho(float left, float right, float bottom, float top, float znear, float zfar) {
	const float lr = 1.f / (left - right);
	const float bt = 1.f / (bottom - top);
	const float nf = 1.f / (znear - zfar);
	return mat4_new((const float[]) {
		-2.f * lr,              0.f,              0.f,         0.f,
		 0.f,              -2.f * bt,              0.f,         0.f,
		 0.f,               0.f,              2.f * nf,         0.f,
		(left + right) * lr, (top + bottom) * bt, (zfar + znear) * nf, 1.f,
	});
}

/**
 * @return A right-handed view matrix looking from `eye` toward `center`, with `up` as the up vector.
 */
static inline mat4 mat4_look_at(vec3 eye, vec3 center, vec3 up) {
	const vec3 z = vec3_normalize(vec3_sub(eye, center));
	const vec3 x = vec3_normalize(vec3_cross(up, z));
	const vec3 y = vec3_normalize(vec3_cross(z, x));
	return mat4_new((const float[]) {
		       x.x,          y.x,          z.x, 0.f,
		       x.y,          y.y,          z.y, 0.f,
		       x.z,          y.z,          z.z, 0.f,
		-vec3_dot(x, eye), -vec3_dot(y, eye), -vec3_dot(z, eye), 1.f,
	});
}

/**
 * @return The inverse of `m`. Returns the identity if `m` is singular.
 */
static inline mat4 mat4_inverse(mat4 a) {
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
		return mat4_identity();
	}
	det = 1.f / det;

	return mat4_new((const float[]) {
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
 * @return The `vec3` point `v` transformed by matrix `m` (w=1, perspective divide applied).
 */
static inline vec3 mat4_transform(mat4 m, vec3 v) {
	return vec3_new(
		v.x*m.m[0][0] + v.y*m.m[1][0] + v.z*m.m[2][0] + m.m[3][0],
		v.x*m.m[0][1] + v.y*m.m[1][1] + v.z*m.m[2][1] + m.m[3][1],
		v.x*m.m[0][2] + v.y*m.m[1][2] + v.z*m.m[2][2] + m.m[3][2]
	);
}

/**
 * @return The `vec4` vector `v` transformed by matrix `m`.
 */
static inline vec4 mat4_transform4(mat4 m, vec4 v) {
	return vec4_new(
		v.x*m.m[0][0] + v.y*m.m[1][0] + v.z*m.m[2][0] + v.w*m.m[3][0],
		v.x*m.m[0][1] + v.y*m.m[1][1] + v.z*m.m[2][1] + v.w*m.m[3][1],
		v.x*m.m[0][2] + v.y*m.m[1][2] + v.z*m.m[2][2] + v.w*m.m[3][2],
		v.x*m.m[0][3] + v.y*m.m[1][3] + v.z*m.m[2][3] + v.w*m.m[3][3]
	);
}

