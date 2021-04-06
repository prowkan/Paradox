#ifdef SPIRV
#define VK_LOCATION(Index) [[vk::location(Index)]]
#define VK_BINDING(Binding, Set) [[vk::binding(Binding, Set)]]
#else
#define VK_LOCATION(Index)
#define VK_BINDING(Binding, Set)
#endif

VK_BINDING(0, 0) Texture2D<float> SummedLuminanceTexture : register(t0);
VK_BINDING(1, 0) RWTexture2D<float> AverageLuminanceTexture : register(u0);

[numthreads(1, 1, 1)]
void CS(uint3 DispatchThreadID : SV_DispatchThreadID)
{
	AverageLuminanceTexture[int2(0, 0)] = SummedLuminanceTexture[int2(0, 0)] / float(1280 * 720);
}