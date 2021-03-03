struct PSInput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
};

Texture2D<float> DepthBufferTexture : register(t0);

SamplerState MaxSampler;

float PS(PSInput PixelShaderInput) : SV_Target
{
	float2 Offset = float2(1.0f / (2.0f * 256.0f * 144.0f), 1.0f / (2.0f * 256.0f * 144.0f));

	float4 MaxDepth, MaxDepths;

	MaxDepths = DepthBufferTexture.Sample(MaxSampler, PixelShaderInput.TexCoord + float2(Offset.x, Offset.y));
	MaxDepth.x = max(max(MaxDepths.x, MaxDepths.y), max(MaxDepths.z, MaxDepths.w));
	MaxDepths = DepthBufferTexture.Sample(MaxSampler, PixelShaderInput.TexCoord + float2(-Offset.x, Offset.y));
	MaxDepth.y = max(max(MaxDepths.x, MaxDepths.y), max(MaxDepths.z, MaxDepths.w));
	MaxDepths = DepthBufferTexture.Sample(MaxSampler, PixelShaderInput.TexCoord + float2(Offset.x, -Offset.y));
	MaxDepth.z = max(max(MaxDepths.x, MaxDepths.y), max(MaxDepths.z, MaxDepths.w));
	MaxDepths = DepthBufferTexture.Sample(MaxSampler, PixelShaderInput.TexCoord + float2(-Offset.x, -Offset.y));
	MaxDepth.w = max(max(MaxDepths.x, MaxDepths.y), max(MaxDepths.z, MaxDepths.w));

	return max(max(MaxDepth.x, MaxDepth.y), max(MaxDepth.z, MaxDepth.w));
}