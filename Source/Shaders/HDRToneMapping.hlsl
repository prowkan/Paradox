struct PSInput
{
	float4 Position : SV_Position;
	[[vk::location(0)]] float2 TexCoord : TEXCOORD;
};

[[vk::binding(0, 0)]] Texture2DMS<float4> HDRFrameBufferTexture : register(t0);
[[vk::binding(1, 0)]] Texture2D HDRBloomTexture : register(t1);

float3 ACESToneMappingOperator(float3 Color)
{
	float A = 2.51f;
	float B = 0.03f;
	float C = 2.43f;
	float D = 0.59f;
	float E = 0.14f;

	return saturate((Color * (A * Color + B)) / (Color * (C * Color + D) + E));
}

float4 PS(PSInput PixelShaderInput, uint SampleIndex : SV_SampleIndex) : SV_Target
{
	int2 Coords = PixelShaderInput.Position.xy - 0.5f;

	float3 HDRColor = HDRFrameBufferTexture.Load(Coords, SampleIndex).rgb;
	float3 BloomColor = HDRBloomTexture.Load(int3(Coords, 0)).rgb;
	float3 ToneMappedColor = ACESToneMappingOperator(HDRColor + BloomColor);

	return float4(ToneMappedColor, 1.0f);
}