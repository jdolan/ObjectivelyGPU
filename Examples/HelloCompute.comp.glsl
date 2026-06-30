/*
 * ObjectivelyGPU: Object oriented graphics framework for SDL3 and C.
 * Copyright (C) 2014 Jay Dolan <jay@jaydolan.com>
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

/**
 * @file HelloCompute.comp.glsl
 * @brief HelloCompute compute shader.
 *
 * Animates 256 particles into a spiral pattern.
 *
 * Uniform (set=2, b=0): float time
 * Storage (set=1, b=0): Particle[] particles (read-write)
 */

#version 450

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

struct Particle {
	vec2 position;
};

layout(set = 2, binding = 0, std140) uniform Params {
	float time;
};

layout(set = 1, binding = 0, std430) buffer ParticleBuffer {
	Particle particles[];
};

void main() {
	uint  id    = gl_GlobalInvocationID.x;
	float angle = (float(id) / 256.0) * 6.28318 + time;
	float r     = 0.1 + 0.6 * float(id % 64u) / 64.0;
	particles[id].position = vec2(cos(angle) * r, sin(angle) * r);
}
