#include "Renderer.h"
#include <chrono>
// DirectX 12 specific headers.
#include <dxgi1_6.h>

#include "DescriptorHeapAllocator.h"
#include "ObjectRenderer.h"
#include "Scene.h"
#include "View.h"
#include "util/RenderUtils.h"

Renderer::Renderer(ShaderCompiler& Compiler) : Compiler(Compiler)
{
    for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
    {
        FrameFenceValues[i] = 0;
    }
}

Renderer::~Renderer()
{
    RenderUtils::CreateDialogAndThrowIfFailed(D3D12CommandQueue->Signal(D3D12Fence.Get(), ++FenceValue));
    WaitForFenceValue(D3D12Fence, FenceValue, FenceEvent);

    delete CBVSRVUAVHeapAllocator;
    CloseHandle(FenceEvent);
    RenderUtils::CreateDialogAndThrowIfFailed(D3D12Device->GetDeviceRemovedReason());
}

bool Renderer::IsInitialized()
{
    return Initialized;
}

ShaderCompiler& Renderer::GetCompiler()
{
    return Compiler;
}

void Renderer::Render()
{
    UINT CurrentBackBufferIndex = DxgiSwapChain->GetCurrentBackBufferIndex();
    auto BackBuffer = BackBuffers[CurrentBackBufferIndex];
    D3D12_CPU_DESCRIPTOR_HANDLE RTV;
    RTV.ptr = D3D12DescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CurrentBackBufferIndex * D3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE DSV = DepthDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList = GetCommandList();

    // Clear the render target.
    {
        TransitionBarrier(CommandList, BackBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

        FLOAT clearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
        CommandList->ClearRenderTargetView(RTV, clearColor, 0, nullptr);
        CommandList->ClearDepthStencilView(DepthDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    }

    // Draw
    {
        CommandList->SetGraphicsRootSignature(GraphicsRootSignature.Get());

        CommandList->RSSetViewports(1, &Viewport);
        CommandList->RSSetScissorRects(1, &ScissorRect);

        CommandList->OMSetRenderTargets(1, &RTV, false, &DSV);
        CommandList->SetGraphicsRoot32BitConstants(0, sizeof(Math::Matrix) / 4, &PerspectiveMatrix, 0);
        ID3D12DescriptorHeap* Heaps[] = {CBVSRVUAVHeapAllocator->GetHeap()};
        CommandList->SetDescriptorHeaps(1, Heaps);

        CurrentScene->Draw(CommandList, CurrentView->GetMatrix());
    }

    // Present
    {
        TransitionBarrier(CommandList, BackBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

        FrameFenceValues[CurrentBackBufferIndex] = ExecuteCommandList(CommandList);

        UINT syncInterval = 0;
        UINT presentFlags = DXGI_PRESENT_ALLOW_TEARING;
        RenderUtils::CreateDialogAndThrowIfFailed(DxgiSwapChain->Present(syncInterval, presentFlags));

        WaitForFenceValue(D3D12Fence, FrameFenceValues[DxgiSwapChain->GetCurrentBackBufferIndex()], FenceEvent);
    }
}

void Renderer::Init(HWND HWnd, uint32_t Width, uint32_t Height)
{
    ClientWidth = Width;
    ClientHeight = Height;
    ScissorRect = D3D12_RECT{0, 0, LONG_MAX, LONG_MAX};
    Viewport = D3D12_VIEWPORT{0.0f, 0.0f, static_cast<float>(Width), static_cast<float>(Height), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH};
    EnableDebugLayer();
    DxgiAdapter = FindAdapter();
    D3D12Device = CreateDevice(DxgiAdapter);
    D3D12CommandQueue = CreateCommandQueue(D3D12Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    DxgiSwapChain = CreateSwapChain(HWnd, D3D12CommandQueue, ClientWidth, ClientHeight, NUM_SWAP_BUFFERS);
    D3D12DescriptorHeap = CreateDescriptorHeap(D3D12Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, NUM_SWAP_BUFFERS);
    DepthDescriptorHeap = CreateDescriptorHeap(D3D12Device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);
    UpdateRenderTargetViews(D3D12Device, DxgiSwapChain, D3D12DescriptorHeap, BackBuffers);
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Heap = CreateDescriptorHeap(D3D12Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2048, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
    CBVSRVUAVHeapAllocator = new DescriptorHeapAllocator(D3D12Device, Heap);

    CreateDepthBuffer();

    RenderUtils::CreateDialogAndThrowIfFailed(D3D12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&D3D12Fence)));

    FenceEvent = CreateEvent(NULL, false, false, "FenceEvent");

    Initialized = true;
}

void Renderer::TransitionBarrier(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList, ID3D12Resource* Resource, D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter, UINT Subresource)
{
    D3D12_RESOURCE_BARRIER Barrier;
    Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    Barrier.Transition.pResource = Resource;
    Barrier.Transition.StateBefore = StateBefore;
    Barrier.Transition.StateAfter = StateAfter;
    Barrier.Transition.Subresource = Subresource;
    CommandList->ResourceBarrier(1, &Barrier);
}

void Renderer::UAVBarrier(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList, ID3D12Resource* Resource)
{
    D3D12_RESOURCE_BARRIER Barrier;
    Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    Barrier.UAV.pResource = Resource;
    CommandList->ResourceBarrier(1, &Barrier);
}

std::unique_ptr<Allocation> Renderer::GetAllocation()
{
    return CBVSRVUAVHeapAllocator->GetAllocation();
}

void Renderer::UploadDefaultBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList, Microsoft::WRL::ComPtr<ID3D12Resource>& BufferResource, Microsoft::WRL::ComPtr<ID3D12Resource>& UploadResource, size_t NumElements, size_t ElementSize, const void* BufferData, D3D12_RESOURCE_FLAGS Flags, size_t MinSize)
{
    if (!BufferData)
    {
        return;
    }

    UINT64 NumBytes = NumElements * ElementSize;
    UINT64 NumBufferBytes = MinSize;
    if (NumBytes > MinSize)
    {
        NumBufferBytes = NumBytes;
    }

    // Create the upload buffer
    if (!UploadResource)
    {
        UploadResource = CreateBufferResource(D3D12_HEAP_TYPE_UPLOAD, NumBufferBytes, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE);
    }

    // Create the default buffer
    if (!BufferResource)
    {
        BufferResource = CreateBufferResource(D3D12_HEAP_TYPE_DEFAULT, NumBufferBytes, D3D12_RESOURCE_STATE_COMMON, Flags);
    }

    // Map/copy the upload destination
    void* UploadData;
    RenderUtils::CreateDialogAndThrowIfFailed(UploadResource->Map(0, NULL, &UploadData));
    memcpy(UploadData, BufferData, NumBytes);

    // Submit command to copy from upload to default
    CommandList->CopyBufferRegion(BufferResource.Get(), 0, UploadResource.Get(), 0, NumBytes);
}

Microsoft::WRL::ComPtr<ID3D12Resource> Renderer::CreateBufferResource(D3D12_HEAP_TYPE HeapType, UINT64 NumBytes, D3D12_RESOURCE_STATES InitialState, D3D12_RESOURCE_FLAGS Flags)
{
    D3D12_HEAP_PROPERTIES HeapProperties = {HeapType};
    D3D12_RESOURCE_DESC Desc = {D3D12_RESOURCE_DIMENSION_BUFFER, 0, NumBytes, 1, 1, 1, DXGI_FORMAT_UNKNOWN, {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, Flags};
    Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
    RenderUtils::CreateDialogAndThrowIfFailed(D3D12Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE, &Desc, InitialState, nullptr, IID_PPV_ARGS(&Resource)));
    return Resource;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> Renderer::GetCommandList()
{
    // Grab an allocator
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator;
    if (TrackedCommandAllocators.empty() || D3D12Fence->GetCompletedValue() < TrackedCommandAllocators.front().FenceValue)
    {
        CommandAllocator = CreateCommandAllocator(D3D12Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    }
    else
    {
        CommandAllocator = TrackedCommandAllocators.front().Allocator;
        TrackedCommandAllocators.pop();
        CommandAllocator->Reset();
    }

    // Grab a list
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList;
    if (!AvailableCommandLists.empty())
    {
        CommandList = AvailableCommandLists.front();
        AvailableCommandLists.pop();
        RenderUtils::CreateDialogAndThrowIfFailed(CommandList->Reset(CommandAllocator.Get(), nullptr));
    }
    else
    {
        CommandList = CreateCommandList(D3D12Device, CommandAllocator, D3D12_COMMAND_LIST_TYPE_DIRECT);
    }
    RenderUtils::CreateDialogAndThrowIfFailed(CommandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), CommandAllocator.Get()));
    return CommandList;
}

uint32_t Renderer::ExecuteCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList)
{
    CommandList->Close();

    ID3D12CommandAllocator* CommandAllocator;
    UINT AllocatorSize = sizeof(CommandAllocator);
    RenderUtils::CreateDialogAndThrowIfFailed(CommandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &AllocatorSize, &CommandAllocator));

    ID3D12CommandList* const CommandLists[] = {CommandList.Get()};
    D3D12CommandQueue->ExecuteCommandLists(1, CommandLists);
    RenderUtils::CreateDialogAndThrowIfFailed(D3D12CommandQueue->Signal(D3D12Fence.Get(), ++FenceValue));

    TrackedCommandAllocators.emplace(CommandAllocatorTracker{FenceValue, CommandAllocator});
    AvailableCommandLists.emplace(CommandList);

    CommandAllocator->Release();

    return FenceValue;
}

ID3D12DevicePtr Renderer::GetDevice()
{
    if (Initialized)
    {
        return D3D12Device;
    }
    return nullptr;
}

Scene* Renderer::GetCurrentScene()
{
    return CurrentScene;
}

void Renderer::SetCurrentScene(Scene* NewScene)
{
    CurrentScene = NewScene;
}

void Renderer::SetCurrentView(View* NewView)
{
    CurrentView = NewView;
}

void Renderer::SetGraphicsRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature)
{
    GraphicsRootSignature = RootSignature;

    float AspectRatio = ClientWidth / static_cast<float>(ClientHeight);
    PerspectiveMatrix = std::move(Math::PerspectiveMatrix(90.0f, AspectRatio, 0.1f, 100.0f));
}

void Renderer::SetComputeRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature)
{
    ComputeRootSignature = RootSignature;
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> Renderer::GetComputeRootSignature()
{
    return ComputeRootSignature;
}

void Renderer::Flush()
{
    // Flush
    RenderUtils::CreateDialogAndThrowIfFailed(D3D12CommandQueue->Signal(D3D12Fence.Get(), ++FenceValue));
    WaitForFenceValue(D3D12Fence, FenceValue, FenceEvent);
}

void Renderer::Resize(uint32_t Width, uint32_t Height)
{
    if (Width == 0 || Height == 0)
    {
        return;
    }
    if (Width != ClientWidth || Height != ClientHeight)
    {
        ClientWidth = Width;
        ClientHeight = Height;

        Viewport = D3D12_VIEWPORT{0.0f, 0.0f, static_cast<float>(Width), static_cast<float>(Height), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH};
        float AspectRatio = ClientWidth / static_cast<float>(ClientHeight);
        PerspectiveMatrix = std::move(Math::PerspectiveMatrix(90.0f, AspectRatio, 0.1f, 10.0f));

        // Flush
        RenderUtils::CreateDialogAndThrowIfFailed(D3D12CommandQueue->Signal(D3D12Fence.Get(), ++FenceValue));
        WaitForFenceValue(D3D12Fence, FenceValue, FenceEvent);

        for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
        {
            // Any references to the back buffers must be released
            // before the swap chain can be resized.
            BackBuffers[i].Reset();
            FrameFenceValues[i] = FrameFenceValues[DxgiSwapChain->GetCurrentBackBufferIndex()];
        }

        DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
        RenderUtils::CreateDialogAndThrowIfFailed(DxgiSwapChain->GetDesc(&SwapChainDesc));
        RenderUtils::CreateDialogAndThrowIfFailed(DxgiSwapChain->ResizeBuffers(NUM_SWAP_BUFFERS, ClientWidth, ClientHeight, SwapChainDesc.BufferDesc.Format, SwapChainDesc.Flags));

        UpdateRenderTargetViews(D3D12Device, DxgiSwapChain, D3D12DescriptorHeap, BackBuffers);
        CreateDepthBuffer();
    }
}

void Renderer::CreateDepthBuffer()
{
    D3D12_CLEAR_VALUE ClearValue;
    ClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    ClearValue.DepthStencil = {1.0f, 0};

    D3D12_HEAP_PROPERTIES HeapProperties = {D3D12_HEAP_TYPE_DEFAULT};
    D3D12_RESOURCE_DESC ResourceDesc = {D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0, ClientWidth, ClientHeight, 1, 0, DXGI_FORMAT_D32_FLOAT, {1, 0}, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL};
    RenderUtils::CreateDialogAndThrowIfFailed(D3D12Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &ClearValue, IID_PPV_ARGS(&DepthBuffer)));

    D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
    DSVDesc.Format = DXGI_FORMAT_D32_FLOAT;
    DSVDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    DSVDesc.Texture2D.MipSlice = 0;
    DSVDesc.Flags = D3D12_DSV_FLAG_NONE;

    D3D12Device->CreateDepthStencilView(DepthBuffer.Get(), &DSVDesc, DepthDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

void Renderer::WaitForFenceValue(uint64_t FenceValue, std::chrono::milliseconds Duration)
{
    WaitForFenceValue(D3D12Fence, FenceValue, FenceEvent, Duration);
}

void Renderer::WaitForFenceValue(Microsoft::WRL::ComPtr<ID3D12Fence> D3D12Fence, uint64_t FenceValue, HANDLE FenceEvent, std::chrono::milliseconds Duration)
{
    if (D3D12Fence->GetCompletedValue() < FenceValue)
    {
        RenderUtils::CreateDialogAndThrowIfFailed(D3D12Fence->SetEventOnCompletion(FenceValue, FenceEvent));
        WaitForSingleObject(FenceEvent, static_cast<DWORD>(Duration.count()));
    }
}

void Renderer::EnableDebugLayer()
{
#if defined(_DEBUG)
    // Always enable the debug layer before doing anything DX12 related
    // so all possible errors generated while creating DX12 objects
    // are caught by the debug layer.
    Microsoft::WRL::ComPtr<ID3D12Debug> DebugInterface;
    RenderUtils::CreateDialogAndThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&DebugInterface)));
    DebugInterface->EnableDebugLayer();
#endif
}

IDXGIAdapterPtr Renderer::FindAdapter()
{
    IDXGIFactoryPtr DxgiFactory;
    UINT CreateFactoryFlags = 0;
#if defined(_DEBUG)
    CreateFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    RenderUtils::CreateDialogAndThrowIfFailed(CreateDXGIFactory2(CreateFactoryFlags, IID_PPV_ARGS(&DxgiFactory)));

    Microsoft::WRL::ComPtr<IDXGIAdapter1> DxgiAdapter1;
    IDXGIAdapterPtr DxgiAdapter4;

    SIZE_T MaxDedicatedVideoMemory = 0;
    for (UINT I = 0; DxgiFactory->EnumAdapters1(I, &DxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++I)
    {
        DXGI_ADAPTER_DESC1 DxgiAdapterDesc1;
        DxgiAdapter1->GetDesc1(&DxgiAdapterDesc1);
        if ((DxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
            SUCCEEDED(D3D12CreateDevice(DxgiAdapter1.Get(),
                                        D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
            DxgiAdapterDesc1.DedicatedVideoMemory > MaxDedicatedVideoMemory)
        {
            MaxDedicatedVideoMemory = DxgiAdapterDesc1.DedicatedVideoMemory;
            RenderUtils::CreateDialogAndThrowIfFailed(DxgiAdapter1.As(&DxgiAdapter4));
        }
    }

    return DxgiAdapter4;
}

Microsoft::WRL::ComPtr<ID3D12CommandQueue> Renderer::CreateCommandQueue(ID3D12DevicePtr D3D12Device, D3D12_COMMAND_LIST_TYPE Type)
{
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> D3D12CommandQueue;

    D3D12_COMMAND_QUEUE_DESC Desc = {};
    Desc.Type = Type;
    Desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    Desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    Desc.NodeMask = 0;

    RenderUtils::CreateDialogAndThrowIfFailed(D3D12Device->CreateCommandQueue(&Desc, IID_PPV_ARGS(&D3D12CommandQueue)));

    return D3D12CommandQueue;
}

IDXGISwapChainPtr Renderer::CreateSwapChain(HWND HWnd, Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandQueue, uint32_t Width, uint32_t Height, uint32_t BufferCount)
{
    IDXGISwapChainPtr DxgiSwapChain;
    IDXGIFactoryPtr DxgiFactory;
    UINT CreateFactoryFlags = 0;
#if defined(_DEBUG)
    CreateFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    RenderUtils::CreateDialogAndThrowIfFailed(CreateDXGIFactory2(CreateFactoryFlags, IID_PPV_ARGS(&DxgiFactory)));

    DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
    SwapChainDesc.Width = Width;
    SwapChainDesc.Height = Height;
    SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    SwapChainDesc.Stereo = FALSE;
    SwapChainDesc.SampleDesc = {1, 0};
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.BufferCount = BufferCount;
    SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

    SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    Microsoft::WRL::ComPtr<IDXGISwapChain1> SwapChain1;
    RenderUtils::CreateDialogAndThrowIfFailed(DxgiFactory->CreateSwapChainForHwnd(
        CommandQueue.Get(),
        HWnd,
        &SwapChainDesc,
        nullptr,
        nullptr,
        &SwapChain1));

    // CreateDialogAndThrowIfFailed(DxgiFactory->MakeWindowAssociation(HWnd, DXGI_MWA_NO_ALT_ENTER));

    RenderUtils::CreateDialogAndThrowIfFailed(SwapChain1.As(&DxgiSwapChain));

    return DxgiSwapChain;
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Renderer::CreateDescriptorHeap(ID3D12DevicePtr Device, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t NumDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS Flags)
{
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DescriptorHeap;

    D3D12_DESCRIPTOR_HEAP_DESC Desc = {};
    Desc.NumDescriptors = NumDescriptors;
    Desc.Type = Type;
    Desc.Flags = Flags;

    RenderUtils::CreateDialogAndThrowIfFailed(Device->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&DescriptorHeap)));

    return DescriptorHeap;
}

void Renderer::UpdateRenderTargetViews(ID3D12DevicePtr Device, IDXGISwapChainPtr SwapChain, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DescriptorHeap, Microsoft::WRL::ComPtr<ID3D12Resource> BackBuffers[])
{
    auto RTVDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle(DescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    for (int i = 0; i < NUM_SWAP_BUFFERS; ++i)
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> BackBuffer;
        RenderUtils::CreateDialogAndThrowIfFailed(SwapChain->GetBuffer(i, IID_PPV_ARGS(&BackBuffer)));

        Device->CreateRenderTargetView(BackBuffer.Get(), nullptr, RTVHandle);

        BackBuffers[i] = BackBuffer;

        RTVHandle.ptr += Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }
}

Microsoft::WRL::ComPtr<ID3D12CommandAllocator> Renderer::CreateCommandAllocator(ID3D12DevicePtr Device, D3D12_COMMAND_LIST_TYPE Type)
{
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator;
    RenderUtils::CreateDialogAndThrowIfFailed(Device->CreateCommandAllocator(Type, IID_PPV_ARGS(&CommandAllocator)));

    return CommandAllocator;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> Renderer::CreateCommandList(ID3D12DevicePtr Device, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator, D3D12_COMMAND_LIST_TYPE Type)
{
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList;
    RenderUtils::CreateDialogAndThrowIfFailed(Device->CreateCommandList(0, Type, CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&CommandList)));

    return CommandList;
}

ID3D12DevicePtr Renderer::CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> Adapter)
{
    ID3D12DevicePtr D3D12Device;
    RenderUtils::CreateDialogAndThrowIfFailed(D3D12CreateDevice(Adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&D3D12Device)));
    return D3D12Device;
}
