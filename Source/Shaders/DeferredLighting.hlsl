struct PSInput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
};

struct PSConstants
{
	float4x4 InvViewProjMatrix;
	float3 CameraWorldPosition;
};

ConstantBuffer<PSConstants> PixelShaderConstants : register(b0);

Texture2DMS<float4> GBufferTexture0 : register(t0);
Texture2DMS<float4> GBufferTexture1 : register(t1);
Texture2DMS<float> DepthBufferTexture : register(t2);
Texture2D<float> ShadowMaskTexture : register(t3);

float4 PS(PSInput PixelShaderInput, uint SampleIndex : SV_SampleIndex) : SV_Target
{
	int2 Coords = PixelShaderInput.Position.xy - 0.5f;

	float4 GBufferData0 = GBufferTexture0.Load(Coords, SampleIndex);
	float4 GBufferData1 = GBufferTexture1.Load(Coords, SampleIndex);

	float4 PixelWorldPosition;

	PixelWorldPosition.xy = PixelShaderInput.TexCoord.xy * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
	PixelWorldPosition.z = DepthBufferTexture.Load(Coords, SampleIndex).x;
	PixelWorldPosition.w = 1.0f;

	PixelWorldPosition = mul(PixelWorldPosition, PixelShaderConstants.InvViewProjMatrix);
	PixelWorldPosition /= PixelWorldPosition.w;

	float3 View = normalize(PixelShaderConstants.CameraWorldPosition - PixelWorldPosition.xyz);

	float ShadowFactor = ShadowMaskTexture.Load(int3(Coords, 0));

	float3 BaseColor = GBufferData0.rgb;
	float3 Normal = 2.0f * GBufferData1.xyz - 1.0f;
	float3 Light = normalize(float3(-1.0f, 1.0f, -1.0f));

	float3 Half = normalize(Light + View);

	return float4(BaseColor * (0.1f + (max(0.0f, dot(Light, Normal)) + ((128.0f + 1.0f) / (2.0f * 3.14f)) * pow(max(0.0f, dot(Half, Normal)), 128.0f)) * ShadowFactor), 1.0f);
}