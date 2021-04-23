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

SamplerState Sampler : register(s0, space0);

float4 PS(PSInput PixelShaderInput) : SV_Target
{
	Texture2D Texture = ResourceDescriptorHeap[20001 + PixelShaderMaterialDataIndicesConstants.TextureIndex];

	return float4(Texture.Sample(Sampler, PixelShaderInput.TexCoord).rgb, 1.0f);
}