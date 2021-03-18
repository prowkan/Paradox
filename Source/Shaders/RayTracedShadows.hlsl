struct Constants
{
	float4x4 InvViewProjMatrix;
};

ConstantBuffer<Constants> ShaderConstants : register(b0);

Texture2D<float> DepthBufferTexture : register(t0);
RaytracingAccelerationStructure Scene : register(t1);

RWTexture2D<float> ShadowMaskTexture : register(u0);

struct RayPayload
{
	float ShadowFactor;
};

[shader("anyhit")]
void AnyHitShader(inout RayPayload Payload, in BuiltInTriangleIntersectionAttributes Attributes)
{
	Payload.ShadowFactor = 0.0f;
}

[shader("miss")]
void MissShader(inout RayPayload Payload)
{
	Payload.ShadowFactor = 1.0f;
}

[shader("raygeneration")]
void RayGenShader()
{
	int2 Coords = DispatchRaysIndex().xy;

	float4 PixelWorldPosition;

	PixelWorldPosition.xy = (float2(DispatchRaysIndex().xy) / float2(DispatchRaysDimensions().xy)) * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
	PixelWorldPosition.z = DepthBufferTexture.Load(int3(Coords, 0)).x;
	PixelWorldPosition.w = 1.0f;

	PixelWorldPosition = mul(PixelWorldPosition, ShaderConstants.InvViewProjMatrix);
	PixelWorldPosition /= PixelWorldPosition.w;

	RayDesc Ray;
	Ray.Origin = PixelWorldPosition.xyz;
	Ray.TMin = 0.01f;
	Ray.TMax = 1000.0f;
	Ray.Direction = normalize(float3(-1.0f, 1.0f, -1.0f));
	
	RayPayload Payload;
	Payload.ShadowFactor = 1.0f;

	TraceRay(Scene, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFFFFFFFF, 0, 0, 0, Ray, Payload);

	ShadowMaskTexture[Coords] = Payload.ShadowFactor;
}