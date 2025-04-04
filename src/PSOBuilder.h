#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d12.h>
#include <unordered_map>
#include <vector>
#include <wrl.h>

typedef Microsoft::WRL::ComPtr<ID3D12Device2> ID3D12DevicePtr;

class PSOBuilder
{
public:
    PSOBuilder();
    Microsoft::WRL::ComPtr<ID3D12PipelineState> BuildGraphics(ID3D12DevicePtr D3D12Device);
    Microsoft::WRL::ComPtr<ID3D12PipelineState> BuildCompute(ID3D12DevicePtr D3D12Device);
    PSOBuilder& SetShaders(Microsoft::WRL::ComPtr<ID3DBlob>& VShaderBlob, Microsoft::WRL::ComPtr<ID3DBlob>& FShaderBlob);
    PSOBuilder& SetComputeShader(Microsoft::WRL::ComPtr<ID3DBlob>& CShaderBlob);
    PSOBuilder& AddInputLayoutParameter(D3D12_INPUT_ELEMENT_DESC&& InputElementDesc);
    PSOBuilder& AddRenderTargetFormat(DXGI_FORMAT Format);
    PSOBuilder& SetPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY_TYPE Type);

    Microsoft::WRL::ComPtr<ID3D12RootSignature> BuildGraphicsRootSignature(ID3D12DevicePtr D3D12Device);
    Microsoft::WRL::ComPtr<ID3D12RootSignature> BuildComputeRootSignature(ID3D12DevicePtr D3D12Device);
    PSOBuilder& AddConstantRootParameter(uint32_t Num32BitValues, uint32_t RegisterSpace = 0);
    PSOBuilder& AddDescriptorTableRootParameter(uint32_t NumRanges, D3D12_DESCRIPTOR_RANGE1* Ranges);
    PSOBuilder& AddConstantBufferViewRootParameter(uint32_t RegisterSpace = 0);

    static Microsoft::WRL::ComPtr<ID3D12RootSignature> GraphicsRootSignature;
    static Microsoft::WRL::ComPtr<ID3D12RootSignature> ComputeRootSignature;

    inline static std::unordered_map<LPCWSTR, Microsoft::WRL::ComPtr<ID3DBlob>> LastKnownGoodShaders;

private:
    Microsoft::WRL::ComPtr<ID3DBlob> ComputeShaderBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> VertexShaderBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> FragmentShaderBlob;
    std::vector<D3D12_INPUT_ELEMENT_DESC> InputLayout;
    std::vector<D3D12_ROOT_PARAMETER1> RootParameters;
    D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    D3D12_RT_FORMAT_ARRAY RenderTargetFormats{};

    uint32_t ConstantBufferCount;
};

template <D3D12_PIPELINE_STATE_SUBOBJECT_TYPE SubObjectType, typename ValueType>
struct alignas(void*) PipelineStateSubobject
{
private:
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type = SubObjectType;

public:
    ValueType Value;
    PipelineStateSubobject& operator=(ValueType const& i)
    {
        Value = i;
        return *this;
    };
};

struct PipelineStateStream
{
    PipelineStateSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE, ID3D12RootSignature*> RootSignature;
    PipelineStateSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS, D3D12_SHADER_BYTECODE> VertexShader;
    PipelineStateSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS, D3D12_SHADER_BYTECODE> FragmentShader;
    PipelineStateSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT, D3D12_INPUT_LAYOUT_DESC> InputLayout;
    PipelineStateSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY, D3D12_PRIMITIVE_TOPOLOGY_TYPE> PrimitiveTopologyType;
    PipelineStateSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS, D3D12_RT_FORMAT_ARRAY> RenderTargetFormats;
    PipelineStateSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT, DXGI_FORMAT> DepthStencilFormat;
    PipelineStateSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER, D3D12_RASTERIZER_DESC> RasterizerState;
};

struct ComputePipelineStateStream
{
    PipelineStateSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE, ID3D12RootSignature*> RootSignature;
    PipelineStateSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS, D3D12_SHADER_BYTECODE> ComputeShader;
};