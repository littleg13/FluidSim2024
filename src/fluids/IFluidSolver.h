#pragma once

#include "util/3DMath.h"
#include <vector>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d12.h>
#include <wrl.h>

class Renderer;
class ShaderCompiler;
class Allocation;

typedef Microsoft::WRL::ComPtr<ID3D12Device2> ID3D12DevicePtr;

struct alignas(Math::Vec4) ParticleRenderData
{
    Math::Vec4 Position;
    Math::Vec4 Velocity;
};

class IFluidSolver
{
public:
    virtual void CPUSolve(std::vector<ParticleRenderData>& Particles, float DeltaTime) = 0;
    virtual void GPUSolve(Renderer* RenderEngine, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList, Microsoft::WRL::ComPtr<ID3D12Resource> ParticleBuffer) = 0;

    virtual void CreatePipelineStateObject(ID3D12DevicePtr D3D12Device, ShaderCompiler& Compiler) = 0;
    virtual void RecompileShaders(ShaderCompiler& Compiler) = 0;
    virtual void CreateBuffers(Renderer* RenderEngine, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList, Allocation* FluidHeapAllocation) = 0;
};