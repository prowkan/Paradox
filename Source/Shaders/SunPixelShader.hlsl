struct PSInput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
};

Texture2D SunTexture : register(t0);

SamplerState Sampler : register(s0);

float4 PS(PSInput PixelShaderInput) : SV_Target
{
	float4 SunColor = SunTexture.Sample(Sampler, PixelShaderInput.TexCoord);

	return float4(100.0f * SunColor.rgb, SunColor.a);
}