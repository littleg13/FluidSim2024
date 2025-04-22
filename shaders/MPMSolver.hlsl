#define BLOCK_SIZE 8
#define GROUP_SIZE 64 // Predefine BLOCK_SIZE^2

#define IDENTITY_MATRIX float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)

#define EOS_STIFFNESS 1000.0f
#define EOS_POWER 7
#define DYNAMIC_VISCOSITY 0.0f

struct ParticleRenderData
{
    float4 Position;
    float4 Velocity;
};

struct ParticlePhysicsData
{
    matrix C;
    matrix DeformGradient;
    float Mass;
    float InitialVolume;
    float Padding1, Padding2;
};

struct FluidParameters
{
    uint NumParticles;
    uint GridResolution;
    float Dx;
    float InvDx;
    float ElasticMu;
    float ElasticLamda;
    float DeltaTime;
    float GridSize;
};

struct GridCell
{
    float4 VelocityMass;
    int4 IntVelocityMass;
};

RWStructuredBuffer<ParticleRenderData> Particles : register(u0);
RWStructuredBuffer<ParticlePhysicsData> ParticleData : register(u1);
// xyz = Velocity, w = mass
RWStructuredBuffer<GridCell> Grid : register(u2);
ConstantBuffer<FluidParameters> Fluid : register(b3);

uint GetThreadIndex(uint ThreadIndex, uint3 GroupId)
{
    return GroupId.x * GROUP_SIZE + ThreadIndex;
}

float4x4 inverse(float4x4 m)
{
    float n11 = m[0][0], n12 = m[1][0], n13 = m[2][0], n14 = m[3][0];
    float n21 = m[0][1], n22 = m[1][1], n23 = m[2][1], n24 = m[3][1];
    float n31 = m[0][2], n32 = m[1][2], n33 = m[2][2], n34 = m[3][2];
    float n41 = m[0][3], n42 = m[1][3], n43 = m[2][3], n44 = m[3][3];

    float t11 = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 - n23 * n32 * n44 + n22 * n33 * n44;
    float t12 = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 + n13 * n32 * n44 - n12 * n33 * n44;
    float t13 = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 - n13 * n22 * n44 + n12 * n23 * n44;
    float t14 = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34;

    float det = n11 * t11 + n21 * t12 + n31 * t13 + n41 * t14;
    float idet = 1.0f / det;

    float4x4 ret;

    ret[0][0] = t11 * idet;
    ret[0][1] = (n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 + n23 * n31 * n44 - n21 * n33 * n44) * idet;
    ret[0][2] = (n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 - n22 * n31 * n44 + n21 * n32 * n44) * idet;
    ret[0][3] = (n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 + n22 * n31 * n43 - n21 * n32 * n43) * idet;

    ret[1][0] = t12 * idet;
    ret[1][1] = (n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 - n13 * n31 * n44 + n11 * n33 * n44) * idet;
    ret[1][2] = (n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 + n12 * n31 * n44 - n11 * n32 * n44) * idet;
    ret[1][3] = (n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 - n12 * n31 * n43 + n11 * n32 * n43) * idet;

    ret[2][0] = t13 * idet;
    ret[2][1] = (n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 + n13 * n21 * n44 - n11 * n23 * n44) * idet;
    ret[2][2] = (n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 - n12 * n21 * n44 + n11 * n22 * n44) * idet;
    ret[2][3] = (n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 + n12 * n21 * n43 - n11 * n22 * n43) * idet;

    ret[3][0] = t14 * idet;
    ret[3][1] = (n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 - n13 * n21 * n34 + n11 * n23 * n34) * idet;
    ret[3][2] = (n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 + n12 * n21 * n34 - n11 * n22 * n34) * idet;
    ret[3][3] = (n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 - n12 * n21 * n33 + n11 * n22 * n33) * idet;

    return ret;
}

matrix outerProduct(float4 B, float4 A)
{
    return float4x4(
        A.x * B.x, A.x * B.y, A.x * B.z, A.x * B.w,
        A.y * B.x, A.y * B.y, A.y * B.z, A.y * B.w,
        A.z * B.x, A.z * B.y, A.z * B.z, A.z * B.w,
        A.w * B.x, A.w * B.y, A.w * B.z, A.w * B.w);
}

matrix NeoHookeanStress(ParticleRenderData Particle, ParticlePhysicsData PhysicsData)
{
    float Volume = determinant(PhysicsData.DeformGradient);

    matrix DeformTranspose = transpose(PhysicsData.DeformGradient);
    matrix DeformTransposeInverse = inverse(DeformTranspose);

    matrix P = ((PhysicsData.DeformGradient - DeformTransposeInverse) * Fluid.ElasticMu) + (DeformTransposeInverse * (Fluid.ElasticLamda * log(Volume)));

    return (P * DeformTranspose) * -(PhysicsData.InitialVolume * 4 * Fluid.InvDx * Fluid.InvDx);
}

matrix ConstitutiveStress(ParticleRenderData Particle, ParticlePhysicsData PhysicsData)
{
    float Volume = determinant(PhysicsData.DeformGradient);
    float Density = PhysicsData.Mass / Volume;
    float Pressure = max(0.0f, EOS_STIFFNESS * (pow(PhysicsData.InitialVolume / Volume, EOS_POWER) - 1.0f));
    matrix Stress = float4x4(
        -Pressure, 0.0f, 0.0f, 0.0f,
        0.0f, -Pressure, 0.0f, 0.0f,
        0.0f, 0.0f, -Pressure, 0.0f,
        0.0f, 0.0f, 0.0f, -Pressure);
    matrix Strain = PhysicsData.C + transpose(PhysicsData.C);
    Stress += DYNAMIC_VISCOSITY * Strain;
    return -PhysicsData.InitialVolume * Stress * 4 * Fluid.InvDx * Fluid.InvDx;
}

// clang-format off
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)] 
void GridToParticle(uint ThreadIndex : SV_GroupIndex, uint3 GroupId : SV_GroupID)
{
    uint Index = GetThreadIndex(ThreadIndex, GroupId);
    if (Index < Fluid.NumParticles)
    {
        Particles[Index].Velocity = float4(0.0f, 0.0f, 0.0f, 0.0f);
        ParticleRenderData Particle = Particles[Index];
        int2 CellIndex = int2((Particle.Position.xy * Fluid.InvDx) - 0.5f);
        float2 CellDifference = (Particle.Position.xy * Fluid.InvDx) - CellIndex;

        // Precalculate quadratic weight coefficients
        float2 Weights[3];
        Weights[0] = pow(1.5f - CellDifference, 2) * 0.5f;
        Weights[1] = 0.75f - pow(CellDifference - 1.0f, 2);
        Weights[2] = pow(CellDifference - 0.5f, 2) * 0.5f;

        matrix B = float4x4(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        for (int x = 0; x < 3; x++)
        {
            for (int y = 0; y < 3; y++)
            {
                float Weight = Weights[x].x * Weights[y].y;

                float4 CellDistance = float4(x - CellDifference.x, y - CellDifference.y, 0.0f, 0.0f) * Fluid.Dx;

                int GridIndex = (CellIndex.x + x) * Fluid.GridResolution + CellIndex.y + y;
                float4 WeightedVelocity = Grid[GridIndex].VelocityMass * Weight;

                B += outerProduct(WeightedVelocity, CellDistance);

                Particles[Index].Velocity.xyz += WeightedVelocity.xyz;
            }
        }
        ParticleData[Index].C = B * 4 * Fluid.InvDx;

        Particles[Index].Position += Particles[Index].Velocity * Fluid.DeltaTime;

        Particles[Index].Position.x = min(max(Particles[Index].Position.x, Fluid.Dx), Fluid.GridSize - 2 * Fluid.Dx);
        Particles[Index].Position.y = min(max(Particles[Index].Position.y, Fluid.Dx), Fluid.GridSize - 2 * Fluid.Dx);

        ParticleData[Index].DeformGradient = (IDENTITY_MATRIX + (ParticleData[Index].C * Fluid.DeltaTime)) * ParticleData[Index].DeformGradient;
    }
}

[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)] 
void GridUpdate(uint ThreadIndex : SV_GroupIndex, uint3 GroupId : SV_GroupID)
{
    uint Index = GetThreadIndex(ThreadIndex, GroupId);
    if (Index < Fluid.GridResolution * Fluid.GridResolution)
    {
        Grid[Index].VelocityMass = Grid[Index].IntVelocityMass / 100000.0f;
        float4 Gravity = float4(0.0f, -9.8f * Fluid.DeltaTime, 0.0f, 0.0f);
        if (Grid[Index].VelocityMass.w > 0.00000001)
        {
            Grid[Index].VelocityMass.xyz /= Grid[Index].VelocityMass.w;

            // Apply Gravity
            Grid[Index].VelocityMass += Gravity;

            // Apply Boundary Conditions
            int X = Index / Fluid.GridResolution;
            int Y = Index % Fluid.GridResolution;

            if (X < 2 || X > Fluid.GridResolution - 2)
            {
                Grid[Index].VelocityMass.x *= 0.001f;
            }

            if (Y < 2 || Y > Fluid.GridResolution - 2)
            {
                Grid[Index].VelocityMass.y *= 0.001f;
            }
        }
    }
}

[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)] 
void ParticleToGrid(uint ThreadIndex : SV_GroupIndex, uint3 GroupId : SV_GroupID)
{
    uint Index = GetThreadIndex(ThreadIndex, GroupId);
    if (Index < Fluid.NumParticles)
    {
        ParticleRenderData Particle = Particles[Index];
        matrix Affine = ConstitutiveStress(Particle, ParticleData[Index]) * Fluid.DeltaTime + (ParticleData[Index].C * ParticleData[Index].Mass);
        //matrix Affine = (ParticleData[Index].C * ParticleData[Index].Mass);
        int2 CellIndex = int2((Particle.Position.xy * Fluid.InvDx) - 0.5f);
        float2 CellDifference = (Particle.Position.xy * Fluid.InvDx) - CellIndex;

        // Precalculate quadratic weight coefficients
        float2 Weights[3];
        Weights[0] = pow(1.5f - CellDifference, 2) * 0.5f;
        Weights[1] = 0.75f - pow(CellDifference - 1.0f, 2);
        Weights[2] = pow(CellDifference - 0.5f, 2) * 0.5f;

        for (int x = 0; x < 3; x++)
        {
            for (int y = 0; y < 3; y++)
            {
                float Weight = Weights[x].x * Weights[y].y;

                float4 CellDistance = float4(x - CellDifference.x, y - CellDifference.y, 0.0f, 0.0f) * Fluid.Dx;

                float4 AffineByDistance = mul(Affine, CellDistance);

                float4 Momentum = Particle.Velocity * ParticleData[Index].Mass;
                Momentum.w = 0.0f;

                int GridIndex = (CellIndex.x + x) * Fluid.GridResolution + CellIndex.y + y;

                // Grid[GridIndex].w += ParticleData[Index].Mass * Weight;
                // Grid[GridIndex].xyz += (Momentum).xyz * Weight;
                int4 IntVelocityAddition = int4(float4((Momentum + AffineByDistance).xyz, ParticleData[Index].Mass) * Weight * 100000);
                InterlockedAdd(Grid[GridIndex].IntVelocityMass.x, IntVelocityAddition.x);
                InterlockedAdd(Grid[GridIndex].IntVelocityMass.y, IntVelocityAddition.y);
                InterlockedAdd(Grid[GridIndex].IntVelocityMass.z, IntVelocityAddition.z);
                InterlockedAdd(Grid[GridIndex].IntVelocityMass.w, IntVelocityAddition.w);
            }
        }
    }
}

[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)] 
void ClearGrid(uint ThreadIndex : SV_GroupIndex, uint3 GroupId : SV_GroupID)
{
    uint Index = GetThreadIndex(ThreadIndex, GroupId);
    if (Index < Fluid.GridResolution * Fluid.GridResolution)
    {
        Grid[Index].VelocityMass = float4(0.0f, 0.0f, 0.0f, 0.0f);
        Grid[Index].IntVelocityMass = int4(0, 0., 0, 0);
    }
}
// clang-format on