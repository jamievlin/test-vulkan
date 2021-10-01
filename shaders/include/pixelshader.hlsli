struct PixelShaderInput
{
    float4 scrPos : SV_POSITION;

    [[vk::location(0)]]
    float3 inNormal;

    [[vk::location(1)]]
    float2 outTexCoord;
};