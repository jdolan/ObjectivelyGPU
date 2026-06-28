#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Particle
{
    float2 position;
};

struct ParticleBuffer
{
    Particle particles[1];
};

struct main0_out
{
    float4 gl_Position [[position]];
    float gl_PointSize [[point_size]];
};

vertex main0_out main0(const device ParticleBuffer& _21 [[buffer(0)]], uint gl_VertexIndex [[vertex_id]])
{
    main0_out out = {};
    out.gl_Position = float4(_21.particles[int(gl_VertexIndex)].position, 0.0, 1.0);
    out.gl_PointSize = 4.0;
    return out;
}

