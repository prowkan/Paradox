struct PSInput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
};

struct PSCameraConstants
{
	float4x4 ViewProjMatrix;
	float4x4 InvViewProjMatrix;
	float3 CameraWorldPosition;
	float NearZ, FarZ;
};

struct PSLightingConstants
{
	float3 MainLightDirection;
};

struct PSRenderTargetConstants
{
	float2 RenderTargetResolution;
};

struct PSClusteredShadingConstants
{
	uint3 ClustersCounts;
};

struct PointLight
{
	float3 Position;
	float Radius;
	float3 Color;
	float Brightness;
};

ConstantBuffer<PSCameraConstants> PixelShaderCameraConstants : register(b0);
ConstantBuffer<PSLightingConstants> PixelShaderLightingConstants : register(b1);
ConstantBuffer<PSRenderTargetConstants> PixelRenderTargetConstants : register(b2);
ConstantBuffer<PSClusteredShadingConstants> PixelShaderClusteredShadingConstants : register(b3);

Texture2DMS<float4> GBufferTexture0 : register(t0);
Texture2DMS<float4> GBufferTexture1 : register(t1);
Texture2DMS<float4> GBufferTexture2 : register(t2);
Texture2DMS<float> DepthBufferTexture : register(t3);
Texture2D<float> ShadowMaskTexture : register(t4);
Buffer<uint2> LightClustersBuffer : register(t5);
Buffer<uint> LightIndicesBuffer : register(t6);
StructuredBuffer<PointLight> PointLightsBuffer : register(t7);

float4 PS(PSInput PixelShaderInput, uint SampleIndex : SV_SampleIndex) : SV_Target
{
	int2 Coords = trunc(PixelShaderInput.Position.xy);

	float4 GBufferData0 = GBufferTexture0.Load(Coords, SampleIndex);
	float4 GBufferData1 = GBufferTexture1.Load(Coords, SampleIndex);

	float4 PixelWorldPosition;

	PixelWorldPosition.xy = PixelShaderInput.TexCoord.xy * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
	PixelWorldPosition.z = DepthBufferTexture.Load(Coords, SampleIndex).x;
	PixelWorldPosition.w = 1.0f;

	PixelWorldPosition = mul(PixelWorldPosition, PixelShaderCameraConstants.InvViewProjMatrix);
	PixelWorldPosition /= PixelWorldPosition.w;

	float3 View = normalize(PixelShaderCameraConstants.CameraWorldPosition - PixelWorldPosition.xyz);

	float ShadowFactor = ShadowMaskTexture.Load(int3(Coords, 0));

	float3 BaseColor = GBufferData0.rgb;
	float3 Normal = 2.0f * GBufferData1.xyz - 1.0f;
	float3 Light = normalize(float3(-1.0f, 1.0f, -1.0f));

	float3 Half = normalize(Light + View);

	uint ClusterCoordX = trunc(PixelShaderInput.Position.x) / PixelRenderTargetConstants.RenderTargetResolution.x * float(PixelShaderClusteredShadingConstants.ClustersCounts.x);
	uint ClusterCoordY = trunc(PixelShaderInput.Position.y) / PixelRenderTargetConstants.RenderTargetResolution.y * float(PixelShaderClusteredShadingConstants.ClustersCounts.y);

	float Depth = DepthBufferTexture.Load(Coords, SampleIndex).x;

	Depth = ((PixelShaderCameraConstants.FarZ * PixelShaderCameraConstants.NearZ) / (PixelShaderCameraConstants.FarZ - PixelShaderCameraConstants.NearZ)) / ((PixelShaderCameraConstants.FarZ / (PixelShaderCameraConstants.FarZ - PixelShaderCameraConstants.NearZ)) - Depth);

	uint ClusterCoordZ = float(PixelShaderClusteredShadingConstants.ClustersCounts.z) * log2(Depth / PixelShaderCameraConstants.FarZ) / log2(PixelShaderCameraConstants.NearZ / PixelShaderCameraConstants.FarZ);

	uint ClusterCoord = ClusterCoordX + ClusterCoordY * PixelShaderClusteredShadingConstants.ClustersCounts.x + ClusterCoordZ * PixelShaderClusteredShadingConstants.ClustersCounts.x * PixelShaderClusteredShadingConstants.ClustersCounts.y;

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