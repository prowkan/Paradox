struct PSInput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
};

struct PSObjectDataIndicesConstants
{
	uint ObjectIndex;
};

struct PSMaterialDataIndicesConstants
{
	uint TextureIndex;
};

ConstantBuffer<PSObjectDataIndicesConstants> PixelShaderObjectDataIndicesConstants : register(b0, space0);
ConstantBuffer<PSMaterialDataIndicesConstants> PixelShaderMaterialDataIndicesConstants : register(b0, space1);

Texture2D Textures[] : register(t0, space4);
SamplerState Sampler : register(s0, space5);

float4 PS(PSInput PixelShaderInput) : SV_Target
{
	return float4(Textures[PixelShaderMaterialDataIndicesConstants.TextureIndex].Sample(Sampler, PixelShaderInput.TexCoord).rgb, 1.0f);
}