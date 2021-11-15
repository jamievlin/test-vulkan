[[vk::binding(0,0)]]
cbuffer UBO
{
    float time;
    float4 cameraPos;
    float4x4 proj;
    float4x4 view;
    float4x4 lightDirMatrix;
};

[[vk::binding(1,0)]]
Texture2D<float4> tex;

[[vk::binding(1,0)]]
SamplerState sLinear;

