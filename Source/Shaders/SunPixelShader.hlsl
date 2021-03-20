struct PSInput
{
	float4 Position : SV_Position;
	[[vk::location(0)]] float2 TexCoord : TEXCOORD;
};

[[vk::binding(1, 0)]] Texture2D SunTexture : register(t0);

[[vk::binding(2, 0)]] SamplerState Sampler : register(s0);

float4 PS(PSInput PixelShaderInput) : SV_Target
{
	float4 SunColor = SunTexture.Sample(Sampler, PixelShaderInput.TexCoord);

	return float4(100.0f * SunColor.rgb, SunColor.a);
}