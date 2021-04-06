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

VK_BINDING(0, 0) ConstantBuffer<PSConstants> PixelShaderConstants : register(b0);

VK_BINDING(1, 0) Texture2DMS<float4> GBufferTexture0 : register(t0);
VK_BINDING(2, 0) Texture2DMS<float4> GBufferTexture1 : register(t1);
VK_BINDING(3, 0) Texture2DMS<float> DepthBufferTexture : register(t2);
VK_BINDING(4, 0) Texture2D<float> ShadowMaskTexture : register(t3);
VK_BINDING(5, 0) Buffer<uint2> LightClustersBuffer : register(t4);
VK_BINDING(6, 0) Buffer<uint> LightIndicesBuffer : register(t5);
VK_BINDING(7, 0) StructuredBuffer<PointLight> PointLightsBuffer : register(t6);

VK_LOCATION(0) float4 PS(PSInput PixelShaderInput, uint SampleIndex : SV_SampleIndex) : SV_Target
{
	int2 Coords = trunc(PixelShaderInput.Position.xy);

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
	//return float4(PixelShaderInput.Position.x, PixelShaderInput.Position.y, SampleIndex, 1.0f);
}