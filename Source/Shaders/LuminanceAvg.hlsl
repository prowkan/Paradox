[[vk::binding(0, 0)]] Texture2D<float> SummedLuminanceTexture : register(t0);
[[vk::binding(1, 0)]] RWTexture2D<float> AverageLuminanceTexture : register(u0);

[numthreads(1, 1, 1)]
void CS(uint3 DispatchThreadID : SV_DispatchThreadID)
{
	AverageLuminanceTexture[int2(0, 0)] = SummedLuminanceTexture[int2(0, 0)] / float(1280 * 720);
}