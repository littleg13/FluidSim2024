#pragma once
#include <vector>

#include "fluids/FluidObject.h"
#include "util/3DMath.h"

class Allocation;

struct GridCell
{
    // xyz = Velocity, W = mass
    Math::Vec4 VelocityMass;
    uint32_t IntHolder1, IntHolder2, IntHolder3, IntHolder4;
};

struct FluidParameters
{
    uint32_t GridResolution;
    float Dx;
    float InvDx;
    // Lame parameters
    float ElasticMu = 40.0f;
    float ElasticLamda = 20.0f;
    float DeltaTime;
    float GridSize;
    float _;

    FluidParameters(uint32_t Resolution, float Lamda, float Mu, float Timestep, float Size)
        : ElasticMu(Mu), ElasticLamda(Lamda), GridResolution(Resolution), Dx(Size / float(Resolution)), InvDx(1 / Dx), GridSize(Size), DeltaTime(Timestep)
    {
    }
};

struct alignas(Math::Vec4) ParticlePhysicsData
{
    Math::Matrix C;
    Math::Matrix DeformGradient = Math::Identity;
    float Mass = 0.5f;
    float InitialVolume = 3.0f;
    float Padding1, Padding2;
};

class MPMSolver
{
public:
    MPMSolver(std::vector<ParticleRenderData>& Particles, FluidParameters& FluidParams);

    void CPUSolve(std::vector<ParticleRenderData>& Particles, float DeltaTime);
    void GPUSolve(Renderer* RenderEngine, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList, Microsoft::WRL::ComPtr<ID3D12Resource> ParticleBuffer);

    void ParticleToGrid(const std::vector<ParticleRenderData>& Particles, float DeltaTime);
    void GridToParticle(std::vector<ParticleRenderData>& Particles, float DeltaTime);

    void CreatePipelineStateObject(ID3D12DevicePtr D3D12Device, ShaderCompiler& Compiler);
    void RecompileShaders(ShaderCompiler& Compiler);
    void CreateBuffers(Renderer* RenderEngine, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList, Allocation* FluidHeapAllocation);

    Math::Matrix4x4 NeoHookeanStress(const ParticleRenderData& Particle, const ParticlePhysicsData& PhysicsData);

private:
    float Size;
    int NumParticles;
    int GridResolution = 64;
    float DX;
    float InvDx;

    FluidParameters FluidValues;
    std::vector<GridCell> Grid;
    std::vector<ParticlePhysicsData> ParticleData;

    Microsoft::WRL::ComPtr<ID3D12Resource> FluidParamBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> ParticleDataBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> GridBuffer;
    D3D12_GPU_DESCRIPTOR_HANDLE GridBufferGPUHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE GridBufferCPUHandle;
    Microsoft::WRL::ComPtr<ID3D12Resource> GridUploadBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> ParticleDataUploadBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> FluidParamUploadBuffer;
    Allocation* HeapAllocation;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineStates[4];
    uint32_t DispatchSizes[4];

    static const float GRID_CLEAR[4];
};