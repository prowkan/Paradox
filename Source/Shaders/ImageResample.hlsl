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

VK_BINDING(0, 0) Texture2D InputTexture : register(t0);

VK_BINDING(2, 0) SamplerState BiLinearSampler : register(s0);

VK_LOCATION(0) float4 PS(PSInput PixelShaderInput) : SV_Target
{
	return float4(InputTexture.Sample(BiLinearSampler, PixelShaderInput.TexCoord).rgb, 1.0f);
}