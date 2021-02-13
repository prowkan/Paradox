struct PSInput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
};

Texture2D GBufferTexture0 : register(t0);
Texture2D GBufferTexture1 : register(t1);

float4 PS(PSInput PixelShaderInput) : SV_Target
{
	int2 Coords = PixelShaderInput.Position.xy - 0.5f;

	float4 GBufferData0 = GBufferTexture0.Load(int3(Coords, 0));
	float4 GBufferData1 = GBufferTexture1.Load(int3(Coords, 0));

	float3 BaseColor = GBufferData0.rgb;
	float3 Normal = 2.0f * GBufferData1.xyz - 1.0f;
	float3 Light = normalize(float3(-1.0f, 1.0f, -1.0f));

	return float4(BaseColor * (0.1f + max(0.0f, dot(Light, Normal))), 1.0f);
}