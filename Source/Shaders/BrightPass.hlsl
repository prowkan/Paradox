#ifdef SPIRV
#define VK_LOCATION(Index) [[vk::location(Index)]]
#define VK_BINDING(Binding, Set) [[vk::binding(Binding, Set)]]
#else
#define VK_LOCATION(Index)
#define VK_BINDING(Binding, Set)
#endif

struct PSInput
{
	float4 Position : SV_Position;
	VK_LOCATION(0) float2 TexCoord : TEXCOORD;
};

VK_BINDING(0, 0) Texture2D HDRFrameBufferTexture : register(t0);
VK_BINDING(1, 0) Texture2D<float> SceneLuminanceTexture : register(t1);

VK_LOCATION(0) float4 PS(PSInput PixelShaderInput) : SV_Target
{
	int2 Coords = trunc(PixelShaderInput.Position.xy);

	float3 HDRColor = HDRFrameBufferTexture.Load(int3(Coords, 0)).rgb;
	float PixelLuminance = SceneLuminanceTexture.Load(int3(Coords, 0)).r;

	if (PixelLuminance < 10.0f) return float4(0.0f, 0.0f, 0.0f, 1.0f);

	return float4(0.25f * HDRColor, 1.0f);
}