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

PixelShaderOutput main(PixelShaderInput psi)
{
    PixelShaderOutput pso;
    pso.fragColor = float4(psi.inColor,1);

    return pso;
}