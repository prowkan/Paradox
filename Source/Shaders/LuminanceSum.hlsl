[[vk::binding(0, 0)]] Texture2D<float> InputLuminanceTexture : register(t0);
[[vk::binding(1, 0)]] RWTexture2D<float> OutputLuminanceTexture : register(u0);

groupshared float LocalLuminances[16][16];

[numthreads(16, 16, 1)]
void CS(uint3 DispatchThreadID : SV_DispatchThreadID, uint3 GroupThreadID : SV_GroupThreadID, uint3 GroupID : SV_GroupID)
{
	float SummedLuminance = 0.0;

	LocalLuminances[GroupThreadID.x][GroupThreadID.y] = InputLuminanceTexture[DispatchThreadID.xy];

	GroupMemoryBarrierWithGroupSync();

	if (GroupThreadID.x == 0 && GroupThreadID.y == 0)
	{
		[unroll]
		for (int i = 0; i < 16; i++)
		{
			[unroll]
			for (int j = 0; j < 16; j++)
			{
				SummedLuminance += LocalLuminances[i][j];
			}
		}

		OutputLuminanceTexture[GroupID.xy] = SummedLuminance;
	}
}