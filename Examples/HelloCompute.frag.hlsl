/**
 * HelloCompute fragment shader.
 *
 * Outputs a fixed lavender colour for every particle point.
 *
 * Output: float4 RGBA
 */

float4 fs_main() : SV_Target {
	return float4(0.8, 0.5, 1.0, 1.0);
}
