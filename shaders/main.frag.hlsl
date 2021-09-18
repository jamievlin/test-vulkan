struct PixelShaderInput
{
    float4 scrPos : SV_POSITION;

    [[vk::location(0)]]
    float3 inColor;
};

struct PixelShaderOutput
{
    [[vk::location(0)]]
    float4 fragColor : SV_TARGET;
};

[[vk::binding(0,0)]]
cbuffer UBO
{
    float time;
    float4x4 proj;
    float4x4 view;
    float4x4 model;
};


PixelShaderOutput main(PixelShaderInput psi)
{
    PixelShaderOutput pso;
    float multValue = 0.5 + (sin(1.1 * time)) * 0.5;
    pso.fragColor = float4(multValue * psi.inColor,1);

    return pso;
}