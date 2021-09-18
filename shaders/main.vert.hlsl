struct VertexInput
{
    [[vk::location(0)]]
    float2 inPosition : VTX_INPUT;

    [[vk::location(1)]]
    float3 inColor;
};

struct PixelShaderInput
{
    float4 position : SV_POSITION;

    [[vk::location(0)]]
    float3 inColor;
};

[[vk::binding(0,0)]]
cbuffer UBO
{
    float4x4 MVP;
}


PixelShaderInput main(VertexInput vi)
{
    PixelShaderInput psi;
    psi.position = float4(vi.inPosition,0,1) * MVP;
    psi.inColor = vi.inColor;
    return psi;
}