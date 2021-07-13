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
	float4 HDRSceneColor : SV_Target0;
};

PSOutput PS(PSInput PixelShaderInput)
{
	PSOutput PixelShaderOutput;

	PixelShaderOutput.HDRSceneColor = float4(1.0f, 0.0f, 0.0f, 0.5f);

	return PixelShaderOutput;
}