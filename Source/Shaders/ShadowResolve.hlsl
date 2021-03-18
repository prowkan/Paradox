/*struct PSInput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
};

struct PSConstants
{
	float4x4 ReProjMatrices[4];
};

ConstantBuffer<PSConstants> PixelShaderConstants : register(b0);

Texture2D<float> DepthBufferTexture : register(t0);
Texture2D<float> CascadedShadowMaps[4] : register(t1);

SamplerComparisonState ShadowMapSampler : register(s0);

float4 PS(PSInput PixelShaderInput) : SV_Target
{
	int2 Coords = PixelShaderInput.Position.xy - 0.5f;

	float4 Position;

	Position.xy = PixelShaderInput.TexCoord.xy * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
	Position.z = DepthBufferTexture.Load(int3(Coords, 0)).x;
	Position.w = 1.0f;

	float ShadowFactor = 1.0f;

	[unroll]
	for (uint i = 0; i < 4; i++)
	{
		float4 ProjectedPosition = mul(Position, PixelShaderConstants.ReProjMatrices[i]);
		ProjectedPosition /= ProjectedPosition.w;
		ProjectedPosition.xy = ProjectedPosition.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

		if (clamp(ProjectedPosition.x, 0.0f, 1.0f) == ProjectedPosition.x && clamp(ProjectedPosition.y, 0.0f, 1.0f) == ProjectedPosition.y)
		{
			ShadowFactor = 0.0f;

			[unroll]
			for (int j = -2; j <= 2; j++)
			{
				[unroll]
				for (int k = -2; k <= 2; k++)
				{
					ShadowFactor += CascadedShadowMaps[i].SampleCmpLevelZero(ShadowMapSampler, ProjectedPosition.xy, ProjectedPosition.z - 0.00025f, int2(j, k));
				}
			}

			ShadowFactor /= 25.0f;

			break;
		}
	}

	return ShadowFactor;
}*/

struct PSInput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
};

struct PSConstants
{
	float4x4 InvViewProjMatrix;
};

ConstantBuffer<PSConstants> PixelShaderConstants : register(b0);

Texture2D<float> DepthBufferTexture : register(t0);
RaytracingAccelerationStructure Scene : register(t1);

float4 PS(PSInput PixelShaderInput) : SV_Target
{
	int2 Coords = PixelShaderInput.Position.xy - 0.5f;

	float4 PixelWorldPosition;

	PixelWorldPosition.xy = PixelShaderInput.TexCoord.xy * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
	PixelWorldPosition.z = DepthBufferTexture.Load(int3(Coords, 0)).x;
	PixelWorldPosition.w = 1.0f;

	PixelWorldPosition = mul(PixelWorldPosition, PixelShaderConstants.InvViewProjMatrix);
	PixelWorldPosition /= PixelWorldPosition.w;

	float ShadowFactor;

	RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> Query;

	RayDesc Ray;
	Ray.Origin = PixelWorldPosition.xyz;
	//Ray.Origin = float3(PixelWorldPosition.x, 5.0f, PixelWorldPosition.z);
	//Ray.Origin = float3(PixelShaderInput.Position.x, 5.0f, PixelShaderInput.Position.y);
	Ray.TMin = 0.1f;
	Ray.TMax = 1000.0f;
	//Ray.TMax = 1000.0f;
	Ray.Direction = normalize(float3(-1.0f, 1.0f, -1.0f));
	//Ray.Direction = float3(0.0f, -1.0f, 0.0f);

	Query.TraceRayInline(Scene, 0, 0xFFFFFFFF, Ray);

	Query.Proceed();

	
	if (Query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
	{
		ShadowFactor = 0.0f;
	}
	else
	{
		ShadowFactor = 1.0f;
	}

	return ShadowFactor;
}