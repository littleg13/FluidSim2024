struct FragmentInput
{
    float4 DiffColor : COLOR;
    float4 PosVS : POSITION;
    float4 NormalVS : NORMAL;
};

struct Light
{
    float4 Pos;
    float3 Color;
};

static const Light LIGHT = {
    float4(0, 0, -10, 1),
    float3(1, 1, 1)};

static const float3 Ambient = {0.1, 0.1, 0.1};

float3 Diffuse(FragmentInput IN)
{
    float4 ToLight = LIGHT.Pos - IN.PosVS;
    float4 LightDir = normalize(ToLight);
    float LightMag = length(ToLight);
    float Attenuation = 1; // / (0.5 + 0.01 * LightMag + 0.001 * LightMag * LightMag);
    return LIGHT.Pos.w * IN.DiffColor.xyz * LIGHT.Color * max(dot(LightDir, IN.NormalVS), 0);
}

float4 main(FragmentInput IN) : SV_Target
{
    return float4(Diffuse(IN) + Ambient, 1.0f);
}