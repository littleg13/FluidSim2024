#include "fluids/FluidObject.h"

#include "DescriptorHeapAllocator.h"
#include "PSOBuilder.h"
#include "Renderer.h"
#include "ShaderCompiler.h"
#include "primitives/PrimitiveObject.h"
#include "primitives/Sphere.h"

#include "fluids/MPMSolver.h"

#include <cmath>
#include <cstdlib>

ShaderDesc FluidVertexShader = {
    L"D:\\Dev\\Projects\\FluidSim2024\\shaders\\FluidVertexShader.hlsl",
    L"vs_6_0",
    L"main"};
ShaderDesc FluidFragmentShader = {
    L"D:\\Dev\\Projects\\FluidSim2024\\shaders\\FluidFragmentShader.hlsl",
    L"ps_6_0",
    L"main"};

FluidObject::FluidObject(int NumParticles, float BoundingBoxSize, FluidSolver SolverType)
    : NumParticles(NumParticles), BoundingBoxSize(BoundingBoxSize)
{
    ResetParticles();
    switch (SolverType)
    {
    case MPMCPUSolver:
        UseCPU = true;
    case MPMGPUSolver:
        // FluidParameters:              NumParticles, Resolution, Lambda, Mu, Timestep, Size
        MPMSolver::FluidParameters Params = {NumParticles, 128, 40.0f, 20.0f, 0.0020f, BoundingBoxSize};
        Solver = new MPMSolver(Particles, Params);
        break;
    }
}

FluidObject::~FluidObject()
{
    delete Solver;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> FluidObject::CreatePipelineStateObject(ID3D12DevicePtr D3D12Device, ShaderCompiler& Compiler)
{
    // Have physics solver create any compute pipeline states it needs
    Solver->CreatePipelineStateObject(D3D12Device, Compiler);

    PSOBuilder Builder;
    Builder.SetShaders(Compiler.GetShader(FluidVertexShader), Compiler.GetShader(FluidFragmentShader))
        .SetPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE)
        .AddInputLayoutParameter({"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0})
        .AddInputLayoutParameter({"NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0})
        .AddRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
    return Builder.BuildGraphics(D3D12Device);
}

void FluidObject::RecompileShaders(ShaderCompiler& Compiler)
{
    Compiler.CompileShaderNonAsync(FluidVertexShader, true);
    Compiler.CompileShaderNonAsync(FluidFragmentShader, true);

    if (!UseCPU)
    {
        Solver->RecompileShaders(Compiler);
    }
}

void FluidObject::ResetParticles()
{
    Particles.clear();
    int Rows = std::sqrt(NumParticles);
    float Delta = (BoundingBoxSize - BoundingBoxSize * 0.2f) / float(Rows);
    for (int i = 0; i < NumParticles; i++)
    {
        float RandomOne = Delta * ((static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) - 0.5f);
        float RandomTwo = Delta * ((static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) - 0.5f);
        ParticleRenderData Vert = {Math::Vec4((i % Rows) * Delta + BoundingBoxSize * 0.1f + RandomOne, (i / Rows) * Delta + BoundingBoxSize * 0.1f + RandomTwo, 0), Math::Vec4()};
        Particles.emplace_back(std::move(Vert));
    }
}

void FluidObject::Reset()
{
    ResetParticles();

    // Reupload the buffer
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList = RenderEngine->GetCommandList();
    RenderEngine->TransitionBarrier(CommandList, InstanceBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);
    RenderEngine->UploadDefaultBufferResource(CommandList, InstanceBuffer, InstanceUploadBuffer, Particles.size(), sizeof(decltype(Particles.back())), Particles.data(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    RenderEngine->TransitionBarrier(CommandList, InstanceBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    Solver->Reset(RenderEngine, CommandList);

    uint32_t FenceValue = RenderEngine->ExecuteCommandList(CommandList);
    RenderEngine->WaitForFenceValue(FenceValue);
}

void FluidObject::HandleKeyPress(uint64_t wParam, bool isRepeat)
{
    if (!isRepeat)
    {
        switch (wParam)
        {
        case 0x52: // R
            Reset();
            break;
        }
    }
}

void FluidObject::CreateBuffers(Renderer* RenderEngineIn)
{
    RenderEngine = RenderEngineIn;
    Sphere::GenerateSphereData();
    SphereRenderer = new PrimitiveObject<Sphere>();
    SphereRenderer->CreateBuffers(RenderEngine);

    HeapAllocation = RenderEngine->GetAllocation();
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList = RenderEngine->GetCommandList();
    RenderEngine->UploadDefaultBufferResource(CommandList, InstanceBuffer, InstanceUploadBuffer, Particles.size(), sizeof(decltype(Particles.back())), Particles.data(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    RenderEngine->TransitionBarrier(CommandList, InstanceBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    // Create the views
    HeapAllocation->CreateBufferUAV(InstanceBuffer, Particles.size(), sizeof(decltype(Particles.back())));

    if (!UseCPU)
    {
        Solver->CreateBuffers(RenderEngineIn, CommandList, HeapAllocation.get());
    }

    uint32_t FenceValue = RenderEngine->ExecuteCommandList(CommandList);
    RenderEngine->WaitForFenceValue(FenceValue);
}

void FluidObject::Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList, const Math::Matrix& ViewMatrix)
{
    ObjectRenderer::Draw(CommandList, ViewMatrix);

    // Set buffer views
    CommandList->IASetVertexBuffers(0, 1, &SphereRenderer->GetVertexBufferView());
    if (HeapAllocation)
    {
        CommandList->SetGraphicsRootDescriptorTable(1, HeapAllocation->GPUHandle);
    }
    CommandList->IASetIndexBuffer(&SphereRenderer->GetIndexBufferView());
    CommandList->DrawIndexedInstanced(SphereRenderer->GetNumVertices(), Particles.size(), 0, 0, 0);
}

void FluidObject::Update(float DeltaTime)
{
    // CPU solve
    if (UseCPU)
    {
        Solver->CPUSolve(Particles, DeltaTime);
        if (InstanceBuffer && InstanceUploadBuffer)
        {
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList = RenderEngine->GetCommandList();
            RenderEngine->TransitionBarrier(CommandList, InstanceBuffer.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COMMON);
            RenderEngine->UploadDefaultBufferResource(CommandList, InstanceBuffer, InstanceUploadBuffer, Particles.size(), sizeof(decltype(Particles.back())), Particles.data(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            RenderEngine->TransitionBarrier(CommandList, InstanceBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            uint32_t FenceValue = RenderEngine->ExecuteCommandList(CommandList);
            RenderEngine->WaitForFenceValue(FenceValue);
        }
    }
    // GPU solve
    else
    {
        if (HeapAllocation)
        {
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList = RenderEngine->GetCommandList();

            CommandList->SetComputeRootSignature(RenderEngine->GetComputeRootSignature().Get());
            ID3D12DescriptorHeap* Heaps[] = {HeapAllocation->Allocator->GetHeap()};
            CommandList->SetDescriptorHeaps(1, Heaps);
            CommandList->SetComputeRootDescriptorTable(0, HeapAllocation->GPUHandle);

            Solver->GPUSolve(RenderEngine, CommandList, InstanceBuffer);

            RenderEngine->UAVBarrier(CommandList, InstanceBuffer.Get());
            RenderEngine->ExecuteCommandList(CommandList);
            RenderEngine->Flush();
        }
    }
}