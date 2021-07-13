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
	float4 GBuffer2 : SV_Target2;
};

Texture2D DiffuseMap : register(t0);
Texture2D NormalMap : register(t1);

SamplerState Sampler : register(s0);

PSOutput PS(PSInput PixelShaderInput)
{
	PSOutput PixelShaderOutput;

	float3 BaseColor = DiffuseMap.Sample(Sampler, PixelShaderInput.TexCoord).rgb;
	float3 Normal;
	Normal.xy = 2.0f * NormalMap.Sample(Sampler, PixelShaderInput.TexCoord).xy - 1.0f;
	Normal.z = sqrt(max(0.0f, 1.0f - Normal.x * Normal.x - Normal.y * Normal.y));
	Normal = normalize(Normal.x * normalize(PixelShaderInput.Tangent) + Normal.y * normalize(PixelShaderInput.Binormal) + Normal.z * normalize(PixelShaderInput.Normal));

	PixelShaderOutput.GBuffer0 = float4(BaseColor, 0.0f);
	PixelShaderOutput.GBuffer1 = float4(Normal * 0.5f + 0.5f, 0.0f);
	PixelShaderOutput.GBuffer2 = float4(0.0f, 0.0f, 0.0f, 0.0f);

	return PixelShaderOutput;
}