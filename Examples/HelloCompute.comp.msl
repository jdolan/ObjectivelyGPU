#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Params
{
    float time;
};

struct Particle
{
    float2 position;
};

struct ParticleBuffer
{
    Particle particles[1];
};

constant uint3 gl_WorkGroupSize [[maybe_unused]] = uint3(256u, 1u, 1u);

kernel void main0(constant Params& _27 [[buffer(0)]], device ParticleBuffer& _50 [[buffer(1)]], uint3 gl_GlobalInvocationID [[thread_position_in_grid]])
{
    uint id = gl_GlobalInvocationID.x;
    float angle = ((float(id) / 256.0) * 6.28318023681640625) + _27.time;
    float r = 0.100000001490116119384765625 + ((0.60000002384185791015625 * float(id % 64u)) / 64.0);
    _50.particles[id].position = float2(cos(angle) * r, sin(angle) * r);
}

