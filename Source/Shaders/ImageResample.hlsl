struct PSInput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
};

Texture2D InputTexture : register(t0);

SamplerState BiLinearSampler : register(s0);

float4 PS(PSInput PixelShaderInput) : SV_Target
{
	return float4(InputTexture.Sample(BiLinearSampler, PixelShaderInput.TexCoord).rgb, 1.0f);
}