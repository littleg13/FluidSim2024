#include "ObjectRenderer.h"

#include "Renderer.h"
#include <d3d12.h>

ObjectRenderer::ObjectRenderer()
{
}

ObjectRenderer::~ObjectRenderer()
{
}

void ObjectRenderer::ApplyRotation(const Math::Matrix& Transform)
{
    Rotation = Math::Multiply(Transform, Rotation);
}

void ObjectRenderer::ApplyTranslation(const Math::Vec4& Transform)
{
    Translation += Transform;
}

void ObjectRenderer::SetTranslation(const Math::Vec4& Transform)
{
    Translation = Transform;
}

Math::Matrix ObjectRenderer::GetModelMatrix()
{
    return Math::TransformationMatrix(Rotation, Translation);
}

void ObjectRenderer::Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList, const Math::Matrix& ViewMatrix)
{
    CommandList->IASetPrimitiveTopology(PrimitiveType);
    Math::Matrix MVMatrix = Math::Multiply(ViewMatrix, GetModelMatrix());
    CommandList->SetGraphicsRoot32BitConstants(0, sizeof(Math::Matrix) / 4, &MVMatrix, 16);
}