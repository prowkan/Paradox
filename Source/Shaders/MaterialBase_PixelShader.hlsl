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

Texture2D DiffuseMap : register(t0);
Texture2D NormalMap : register(t1);

SamplerState Sampler : register(s0);

PSOutput PS(PSInput PixelShaderInput)
{
	PSOutput PixelShaderOutput;

	float3 BaseColor = DiffuseMap.Sample(Sampler, PixelShaderInput.TexCoord).rgb;
	//float3 Light = normalize(float3(-1.0f, 1.0f, -1.0f));
	float3 Normal;
	Normal.xy = 2.0f * NormalMap.Sample(Sampler, PixelShaderInput.TexCoord).xy - 1.0f;
	Normal.z = sqrt(max(0.0f, 1.0f - Normal.x * Normal.x - Normal.y * Normal.y));
	Normal = normalize(Normal.x * normalize(PixelShaderInput.Tangent) + Normal.y * normalize(PixelShaderInput.Binormal) + Normal.z * normalize(PixelShaderInput.Normal));

	//return float4(BaseColor * (0.1f + max(0.0f, dot(Light, Normal))), 1.0f);

	PixelShaderOutput.GBuffer0 = float4(BaseColor, 0.0f);
	PixelShaderOutput.GBuffer1 = float4(Normal * 0.5f + 0.5f, 0.0f);

	return PixelShaderOutput;
}