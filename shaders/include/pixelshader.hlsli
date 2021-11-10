struct PixelShaderInput
{
    float4 scrPos : SV_POSITION;

    [[vk::location(1)]]
    float3 worldPos;

    [[vk::location(2)]]
    float3 inNormal;

    [[vk::location(3)]]
    float2 outTexCoord;
};