struct PSInput
{
	float4 Position : SV_Position;
	[[vk::location(0)]] float2 TexCoord : TEXCOORD;
};

[[vk::binding(0, 0)]] Texture2D InputTexture : register(t0);

[[vk::binding(2, 0)]] SamplerState BiLinearSampler : register(s0);

float4 PS(PSInput PixelShaderInput) : SV_Target
{
	return float4(InputTexture.Sample(BiLinearSampler, PixelShaderInput.TexCoord).rgb, 1.0f);
}