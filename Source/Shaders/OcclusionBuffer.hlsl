struct PSInput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
};

Texture2D<float> DepthBufferTexture : register(t0);

SamplerState MinSampler;

float PS(PSInput PixelShaderInput) : SV_Target
{
	float2 Offset = float2(1.0f / (2.0f * 256.0f * 144.0f), 1.0f / (2.0f * 256.0f * 144.0f));

	float4 MinDepth/*, MinDepths*/;

	/*MinDepths = DepthBufferTexture.Sample(MinSampler, PixelShaderInput.TexCoord + float2(Offset.x, Offset.y));
	MinDepth.x = min(min(MinDepths.x, MinDepths.y), min(MinDepths.z, MinDepths.w));
	MinDepths = DepthBufferTexture.Sample(MinSampler, PixelShaderInput.TexCoord + float2(-Offset.x, Offset.y));
	MinDepth.y = min(min(MinDepths.x, MinDepths.y), min(MinDepths.z, MinDepths.w));
	MinDepths = DepthBufferTexture.Sample(MinSampler, PixelShaderInput.TexCoord + float2(Offset.x, -Offset.y));
	MinDepth.z = min(min(MinDepths.x, MinDepths.y), min(MinDepths.z, MinDepths.w));
	MinDepths = DepthBufferTexture.Sample(MinSampler, PixelShaderInput.TexCoord + float2(-Offset.x, -Offset.y));
	MinDepth.w = min(min(MinDepths.x, MinDepths.y), min(MinDepths.z, MinDepths.w));*/

	MinDepth.x = DepthBufferTexture.Sample(MinSampler, PixelShaderInput.TexCoord + float2(Offset.x, Offset.y));
	MinDepth.y = DepthBufferTexture.Sample(MinSampler, PixelShaderInput.TexCoord + float2(-Offset.x, Offset.y));
	MinDepth.z = DepthBufferTexture.Sample(MinSampler, PixelShaderInput.TexCoord + float2(Offset.x, -Offset.y));
	MinDepth.w = DepthBufferTexture.Sample(MinSampler, PixelShaderInput.TexCoord + float2(-Offset.x, -Offset.y));
	
	return min(min(MinDepth.x, MinDepth.y), min(MinDepth.z, MinDepth.w));
}