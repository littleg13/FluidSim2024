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
};

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

    OUT.DiffColor = float4(0.7f, 0.7f, 0.7f, 1.0f);

    OUT.PosVS = mul(MVPBuffer.MV, IN.Position);
    OUT.NormalVS = normalize(mul(MVPBuffer.MV, IN.Normal));

    OUT.Position = mul(MVPBuffer.P, OUT.PosVS);
    return OUT;
}