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