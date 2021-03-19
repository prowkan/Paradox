struct PSInput
{
	float4 Position : SV_Position;
	[[vk::location(0)]] float2 TexCoord : TEXCOORD;
};

[[vk::binding(0, 0)]] Texture2D<float> DepthBufferTexture : register(t0);

[[vk::binding(1, 0)]] SamplerState MinSampler;

[[vk::location(0)]] float PS(PSInput PixelShaderInput) : SV_Target
{
	float2 Offset = float2(1.0f / (2.0f * 256.0f * 144.0f), 1.0f / (2.0f * 256.0f * 144.0f));

	float4 MinDepth;

	MinDepth.x = DepthBufferTexture.Sample(MinSampler, PixelShaderInput.TexCoord + float2(Offset.x, Offset.y));
	MinDepth.y = DepthBufferTexture.Sample(MinSampler, PixelShaderInput.TexCoord + float2(-Offset.x, Offset.y));
	MinDepth.z = DepthBufferTexture.Sample(MinSampler, PixelShaderInput.TexCoord + float2(Offset.x, -Offset.y));
	MinDepth.w = DepthBufferTexture.Sample(MinSampler, PixelShaderInput.TexCoord + float2(-Offset.x, -Offset.y));
	
	return min(min(MinDepth.x, MinDepth.y), min(MinDepth.z, MinDepth.w));
}