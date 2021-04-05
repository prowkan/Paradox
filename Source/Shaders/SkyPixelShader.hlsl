struct PSInput
{
	float4 Position : SV_Position;
	[[vk::location(0)]] float2 TexCoord : TEXCOORD;
};

[[vk::binding(1, 0)]] Texture2D SkyTexture : register(t0);

[[vk::binding(2, 0)]] SamplerState Sampler : register(s0);

float4 PS(PSInput PixelShaderInput) : SV_Target
{
	float3 SkyColor = SkyTexture.Sample(Sampler, PixelShaderInput.TexCoord).rgb;

	return float4(SkyColor, 1.0f);
}