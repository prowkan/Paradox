#ifdef SPIRV
#define VK_LOCATION(Index) [[vk::location(Index)]]
#define VK_BINDING(Binding, Set) [[vk::binding(Binding, Set)]]
#else
#define VK_LOCATION(Index)
#define VK_BINDING(Binding, Set)
#endif

struct PSInput
{
	float4 Position : SV_Position;
	VK_LOCATION(0) float2 TexCoord : TEXCOORD;
	VK_LOCATION(1) float3 Normal : NORMAL;
	VK_LOCATION(2) float3 Tangent : TANGENT;
	VK_LOCATION(3) float3 Binormal : BINORMAL;
};

struct PSOutput
{
	VK_LOCATION(0) float4 GBuffer0 : SV_Target0;
	VK_LOCATION(1) float4 GBuffer1 : SV_Target1;
};

VK_BINDING(0, 1) Texture2D DiffuseMap : register(t0);
VK_BINDING(1, 1) Texture2D NormalMap : register(t1);

VK_BINDING(0, 2) SamplerState Sampler : register(s0);

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

	return PixelShaderOutput;
}