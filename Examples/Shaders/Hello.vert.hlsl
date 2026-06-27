/**
 * Hello example vertex shader.
 *
 * Inputs  (location 0): float3 position
 *         (location 1): float3 color
 * Uniform (b0, space1): float4x4 ModelViewProj
 * Outputs             : float4 color (passed through), SV_Position
 */

cbuffer UBO : register(b0, space1) {
	float4x4 ModelViewProj;
};

struct VSInput {
	float3 position : TEXCOORD0;
	float3 color    : TEXCOORD1;
};

struct VSOutput {
	float4 color    : TEXCOORD0;
	float4 position : SV_Position;
};

VSOutput vs_main(VSInput input) {
	VSOutput output;
	output.color    = float4(input.color, 1.0);
	output.position = mul(ModelViewProj, float4(input.position, 1.0));
	return output;
}
