#include "vertexinput.hlsli"
#include "ubo.hlsli"
#include "meshubo.hlsli"


float4 main(VertexInput vsi) : SV_POSITION
{
    float4 inpos4 = float4(vsi.inPosition, 1);
    float4 outval = mul(lightDirMatrix, mul(meshModel, inpos4));
    return outval;
}