#pragma once

#include <d3d12.h>
#include <memory>
#include <queue>
#include <wrl.h>

class Allocation;

typedef Microsoft::WRL::ComPtr<ID3D12Device2> ID3D12DevicePtr;

class DescriptorHeapAllocator
{
public:
    DescriptorHeapAllocator(ID3D12DevicePtr D3D12Device, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DescriptorHeap);

    ID3D12DescriptorHeap* GetHeap();

    std::unique_ptr<Allocation> GetAllocation();
    void Dealloc(Allocation* Alloc);

private:
    friend class Allocation;
    ID3D12DevicePtr Device;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Heap;
    D3D12_CPU_DESCRIPTOR_HANDLE CurrentCPUHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE CurrentGPUHandle;
    std::queue<uint32_t> FreeBlocks;
    uint32_t DescriptorSize;
    uint32_t BlockSize = 8;
    uint32_t NumBlocks;
};

class Allocation
{
public:
    Allocation() {};
    Allocation(Allocation&& Other)
    {
        Allocator = Other.Allocator;
        CPUHandle = Other.CPUHandle;
        GPUHandle = Other.GPUHandle;
        NumDescriptors = Other.NumDescriptors;
        MaxDescriptors = Other.MaxDescriptors;
        BlockHandle = Other.BlockHandle;

        Other.Allocator = nullptr;
    };
    Allocation(const Allocation& Other) = delete;
    ~Allocation()
    {
        if (Allocator)
        {
            Allocator->Dealloc(this);
        }
    };

    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32_t Offset);
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32_t Offset);

    uint32_t CreateBufferUAV(Microsoft::WRL::ComPtr<ID3D12Resource> BufferResource, size_t NumElements, size_t ElementSize, DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN);
    uint32_t CreateBufferCBV(Microsoft::WRL::ComPtr<ID3D12Resource> BufferResource, size_t NumBytes);

    D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;
    uint32_t BlockHandle;
    uint32_t NumDescriptors = 0;
    uint32_t MaxDescriptors;
    DescriptorHeapAllocator* Allocator;
};