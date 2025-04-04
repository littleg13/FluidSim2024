#include "DescriptorHeapAllocator.h"

DescriptorHeapAllocator::DescriptorHeapAllocator(ID3D12DevicePtr D3D12Device, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DescriptorHeap)
    : Device(D3D12Device), Heap(DescriptorHeap), CurrentCPUHandle(Heap->GetCPUDescriptorHandleForHeapStart()), CurrentGPUHandle(Heap->GetGPUDescriptorHandleForHeapStart()), NumBlocks(Heap->GetDesc().NumDescriptors / BlockSize)
{
    DescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    for (int i = 0; i < NumBlocks; i++)
    {
        FreeBlocks.emplace(i);
    }
}

ID3D12DescriptorHeap* DescriptorHeapAllocator::GetHeap()
{
    return Heap.Get();
}

std::unique_ptr<Allocation> DescriptorHeapAllocator::GetAllocation()
{

    if (!FreeBlocks.empty())
    {
        std::unique_ptr<Allocation> Alloc = std::make_unique<Allocation>();
        Alloc->BlockHandle = FreeBlocks.front();
        FreeBlocks.pop();
        Alloc->CPUHandle.ptr = Heap->GetCPUDescriptorHandleForHeapStart().ptr + DescriptorSize * Alloc->BlockHandle * BlockSize;
        Alloc->GPUHandle.ptr = Heap->GetGPUDescriptorHandleForHeapStart().ptr + DescriptorSize * Alloc->BlockHandle * BlockSize;
        Alloc->MaxDescriptors = BlockSize;
        Alloc->Allocator = this;
        return Alloc;
    }
    return nullptr;
}

void DescriptorHeapAllocator::Dealloc(Allocation* Alloc)
{
    FreeBlocks.push(Alloc->BlockHandle);
}

D3D12_GPU_DESCRIPTOR_HANDLE Allocation::GetGPUHandle(uint32_t Offset)
{
    D3D12_GPU_DESCRIPTOR_HANDLE Handle = GPUHandle;
    Handle.ptr += Offset * Allocator->DescriptorSize;
    return Handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE Allocation::GetCPUHandle(uint32_t Offset)
{
    D3D12_CPU_DESCRIPTOR_HANDLE Handle = CPUHandle;
    Handle.ptr += Offset * Allocator->DescriptorSize;
    return Handle;
}

uint32_t Allocation::CreateBufferUAV(Microsoft::WRL::ComPtr<ID3D12Resource> BufferResource, size_t NumElements, size_t ElementSize, DXGI_FORMAT Format)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC Desc = {Format, D3D12_UAV_DIMENSION_BUFFER};
    Desc.Buffer = {0, UINT(NumElements), UINT(ElementSize), 0, D3D12_BUFFER_UAV_FLAG_NONE};

    D3D12_CPU_DESCRIPTOR_HANDLE CurrentCPUHandle = CPUHandle;
    CurrentCPUHandle.ptr += Allocator->DescriptorSize * NumDescriptors;
    Allocator->Device->CreateUnorderedAccessView(BufferResource.Get(), nullptr, &Desc, CurrentCPUHandle);
    NumDescriptors++;
    return NumDescriptors - 1;
}

uint32_t Allocation::CreateBufferCBV(Microsoft::WRL::ComPtr<ID3D12Resource> BufferResource, size_t NumBytes)
{
    D3D12_CONSTANT_BUFFER_VIEW_DESC Desc = {BufferResource->GetGPUVirtualAddress(), UINT(NumBytes)};

    D3D12_CPU_DESCRIPTOR_HANDLE CurrentCPUHandle = CPUHandle;
    CurrentCPUHandle.ptr += Allocator->DescriptorSize * NumDescriptors;
    Allocator->Device->CreateConstantBufferView(&Desc, CurrentCPUHandle);
    NumDescriptors++;
    return NumDescriptors - 1;
}