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

VK_LOCATION(0) float4 PS(PSInput PixelShaderInput) : SV_Target
{
	int2 Coords = trunc(PixelShaderInput.Position.xy);

	float GaussianWeights[11] =
	{
		0.0093,
		0.028002,
		0.065984,
		0.121703,
		0.175713,
		0.198596,
		0.175713,
		0.121703,
		0.065984,
		0.028002,
		0.0093
	};

	float3 BlurredColor = float3(0.0f, 0.0f, 0.0f);

	[unroll]
	for (int i = -5; i <= 5; i++)
	{
		BlurredColor += GaussianWeights[i + 5] * InputTexture.Load(int3(Coords.x + i, Coords.y, 0)).rgb;
	}

	return float4(BlurredColor, 1.0f);
}