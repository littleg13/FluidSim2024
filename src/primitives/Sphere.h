#include <d3d12.h>
#include <vector>
#include <wrl.h>

#include "util/3DMath.h"

class Sphere
{
public:
    struct alignas(Math::Vec4) Vertex
    {
        Math::Vec4 Position;
        Math::Vec4 Normal;
    };

    inline static std::vector<Vertex> Vertices = {};

    inline static std::vector<uint32_t> Indices = {};

    static void GenerateSphereData()
    {
        if (!Vertices.empty())
        {
            return;
        }

        const int Segments = 3;
        const int Rings = 3;
        const float Radius = 0.005f;

        // Generate vertices
        for (int ring = 0; ring <= Rings; ring++)
        {
            float Phi = PI * float(ring) / float(Rings);
            for (int segment = 0; segment <= Segments; segment++)
            {
                float Theta = 2.0f * PI * float(segment) / float(Segments);

                float X = Radius * std::sin(Phi) * std::cos(Theta);
                float Y = Radius * std::cos(Phi);
                float Z = Radius * std::sin(Phi) * std::sin(Theta);

                Vertex Vertex;
                Vertex.Position = {X, Y, Z, 1.0f};
                Vertex.Normal = {X, Y, Z, 0.0f}; // Normal is the same as position for a unit sphere
                Vertices.push_back(Vertex);

                int Current = ring * (Segments + 1) + segment;
                int Next = Current + Segments + 1;

                // Generate indices
                Indices.push_back(Current);
                Indices.push_back(Next);
                Indices.push_back(Current + 1);

                Indices.push_back(Current + 1);
                Indices.push_back(Next);
                Indices.push_back(Next + 1);
            }
        }
    }

    inline static Microsoft::WRL::ComPtr<ID3D12Resource> VertexBuffer;
    inline static Microsoft::WRL::ComPtr<ID3D12Resource> IndexBuffer;
    inline static int RefCount = 0;
};