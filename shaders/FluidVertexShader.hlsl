struct ModelViewProjection
{
    matrix P;
    matrix MV;
};

ConstantBuffer<ModelViewProjection> MVPBuffer : register(b0);
struct VertexInput
{
    float4 Position : POSITION;
    float4 Normal : NORMAL;
    uint InstanceID : SV_InstanceID;
};

struct ParticleData
{
    float4 Position;
    float4 Velocity;
};

RWStructuredBuffer<ParticleData> Particles : register(u0);

struct VertexShaderOutput
{
    float4 DiffColor : COLOR;
    float4 PosVS : POSITION;
    float4 NormalVS : NORMAL;
    float4 Position : SV_Position;
};

VertexShaderOutput main(VertexInput IN)
{
    VertexShaderOutput OUT;
    ParticleData Particle = Particles[IN.InstanceID];
    // Mag / MAX_VELOCITY
    float VelMag = saturate(length(Particle.Velocity.xyz) / 5.0f);
    if (VelMag < 0.5)
    {
        // Interpolate between blue and green
        OUT.DiffColor.xyz = lerp(float3(0, 0, 1), float3(0, 1, 0), VelMag * 2);
    }
    else
    {
        // Interpolate between green and red
        OUT.DiffColor.xyz = lerp(float3(0, 1, 0), float3(1, 0, 0), (VelMag - 0.5) * 2);
    }
    OUT.PosVS = mul(MVPBuffer.MV, IN.Position + float4(Particle.Position.xyz, 0));
    OUT.NormalVS = normalize(mul(MVPBuffer.MV, IN.Normal));
    OUT.Position = mul(MVPBuffer.P, OUT.PosVS);
    return OUT;
}