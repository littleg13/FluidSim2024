#pragma once

#include "ObjectRenderer.h"
#include <memory>
#include <unordered_map>

class Renderer;
template <typename T>
class PrimitiveObject;
class Sphere;
class Allocation;

class IFluidSolver;
struct ParticleRenderData;

enum FluidSolver
{
    MPMCPUSolver,
    MPMGPUSolver
};

class FluidObject : public ObjectRenderer
{
public:
    FluidObject(int NumParticles, float BoundingBoxSize, FluidSolver SolverType);
    ~FluidObject();
    void CreateBuffers(Renderer* RenderEngineIn);
    void Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList, const Math::Matrix& ViewMatrix);
    virtual void Update(float DeltaTime);
    virtual Microsoft::WRL::ComPtr<ID3D12PipelineState> CreatePipelineStateObject(ID3D12DevicePtr D3D12Device, ShaderCompiler& Compiler) override;
    virtual void RecompileShaders(ShaderCompiler& Compiler) override;

private:
    bool UseCPU = false;
    std::vector<ParticleRenderData> Particles;
    PrimitiveObject<Sphere>* SphereRenderer;
    std::unique_ptr<Allocation> HeapAllocation;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> ComputePipelineState;

    IFluidSolver* Solver;

    Renderer* RenderEngine;

    Microsoft::WRL::ComPtr<ID3D12Resource> InstanceBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> InstanceUploadBuffer;
    D3D12_VERTEX_BUFFER_VIEW InstanceBufferView;
};