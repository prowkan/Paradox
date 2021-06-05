struct PSInput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Binormal : BINORMAL;
};

struct PSOutput
{
	float4 GBuffer0 : SV_Target0;
	float4 GBuffer1 : SV_Target1;
};

cbuffer DrawData : register(b0, space0)
{
	uint4 DataIndices0;
	uint DataIndices1;
}

[RootSignature("RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | SAMPLER_HEAP_DIRECTLY_INDEXED), CBV(b0, space = 0, visibility = SHADER_VISIBILITY_ALL)")]
PSOutput PS(PSInput PixelShaderInput)
{
	PSOutput PixelShaderOutput;

	Texture2D<float4> DiffuseMap = ResourceDescriptorHeap[DataIndices0.z];
	Texture2D<float4> NormalMap = ResourceDescriptorHeap[DataIndices0.w];

	SamplerState Sampler = SamplerDescriptorHeap[DataIndices1];

	float3 BaseColor = DiffuseMap.Sample(Sampler, PixelShaderInput.TexCoord).rgb;
	float3 Normal;
	Normal.xy = 2.0f * NormalMap.Sample(Sampler, PixelShaderInput.TexCoord).xy - 1.0f;
	Normal.z = sqrt(max(0.0f, 1.0f - Normal.x * Normal.x - Normal.y * Normal.y));
	Normal = normalize(Normal.x * normalize(PixelShaderInput.Tangent) + Normal.y * normalize(PixelShaderInput.Binormal) + Normal.z * normalize(PixelShaderInput.Normal));

	PixelShaderOutput.GBuffer0 = float4(BaseColor, 0.0f);
	PixelShaderOutput.GBuffer1 = float4(Normal * 0.5f + 0.5f, 0.0f);

	/*PixelShaderOutput.GBuffer0 = float4(0.0f, 0.0f, 0.0f, 0.0f);
	PixelShaderOutput.GBuffer1 = float4(0.0f, 0.0f, 0.0f, 0.0f);*/

	return PixelShaderOutput;
}