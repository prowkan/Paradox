struct PSInput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
};

Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

float4 PS(PSInput PixelShaderInput) : SV_Target
{
	return float4(Texture.Sample(Sampler, PixelShaderInput.TexCoord).rgb, 1.0f);
}