#pragma once

#include <vector>

#include "ObjectRenderer.h"
#include "util/3DMath.h"

typedef Microsoft::WRL::ComPtr<ID3D12Device2> ID3D12DevicePtr;

class Renderer;

template <typename T>
class PrimitiveObject : public ObjectRenderer
{
public:
    PrimitiveObject();
    ~PrimitiveObject();
    void CreateBuffers(Renderer* RenderEngine);
    void DestroyBuffers();

    void Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList, const Math::Matrix& ViewMatrix);

    D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView();
    D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView();
    virtual int GetNumVertices();
    virtual Microsoft::WRL::ComPtr<ID3D12PipelineState> CreatePipelineStateObject(ID3D12DevicePtr D3D12Device, ShaderCompiler& Compiler) override;
    virtual void RecompileShaders(ShaderCompiler& Compiler) override;

private:
    bool BuffersInitialized = false;
    Microsoft::WRL::ComPtr<ID3D12Resource> VertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> IndexBuffer;
    D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
    D3D12_INDEX_BUFFER_VIEW IndexBufferView;
};
