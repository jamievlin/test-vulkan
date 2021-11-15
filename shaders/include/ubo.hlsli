[[vk::binding(0,0)]]
cbuffer UBO
{
    float time;
    float4 cameraPos;
    float4x4 proj;
    float4x4 view;
    float4x4 lightDirMatrix;
};

[[vk::binding(0,1)]]
cbuffer MeshUBO
{
    float4 baseColor;
    float roughness;
    float metallic;
    float f0;
    float _unused;
    float4x4 meshModel;
    float4x4 meshModelInvDual;
};