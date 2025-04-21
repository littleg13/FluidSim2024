#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "util/3DMath.h"
#include <d3d12.h>
#include <vector>
#include <wrl.h>

typedef Microsoft::WRL::ComPtr<ID3D12Device2> ID3D12DevicePtr;

class Renderer;
class ShaderCompiler;

class ObjectRenderer
{
public:
    ObjectRenderer();
    ~ObjectRenderer();

    virtual void Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList, const Math::Matrix& ViewMatrix);

    virtual void Update(float DeltaTime) {};

    virtual void HandleKeyPress(uint64_t wParam, bool isRepeat) {};

    void ApplyRotation(const Math::Matrix& Transform);
    void ApplyTranslation(const Math::Vec4& Transform);
    void SetTranslation(const Math::Vec4& Transform);
    Math::Matrix GetModelMatrix();

    virtual Microsoft::WRL::ComPtr<ID3D12PipelineState> CreatePipelineStateObject(ID3D12DevicePtr D3D12Device, ShaderCompiler& Compiler) = 0;
    virtual void RecompileShaders(ShaderCompiler& Compiler) = 0;

protected:
    D3D_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

private:
    Math::Matrix Rotation = {Math::Identity};
    Math::Vec4 Translation = Math::Vec4();
};