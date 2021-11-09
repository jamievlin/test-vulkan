[[vk::binding(0,0)]]
cbuffer UBO
{
    float time;
    float4 cameraPos;
    float4x4 proj;
    float4x4 view;
    float4x4 model;
    float4x4 modelInvDual;
};