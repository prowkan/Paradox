struct PSInput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
};

struct PSConstants
{
	float4x4 InvViewProjMatrices[4];
};

ConstantBuffer<PSConstants> PixelShaderConstants : register(b0);

Texture2D<float> DepthBufferTexture : register(t0);
Texture2D<float> CascadedShadowMaps[4] : register(t1);

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
		float4 ProjectedPosition = mul(Position, PixelShaderConstants.InvViewProjMatrices[i]);
		ProjectedPosition /= ProjectedPosition.w;
		ProjectedPosition.xy = ProjectedPosition.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

		if (clamp(ProjectedPosition.x, 0.0f, 1.0f) == ProjectedPosition.x && clamp(ProjectedPosition.y, 0.0f, 1.0f) == ProjectedPosition.y)
		{
			float ShadowMapDepth = CascadedShadowMaps[i].Load(int3(ProjectedPosition.xy * 2048.0f, 0)).x;

			if (ProjectedPosition.z > ShadowMapDepth + 0.00025f) ShadowFactor = 0.0f;

			break;
		}
	}

	return ShadowFactor;
}