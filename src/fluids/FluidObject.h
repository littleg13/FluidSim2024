#pragma once

#include "ObjectRenderer.h"
#include <memory>

#define PARTICLE_COUNT 100000

class Renderer;
template <typename T>
class PrimitiveObject;
class Sphere;
class Allocation;

class MPMSolver;

struct alignas(Math::Vec4) ParticleRenderData
{
    Math::Vec4 Position;
    Math::Vec4 Velocity;
};

class FluidObject : public ObjectRenderer
{
public:
    FluidObject();
    ~FluidObject();
    void CreateBuffers(Renderer* RenderEngineIn);
    void Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList, const Math::Matrix& ViewMatrix);
    virtual void Update(float DeltaTime);
    virtual Microsoft::WRL::ComPtr<ID3D12PipelineState> CreatePipelineStateObject(ID3D12DevicePtr D3D12Device, ShaderCompiler& Compiler) override;
    virtual void RecompileShaders(ShaderCompiler& Compiler) override;

private:
    bool UseCPU = true;
    std::vector<ParticleRenderData> Particles;
    PrimitiveObject<Sphere>* SphereRenderer;
    std::unique_ptr<Allocation> HeapAllocation;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> ComputePipelineState;

    MPMSolver* Solver;

    Renderer* RenderEngine;

    Microsoft::WRL::ComPtr<ID3D12Resource> InstanceBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> InstanceUploadBuffer;
    D3D12_VERTEX_BUFFER_VIEW InstanceBufferView;
};