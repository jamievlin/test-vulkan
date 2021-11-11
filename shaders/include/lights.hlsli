struct Light
{
    uint lightType;
    float intensity;

    float4 color;
    float4 position;
    float4 parameters;
};

[[vk::binding(2,0)]]
tbuffer lights
{
    uint lightCount;
    Light lightsObj[64];
};