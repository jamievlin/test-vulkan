struct VertexInput
{
    [[vk::location(0)]]
    float3 inPosition : VTX_INPUT;

    [[vk::location(1)]]
    float3 inColor;

    [[vk::location(2)]]
    float2 texCoord;
};