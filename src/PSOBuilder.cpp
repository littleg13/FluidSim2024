#include "PSOBuilder.h"

#include "dxc/dxcapi.h"
#include "util/RenderUtils.h"
#include <d3dcompiler.h>

Microsoft::WRL::ComPtr<ID3D12RootSignature> PSOBuilder::GraphicsRootSignature;
Microsoft::WRL::ComPtr<ID3D12RootSignature> PSOBuilder::ComputeRootSignature;

PSOBuilder::PSOBuilder()
    : ConstantBufferCount(0)
{
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> PSOBuilder::BuildGraphicsRootSignature(ID3D12DevicePtr D3D12Device)
{
    D3D12_ROOT_SIGNATURE_FLAGS RootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC RootSignatureDesc = {
        D3D_ROOT_SIGNATURE_VERSION_1_1,
        {.Desc_1_1 = {static_cast<uint32_t>(RootParameters.size()), RootParameters.data(), 0, nullptr, RootSignatureFlags}}};
    Microsoft::WRL::ComPtr<ID3DBlob> RootSignatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> ErrorBlob;
    RenderUtils::CreateDialogAndThrowIfFailed(D3D12SerializeVersionedRootSignature(&RootSignatureDesc, &RootSignatureBlob, &ErrorBlob));
    RenderUtils::CreateDialogAndThrowIfFailed(D3D12Device->CreateRootSignature(0, RootSignatureBlob->GetBufferPointer(), RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&GraphicsRootSignature)));

    return GraphicsRootSignature;
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> PSOBuilder::BuildComputeRootSignature(ID3D12DevicePtr D3D12Device)
{
    D3D12_ROOT_SIGNATURE_FLAGS RootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC RootSignatureDesc = {
        D3D_ROOT_SIGNATURE_VERSION_1_1,
        {.Desc_1_1 = {static_cast<uint32_t>(RootParameters.size()), RootParameters.data(), 0, nullptr, RootSignatureFlags}}};
    Microsoft::WRL::ComPtr<ID3DBlob> RootSignatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> ErrorBlob;
    RenderUtils::CreateDialogAndThrowIfFailed(D3D12SerializeVersionedRootSignature(&RootSignatureDesc, &RootSignatureBlob, &ErrorBlob));
    RenderUtils::CreateDialogAndThrowIfFailed(D3D12Device->CreateRootSignature(0, RootSignatureBlob->GetBufferPointer(), RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&ComputeRootSignature)));

    return ComputeRootSignature;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PSOBuilder::BuildGraphics(ID3D12DevicePtr D3D12Device)
{
    PipelineStateStream StateStream;
    StateStream.RootSignature = GraphicsRootSignature.Get();
    StateStream.VertexShader = D3D12_SHADER_BYTECODE{VertexShaderBlob->GetBufferPointer(), VertexShaderBlob->GetBufferSize()};
    StateStream.FragmentShader = D3D12_SHADER_BYTECODE{FragmentShaderBlob->GetBufferPointer(), FragmentShaderBlob->GetBufferSize()};
    StateStream.InputLayout = D3D12_INPUT_LAYOUT_DESC{InputLayout.data(), static_cast<uint32_t>(InputLayout.size())};
    StateStream.PrimitiveTopologyType = PrimitiveType;
    StateStream.RenderTargetFormats = RenderTargetFormats;
    StateStream.DepthStencilFormat = DXGI_FORMAT_D32_FLOAT;
    StateStream.RasterizerState = D3D12_RASTERIZER_DESC{D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_BACK, true, 0, 0, 0, true, false, false, 0, D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF};
    D3D12_PIPELINE_STATE_STREAM_DESC PipelineStateDesc = {sizeof(PipelineStateStream), &StateStream};

    Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineState;
    RenderUtils::CreateDialogAndThrowIfFailed(D3D12Device->CreatePipelineState(&PipelineStateDesc, IID_PPV_ARGS(&PipelineState)));
    return PipelineState;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PSOBuilder::BuildCompute(ID3D12DevicePtr D3D12Device)
{
    ComputePipelineStateStream StateStream;
    StateStream.RootSignature = ComputeRootSignature.Get();
    StateStream.ComputeShader = D3D12_SHADER_BYTECODE{ComputeShaderBlob->GetBufferPointer(), ComputeShaderBlob->GetBufferSize()};
    D3D12_PIPELINE_STATE_STREAM_DESC PipelineStateDesc = {sizeof(ComputePipelineStateStream), &StateStream};

    Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineState;
    RenderUtils::CreateDialogAndThrowIfFailed(D3D12Device->CreatePipelineState(&PipelineStateDesc, IID_PPV_ARGS(&PipelineState)));
    return PipelineState;
}

PSOBuilder& PSOBuilder::SetShaders(Microsoft::WRL::ComPtr<ID3DBlob>& VShaderBlob, Microsoft::WRL::ComPtr<ID3DBlob>& FShaderBlob)
{
    VertexShaderBlob = VShaderBlob;
    FragmentShaderBlob = FShaderBlob;
    return *this;
}

PSOBuilder& PSOBuilder::SetComputeShader(Microsoft::WRL::ComPtr<ID3DBlob>& CShaderBlob)
{
    ComputeShaderBlob = CShaderBlob;
    return *this;
}

PSOBuilder& PSOBuilder::AddInputLayoutParameter(D3D12_INPUT_ELEMENT_DESC&& InputElementDesc)
{
    InputLayout.emplace_back(std::move(InputElementDesc));
    return *this;
}

PSOBuilder& PSOBuilder::AddRenderTargetFormat(DXGI_FORMAT Format)
{
    if (RenderTargetFormats.NumRenderTargets >= 8)
    {
        return *this;
    }
    RenderTargetFormats.RTFormats[RenderTargetFormats.NumRenderTargets] = Format;
    RenderTargetFormats.NumRenderTargets += 1;
    return *this;
}
PSOBuilder& PSOBuilder::SetPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY_TYPE Type)
{
    PrimitiveType = Type;
    return *this;
}

PSOBuilder& PSOBuilder::AddConstantRootParameter(uint32_t Num32BitValues, uint32_t RegisterSpace)
{
    D3D12_ROOT_PARAMETER1 RootParameter = {
        D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
        {.Constants = {ConstantBufferCount, RegisterSpace, Num32BitValues}},
        D3D12_SHADER_VISIBILITY_ALL};
    RootParameters.emplace_back(RootParameter);
    ConstantBufferCount++;
    return *this;
}

PSOBuilder& PSOBuilder::AddDescriptorTableRootParameter(uint32_t NumRanges, D3D12_DESCRIPTOR_RANGE1* Ranges)
{
    D3D12_ROOT_PARAMETER1 RootParameter = {
        D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
        {.DescriptorTable = {NumRanges, Ranges}},
        D3D12_SHADER_VISIBILITY_ALL};
    RootParameters.emplace_back(RootParameter);
    return *this;
}

PSOBuilder& PSOBuilder::AddConstantBufferViewRootParameter(uint32_t RegisterSpace)
{
    D3D12_ROOT_PARAMETER1 RootParameter = {
        D3D12_ROOT_PARAMETER_TYPE_CBV,
        {.Descriptor = {ConstantBufferCount, RegisterSpace}},
        D3D12_SHADER_VISIBILITY_ALL};
    RootParameters.emplace_back(RootParameter);
    ConstantBufferCount++;
    return *this;
}