[[vk::binding(0, 0)]] Texture2D HDRColorTexture : register(t0);
[[vk::binding(1, 0)]] RWTexture2D<float> OutputTexture : register(u0);

[numthreads(16, 16, 1)]
void CS(uint3 DispatchThreadID : SV_DispatchThreadID)
{
	float3 HDRColor = HDRColorTexture[DispatchThreadID.xy].rgb;
	float Luminance = 0.2126 * HDRColor.r + 0.7152 * HDRColor.g + 0.0722 * HDRColor.b;
	OutputTexture[DispatchThreadID.xy] = Luminance;
}