#include "MPMSolver.h"

#include "DescriptorHeapAllocator.h"
#include "PSOBuilder.h"
#include "Renderer.h"
#include "ShaderCompiler.h"

#include <algorithm>
#include <cmath>

ShaderDesc MPMG2PComputeShader = {
    L"D:\\Dev\\Projects\\FluidSim2024\\shaders\\MPMSolver.hlsl",
    L"cs_6_0",
    L"GridToParticle"};

ShaderDesc MPMP2GComputeShader = {
    L"D:\\Dev\\Projects\\FluidSim2024\\shaders\\MPMSolver.hlsl",
    L"cs_6_0",
    L"ParticleToGrid"};

ShaderDesc MPMGridUpdateComputeShader = {
    L"D:\\Dev\\Projects\\FluidSim2024\\shaders\\MPMSolver.hlsl",
    L"cs_6_0",
    L"GridUpdate"};

ShaderDesc MPMClearGridComputeShader = {
    L"D:\\Dev\\Projects\\FluidSim2024\\shaders\\MPMSolver.hlsl",
    L"cs_6_0",
    L"ClearGrid"};

#define GROUP_SIZE 64.0f

MPMSolver::MPMSolver(std::vector<ParticleRenderData>& Particles, FluidParameters& FluidParams)
    : GridResolution(FluidParams.GridResolution), NumParticles(Particles.size()), Size(FluidParams.GridSize), FluidValues(FluidParams)
{
    Grid = std::vector<GridCell>(GridResolution * GridResolution);
    ParticleData = std::vector<ParticlePhysicsData>(NumParticles);

    DX = FluidParams.Dx;
    InvDx = 1 / DX;
}

void MPMSolver::CreatePipelineStateObject(ID3D12DevicePtr D3D12Device, ShaderCompiler& Compiler)
{
    PSOBuilder ComputeBuilder;
    ComputeBuilder.SetComputeShader(Compiler.GetShader(MPMClearGridComputeShader));
    PipelineStates[0] = ComputeBuilder.BuildCompute(D3D12Device);
    DispatchSizes[0] = static_cast<int>(ceil(Grid.size() / GROUP_SIZE));

    ComputeBuilder.SetComputeShader(Compiler.GetShader(MPMP2GComputeShader));
    PipelineStates[1] = ComputeBuilder.BuildCompute(D3D12Device);
    DispatchSizes[1] = static_cast<int>(ceil(NumParticles / GROUP_SIZE));

    ComputeBuilder.SetComputeShader(Compiler.GetShader(MPMGridUpdateComputeShader));
    PipelineStates[2] = ComputeBuilder.BuildCompute(D3D12Device);
    DispatchSizes[2] = static_cast<int>(ceil(Grid.size() / GROUP_SIZE));

    ComputeBuilder.SetComputeShader(Compiler.GetShader(MPMG2PComputeShader));
    PipelineStates[3] = ComputeBuilder.BuildCompute(D3D12Device);
    DispatchSizes[3] = static_cast<int>(ceil(NumParticles / GROUP_SIZE));
}

void MPMSolver::RecompileShaders(ShaderCompiler& Compiler)
{
    Compiler.CompileShaderNonAsync(MPMClearGridComputeShader, true);
    Compiler.CompileShaderNonAsync(MPMG2PComputeShader, true);
    Compiler.CompileShaderNonAsync(MPMP2GComputeShader, true);
    Compiler.CompileShaderNonAsync(MPMGridUpdateComputeShader, true);
}

void MPMSolver::CreateBuffers(Renderer* RenderEngine, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList, Allocation* FluidHeapAllocation)
{
    HeapAllocation = FluidHeapAllocation;

    RenderEngine->UploadDefaultBufferResource(CommandList, ParticleDataBuffer, ParticleDataUploadBuffer, ParticleData.size(), sizeof(decltype(ParticleData.back())), ParticleData.data(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    RenderEngine->TransitionBarrier(CommandList, ParticleDataBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    RenderEngine->UploadDefaultBufferResource(CommandList, GridBuffer, GridUploadBuffer, Grid.size(), sizeof(decltype(Grid.back())), Grid.data(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    RenderEngine->TransitionBarrier(CommandList, GridBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    RenderEngine->UploadDefaultBufferResource(CommandList, FluidParamBuffer, FluidParamUploadBuffer, 1, sizeof(FluidParameters), &FluidValues, D3D12_RESOURCE_FLAG_NONE, 256);
    RenderEngine->TransitionBarrier(CommandList, FluidParamBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    HeapAllocation->CreateBufferUAV(ParticleDataBuffer, ParticleData.size(), sizeof(decltype(ParticleData.back())));
    HeapAllocation->CreateBufferUAV(GridBuffer, Grid.size(), sizeof(decltype(Grid.back())));
    HeapAllocation->CreateBufferCBV(FluidParamBuffer, 256);
}

void MPMSolver::GPUSolve(Renderer* RenderEngine, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList, Microsoft::WRL::ComPtr<ID3D12Resource> ParticleBuffer)
{
    for (int i = 0; i < 4; i++)
    {
        CommandList->SetPipelineState(PipelineStates[i].Get());
        CommandList->Dispatch(DispatchSizes[i], 1, 1);
        RenderEngine->UAVBarrier(CommandList, GridBuffer.Get());
        RenderEngine->UAVBarrier(CommandList, ParticleDataBuffer.Get());
        RenderEngine->UAVBarrier(CommandList, ParticleBuffer.Get());
    }
}

void MPMSolver::CPUSolve(std::vector<ParticleRenderData>& Particles, float DeltaTime)
{
    // Reset grid
    for (GridCell& Cell : Grid)
    {
        Cell.VelocityMass = Math::Vec4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    ParticleToGrid(Particles, DeltaTime);

    // Grid Velocity update
    Math::Vec4 Gravity = Math::Vec4(0.0f, -9.8f * DeltaTime, 0.0f, 0.0f);
    int i = 0;
    for (GridCell& Cell : Grid)
    {
        if (Cell.VelocityMass.w > 0.00001)
        {
            Cell.VelocityMass.x /= Cell.VelocityMass.w;
            Cell.VelocityMass.y /= Cell.VelocityMass.w;
            Cell.VelocityMass.z /= Cell.VelocityMass.w;

            // Apply Gravity
            Cell.VelocityMass += Gravity;

            // Apply Boundary Conditions
            int X = i / GridResolution;
            int Y = i % GridResolution;

            if (X < 2 || X > GridResolution - 3)
            {
                Cell.VelocityMass.x = 0.0f;
            }

            if (Y < 2 || Y > GridResolution - 3)
            {
                Cell.VelocityMass.y = 0.0f;
            }
        }
        i++;
    }

    GridToParticle(Particles, DeltaTime);
}

void MPMSolver::ParticleToGrid(const std::vector<ParticleRenderData>& Particles, float DeltaTime)
{
    // Particle to Grid
    Math::Vec2<float> Weights[3];
    int ParticleIndex = 0;
    for (const ParticleRenderData& Particle : Particles)
    {

        Math::Matrix4x4 Affine = NeoHookeanStress(Particle, ParticleData[ParticleIndex]) * DeltaTime + (ParticleData[ParticleIndex].C * ParticleData[ParticleIndex].Mass);
        Math::Vec2<int32_t> CellIndex = Math::Vec2<int32_t>(Particle.Position.x * InvDx - 0.5f, Particle.Position.y * InvDx - 0.5f);
        Math::Vec2 CellDifference = Math::Vec2(Particle.Position.x * InvDx - CellIndex.x, Particle.Position.y * InvDx - CellIndex.y);

        // Precalculate quadratic weight coefficients
        Weights[0] = Math::Vec2(1.5f - CellDifference.x, 1.5f - CellDifference.y).Pow(2) * 0.5f;
        Weights[1] = Math::Vec2(0.75f, 0.75f) - Math::Vec2(CellDifference.x - 1.0f, CellDifference.y - 1.0f).Pow(2);
        Weights[2] = Math::Vec2(CellDifference.x - 0.5f, CellDifference.y - 0.5f).Pow(2) * 0.5f;

        for (int x = 0; x < 3; x++)
        {
            for (int y = 0; y < 3; y++)
            {
                float Weight = Weights[x].x * Weights[y].y;

                Math::Vec4 CellDistance = Math::Vec4(x - CellDifference.x, y - CellDifference.y, 0.0f, 0.0f) * DX;

                Math::Vec4 AffineByDistance = Affine * CellDistance;

                Math::Vec4 Momentum = Particle.Velocity * ParticleData[ParticleIndex].Mass;
                Momentum.w = 0.0f;

                int Index = (CellIndex.x + x) * GridResolution + CellIndex.y + y;

                GridCell& Cell = Grid[Index];
                Cell.VelocityMass.w += ParticleData[ParticleIndex].Mass * Weight;
                Cell.VelocityMass += (Momentum + AffineByDistance) * Weight;
            }
        }
        ParticleIndex++;
    }
}

void MPMSolver::GridToParticle(std::vector<ParticleRenderData>& Particles, float DeltaTime)
{
    // Grid to Particle
    Math::Vec2<float> Weights[3];
    int ParticleIndex = 0;
    for (ParticleRenderData& Particle : Particles)
    {
        Particle.Velocity = Math::Vec4();

        Math::Vec2<int32_t> CellIndex = Math::Vec2<int32_t>(Particle.Position.x * InvDx - 0.5f, Particle.Position.y * InvDx - 0.5f);
        Math::Vec2 CellDifference = Math::Vec2(Particle.Position.x * InvDx - CellIndex.x, Particle.Position.y * InvDx - CellIndex.y);

        // Precalculate quadratic weight coefficients
        Weights[0] = Math::Vec2(1.5f - CellDifference.x, 1.5f - CellDifference.y).Pow(2) * 0.5f;
        Weights[1] = Math::Vec2(0.75f, 0.75f) - Math::Vec2(CellDifference.x - 1.0f, CellDifference.y - 1.0f).Pow(2);
        Weights[2] = Math::Vec2(CellDifference.x - 0.5f, CellDifference.y - 0.5f).Pow(2) * 0.5f;
        Math::Matrix4x4 B;
        for (int x = 0; x < 3; x++)
        {
            for (int y = 0; y < 3; y++)
            {
                float Weight = Weights[x].x * Weights[y].y;

                Math::Vec2<int32_t> CurrentCell = Math::Vec2<int32_t>(x, y);
                Math::Vec4 CellDistance = Math::Vec4(x - CellDifference.x, y - CellDifference.y, 0.0f, 0.0f) * DX;

                int Index = (CellIndex.x + x) * GridResolution + CellIndex.y + y;
                GridCell& Cell = Grid[Index];
                Math::Vec4 WeightedVelocity(Cell.VelocityMass.x * Weight, Cell.VelocityMass.y * Weight, Cell.VelocityMass.z * Weight, 0.0f);

                B += WeightedVelocity.OuterProduct(CellDistance) * InvDx;

                Particle.Velocity += WeightedVelocity;
            }
        }
        ParticleData[ParticleIndex].C = B * 4;

        Particle.Position += Particle.Velocity * DeltaTime;

        Particle.Position.x = std::min(std::max(Particle.Position.x, DX), Size - (DX));
        Particle.Position.y = std::min(std::max(Particle.Position.y, DX), Size - (DX));

        ParticleData[ParticleIndex].DeformGradient = (Math::Identity + (ParticleData[ParticleIndex].C * DeltaTime)) * ParticleData[ParticleIndex].DeformGradient;
        ParticleIndex++;
    }
}

Math::Matrix4x4 MPMSolver::NeoHookeanStress(const ParticleRenderData& Particle, const ParticlePhysicsData& PhysicsData)
{
    float Volume = PhysicsData.DeformGradient.Determinant();

    Math::Matrix4x4 DeformTranspose = PhysicsData.DeformGradient.Transpose();
    Math::Matrix4x4 DeformTransposeInverse = DeformTranspose.Inverse();

    Math::Matrix4x4 P = ((PhysicsData.DeformGradient - DeformTransposeInverse) * FluidValues.ElasticMu) + (DeformTransposeInverse * (FluidValues.ElasticLamda * std::log(Volume)));

    return (P * DeformTranspose) * -(PhysicsData.InitialVolume * 4 * InvDx * InvDx);
}