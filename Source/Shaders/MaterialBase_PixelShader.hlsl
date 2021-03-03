struct PSInput
{
	[[vk::location(0)]] float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
};

[[vk::binding(0, 1)]] Texture2D Texture : register(t0);
[[vk::binding(0, 2)]] SamplerState Sampler : register(s0);

[[vk::location(0)]] float4 PS(PSInput PixelShaderInput) : SV_Target
{
	return float4(Texture.Sample(Sampler, PixelShaderInput.TexCoord).rgb, 1.0f);
}