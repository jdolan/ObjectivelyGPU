/**
 * HelloCompute compute shader.
 *
 * Animates NUM_PARTICLES (256) particles into a spiral pattern.
 *
 * Uniform (b0, space2): float time
 * Storage (u0, space1): Particle[] particles  (read-write)
 */

struct Particle {
	float2 position;
};

cbuffer Params : register(b0, space2) {
	float time;
};

RWStructuredBuffer<Particle> particles : register(u0, space1);

[numthreads(256, 1, 1)]
void cs_main(uint id : SV_DispatchThreadID) {
	float angle = (float(id) / 256.0) * 6.28318 + time;
	float r     = 0.1 + 0.6 * float(id % 64) / 64.0;
	particles[id].position = float2(cos(angle) * r, sin(angle) * r);
}
