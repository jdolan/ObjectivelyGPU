/**
 * HelloCompute vertex shader.
 *
 * Reads particle positions from a GPU storage buffer (written by the
 * compute stage) and emits one 4-pixel point per particle.
 *
 * Storage (t0, space2): Particle[] particles  (read-only)
 * Inputs             : SV_VertexID
 * Outputs            : SV_Position, [[point_size]] via vk::builtin
 */

struct Particle {
	float2 position;
};

StructuredBuffer<Particle> particles : register(t0, space2);

struct VSOutput {
	float4 position  : SV_Position;
	[[vk::builtin("PointSize")]]
	float  pointSize : TEXCOORD0;
};

VSOutput vs_main(uint id : SV_VertexID) {
	VSOutput output;
	output.position  = float4(particles[id].position, 0.0, 1.0);
	output.pointSize = 4.0;
	return output;
}
