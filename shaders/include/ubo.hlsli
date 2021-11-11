[[vk::binding(0,0)]]
cbuffer UBO
{
    float time;
    float4 cameraPos;
    float4x4 proj;
    float4x4 view;
};

[[vk::binding(0,1)]]
cbuffer MeshUBO
{
    float4 baseColor;
    float4x4 meshModel;
    float4x4 meshModelInvDual;
};