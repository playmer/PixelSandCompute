StructuredBuffer<uint> BlockBuffer : register(t0, space0);

[[vk::image_format("rgba8")]]
RWTexture2D<float4> outImage : register(u0, space1);

[numthreads(8, 8, 1)]
void main(uint3 GlobalInvocationID : SV_DispatchThreadID)
{
    float4 block_colors[] = {
        float4(0.537f, 0.537f, 0.537f, 1.0f), // Stone
        float4(0.537f, 0.318f, 0.161f, 1.0f), // Dirt
        float4(0.564f, 0.835f, 1.0f, 1.0f),   // Air
    };

    int2 coord = int2(GlobalInvocationID.xy);
    //outImage[coord] = ((coord.y % 2) == 0) ? 
    //    ((coord.x % 2) == 0) ? float4(0.0f, 0.0f, 0.0f, 1.0f) : float4(1.0f, 1.0f, 1.0f, 1.0f) :
    //    ((coord.x % 2) == 0) ? float4(1.0f, 1.0f, 1.0f, 1.0f) : float4(0.0f, 0.0f, 0.0f, 1.0f);

    uint index = (coord.y * 1920) + coord.x;

    outImage[coord] = block_colors[BlockBuffer[index]];
}
