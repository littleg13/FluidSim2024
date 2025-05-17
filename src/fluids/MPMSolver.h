#pragma once
#include <vector>

#include "fluids/IFluidSolver.h"
#include "util/3DMath.h"

class Allocation;

struct GridCell
{
    // xyz = Velocity, W = mass
    Math::Vec4 VelocityMass;
    uint32_t IntHolder1, IntHolder2, IntHolder3, IntHolder4;
};

struct alignas(Math::Vec4) ParticlePhysicsData
{
    Math::Matrix C;
    Math::Matrix DeformGradient = Math::Identity;
    float Mass = 4.0f;
    float InitialVolume = 1.0f;
    float J = 1.0f;
    float Padding2;
};

class MPMSolver : public IFluidSolver
{
public:
    struct FluidParameters
    {
        int NumParticles;
        uint32_t GridResolution;
        uint32_t NumGridCells;
        float Dx;
        float InvDx;
        // Lame parameters
        float ElasticMu = 40.0f;
        float ElasticLamda = 20.0f;
        float DeltaTime;
        float GridSize;
        float Padding1, Padding2, Padding3;

        FluidParameters(int Num, uint32_t Resolution, float Lamda, float Mu, float Timestep, float Size)
            : NumParticles(Num), ElasticMu(Mu), ElasticLamda(Lamda), GridResolution(Resolution), Dx(Size / float(Resolution)), InvDx(1 / Dx), GridSize(Size), DeltaTime(Timestep), NumGridCells(GridResolution * GridResolution * GridResolution)
        {
        }
    };

    MPMSolver(std::vector<ParticleRenderData>& Particles, FluidParameters& FluidParams);

    virtual void Reset(Renderer* RenderEngine, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList) override;

    virtual void CPUSolve(std::vector<ParticleRenderData>& Particles, float DeltaTime) override;
    virtual void GPUSolve(Renderer* RenderEngine, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList, Microsoft::WRL::ComPtr<ID3D12Resource> ParticleBuffer) override;

    void ParticleToGrid(const std::vector<ParticleRenderData>& Particles, float DeltaTime);
    void GridToParticle(std::vector<ParticleRenderData>& Particles, float DeltaTime);

    virtual void CreatePipelineStateObject(ID3D12DevicePtr D3D12Device, ShaderCompiler& Compiler) override;
    virtual void RecompileShaders(ShaderCompiler& Compiler) override;
    virtual void CreateBuffers(Renderer* RenderEngine, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList, Allocation* FluidHeapAllocation) override;

    Math::Matrix4x4 NeoHookeanStress(const ParticleRenderData& Particle, const ParticlePhysicsData& PhysicsData);

private:
    float Size;
    int NumParticles;
    int GridResolution = 64;
    float DX;
    float InvDx;

    // CPU data
    FluidParameters FluidValues;
    std::vector<GridCell> Grid;
    std::vector<ParticlePhysicsData> ParticleData;

    // GPU data
    Microsoft::WRL::ComPtr<ID3D12Resource> FluidParamBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> ParticleDataBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> GridBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> GridUploadBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> ParticleDataUploadBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> FluidParamUploadBuffer;
    Allocation* HeapAllocation;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineStates[4];
    uint32_t DispatchSizes[4];

    static const float GRID_CLEAR[4];
};