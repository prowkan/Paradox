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

struct PointLight
{
	float3 Position;
	float Radius;
	float3 Color;
	float Brightness;
};

cbuffer cb0 : register(b0)
{
	PSConstants PixelShaderConstants;
};

Texture2DMS<float4> GBufferTexture0 : register(t0);
Texture2DMS<float4> GBufferTexture1 : register(t1);
Texture2DMS<float> DepthBufferTexture : register(t2);
Texture2D<float> ShadowMaskTexture : register(t3);
Buffer<uint2> LightClustersBuffer : register(t4);
Buffer<uint> LightIndicesBuffer : register(t5);
StructuredBuffer<PointLight> PointLightsBuffer : register(t6);

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

	uint ClusterCoordX = (PixelShaderInput.Position.x - 0.5f) / 1280.0f * 32.0f;
	uint ClusterCoordY = (PixelShaderInput.Position.y - 0.5f) / 720.0f * 18.0f;

	float Depth = DepthBufferTexture.Load(Coords, SampleIndex).x;

	float Near = 1000.0f;
	float Far = 0.01f;

	float a = Far / (Far - Near);
	float b = (Far * Near) / (Far - Near);

	Depth = b / (a - Depth);

	uint ClusterCoordZ = 24.0f * log2(Depth / Far) / log2(Near / Far);

	uint ClusterCoord = ClusterCoordX + ClusterCoordY * 32 + ClusterCoordZ * 32 * 18;

	uint2 OffsetAndCount = LightClustersBuffer.Load(ClusterCoord);

	uint Offset = OffsetAndCount.x;
	uint Count = OffsetAndCount.y;

	float3 Color = BaseColor * (0.1f + (max(0.0f, dot(Light, Normal)) + ((128.0f + 1.0f) / (2.0f * 3.14f)) * pow(max(0.0f, dot(Half, Normal)), 128.0f)) * ShadowFactor);

	[loop]
	for (uint i = 0; i < Count; i++)
	{
		uint PointLightIndex = LightIndicesBuffer.Load(Offset + i);
		PointLight pointLight = PointLightsBuffer.Load(PointLightIndex);

		Light = normalize(pointLight.Position - PixelWorldPosition.xyz);

		Color += BaseColor * max(0.0f, dot(Light, Normal)) * pointLight.Color * pointLight.Brightness * (length(pointLight.Position - PixelWorldPosition.xyz) < pointLight.Radius);
	}

	return float4(Color, 1.0f);
}