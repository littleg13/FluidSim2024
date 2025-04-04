#include <d3d12.h>
#include <vector>
#include <wrl.h>

#include "util/3DMath.h"

class Plane
{
public:
    struct alignas(Math::Vec4) Vertex
    {
        Math::Vec4 Position;
        Math::Vec4 Normal;
    };

    inline static const std::vector<Vertex> Vertices = {
        {{-0.5f, 0.0f, -0.5f, 1.0f}, {0, 1.0f, 0, 0}}, // 0
        {{0.5f, 0.0f, -0.5f, 1.0f}, {0, 1.0f, 0, 0}},  // 1
        {{0.5f, 0.0f, 0.5f, 1.0f}, {0, 1.0f, 0, 0}},   // 2
        {{-0.5f, 0.0f, 0.5f, 1.0f}, {0, 1.0f, 0, 0}}}; // 3

    inline static std::vector<uint32_t> Indices{
        2, 1, 0,
        0, 3, 2};

    inline static Microsoft::WRL::ComPtr<ID3D12Resource> VertexBuffer;
    inline static Microsoft::WRL::ComPtr<ID3D12Resource> IndexBuffer;
    inline static int RefCount = 0;

private:
};