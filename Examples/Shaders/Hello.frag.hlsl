/**
 * Hello example fragment shader.
 *
 * Inputs (TEXCOORD0): float4 color from vertex stage
 * Output            : float4 RGBA
 */

struct PSInput {
	float4 color : TEXCOORD0;
};

float4 fs_main(PSInput input) : SV_Target {
	return input.color;
}
