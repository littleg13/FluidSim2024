#include "PrimitiveObject.h"

#include "PSOBuilder.h"
#include "Renderer.h"
#include "ShaderCompiler.h"
#include "primitives/Cube.h"
#include "primitives/Plane.h"
#include "primitives/Sphere.h"

ShaderDesc PrimitiveVertexShader = {
    L"D:\\Dev\\Projects\\FluidSim2024\\shaders\\VertexShader.hlsl",
    L"vs_6_0",
    L"main"};
ShaderDesc PrimitiveFragmentShader = {
    L"D:\\Dev\\Projects\\FluidSim2024\\shaders\\FragmentShader.hlsl",
    L"ps_6_0",
    L"main"};

template <typename T>
PrimitiveObject<T>::PrimitiveObject()
{
}

template <typename T>
PrimitiveObject<T>::~PrimitiveObject()
{
    DestroyBuffers();
}

template <typename T>
void PrimitiveObject<T>::Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList, const Math::Matrix& ViewMatrix)
{
    ObjectRenderer::Draw(CommandList, ViewMatrix);

    // Set buffer views
    CommandList->IASetVertexBuffers(0, 1, &GetVertexBufferView());
    CommandList->IASetIndexBuffer(&GetIndexBufferView());
    CommandList->DrawIndexedInstanced(GetNumVertices(), 1, 0, 0, 0);
}

template <typename T>
void PrimitiveObject<T>::RecompileShaders(ShaderCompiler& Compiler)
{
    Compiler.CompileShaderNonAsync(PrimitiveVertexShader, true);
    Compiler.CompileShaderNonAsync(PrimitiveFragmentShader, true);
}

template <typename T>
Microsoft::WRL::ComPtr<ID3D12PipelineState> PrimitiveObject<T>::CreatePipelineStateObject(ID3D12DevicePtr D3D12Device, ShaderCompiler& Compiler)
{
    PSOBuilder Builder;
    Builder.SetShaders(Compiler.GetShader(PrimitiveVertexShader), Compiler.GetShader(PrimitiveFragmentShader))
        .AddInputLayoutParameter({"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0})
        .AddInputLayoutParameter({"NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0})
        .AddRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
    return Builder.BuildGraphics(D3D12Device);
}

template <typename T>
void PrimitiveObject<T>::CreateBuffers(Renderer* RenderEngine)
{
    if (BuffersInitialized)
    {
        return;
    }

    if (T::RefCount)
    {
        VertexBuffer = T::VertexBuffer;
        IndexBuffer = T::IndexBuffer;
        T::RefCount++;
    }
    else
    {
        // Create and upload the buffers
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList = RenderEngine->GetCommandList();
        Microsoft::WRL::ComPtr<ID3D12Resource> VertexUploadBuffer;
        RenderEngine->UploadDefaultBufferResource(CommandList, VertexBuffer, VertexUploadBuffer, T::Vertices.size(), sizeof(decltype(T::Vertices.back())), T::Vertices.data(), D3D12_RESOURCE_FLAG_NONE);

        Microsoft::WRL::ComPtr<ID3D12Resource> IndexUploadBuffer;
        RenderEngine->UploadDefaultBufferResource(CommandList, IndexBuffer, IndexUploadBuffer, T::Indices.size(), sizeof(uint32_t), T::Indices.data(), D3D12_RESOURCE_FLAG_NONE);

        uint32_t FenceValue = RenderEngine->ExecuteCommandList(CommandList);

        T::VertexBuffer = VertexBuffer;
        T::IndexBuffer = IndexBuffer;
        T::RefCount++;
        RenderEngine->WaitForFenceValue(FenceValue);
    }

    // Create the views
    VertexBufferView.BufferLocation = VertexBuffer->GetGPUVirtualAddress();
    VertexBufferView.SizeInBytes = T::Vertices.size() * sizeof(decltype(T::Vertices.back()));
    VertexBufferView.StrideInBytes = sizeof(decltype(T::Vertices.back()));

    IndexBufferView.BufferLocation = IndexBuffer->GetGPUVirtualAddress();
    IndexBufferView.SizeInBytes = T::Indices.size() * sizeof(uint32_t);
    IndexBufferView.Format = DXGI_FORMAT_R32_UINT;

    BuffersInitialized = true;
}

template <typename T>
D3D12_VERTEX_BUFFER_VIEW& PrimitiveObject<T>::GetVertexBufferView()
{
    if (BuffersInitialized)
    {
        return VertexBufferView;
    }
    return VertexBufferView;
}

template <typename T>
D3D12_INDEX_BUFFER_VIEW& PrimitiveObject<T>::GetIndexBufferView()
{
    if (BuffersInitialized)
    {
        return IndexBufferView;
    }
    return IndexBufferView;
}

template <typename T>
int PrimitiveObject<T>::GetNumVertices()
{
    return T::Indices.size();
}

template <typename T>
void PrimitiveObject<T>::DestroyBuffers()
{
    if (!BuffersInitialized)
    {
        return;
    }
    VertexBuffer.Reset();
    IndexBuffer.Reset();
    T::RefCount--;
    if (T::RefCount == 0)
    {
        T::VertexBuffer.Reset();
        T::IndexBuffer.Reset();
    }
    BuffersInitialized = false;
}

template class PrimitiveObject<Cube>;
template class PrimitiveObject<Plane>;
template class PrimitiveObject<Sphere>;