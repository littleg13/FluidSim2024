#include "Scene.h"

#include "ObjectRenderer.h"

Scene::Scene(ID3D12DevicePtr Device, ShaderCompiler& Compiler)
    : D3D12Device(Device), Compiler(Compiler)
{
}

void Scene::AddObject(ObjectRenderer* Object)
{
    std::type_index TypeIndex = std::type_index(typeid(*Object));
    if (auto Iter = RenderGroups.find(TypeIndex); Iter != RenderGroups.end())
    {
        Iter->second.Objects.emplace_back(Object);
    }
    else
    {
        RenderGroup Group = {
            Object->CreatePipelineStateObject(D3D12Device, Compiler),
            {Object}};
        RenderGroups.insert({TypeIndex, Group});
    }
}

void Scene::ReloadShaders()
{
    for (auto& [typeID, renderGroup] : RenderGroups)
    {
        if (!renderGroup.Objects.empty())
        {
            renderGroup.Objects[0]->RecompileShaders(Compiler);
            RenderGroups[typeID].PipelineStateObject = renderGroup.Objects[0]->CreatePipelineStateObject(D3D12Device, Compiler);
        }
    }
}

void Scene::Update(double DeltaTime)
{
    for (auto& [typeID, renderGroup] : RenderGroups)
    {
        for (auto object : renderGroup.Objects)
        {
            object->Update(DeltaTime);
        }
    }
}

void Scene::Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList, const Math::Matrix& ViewMatrix)
{
    for (auto& [typeID, renderGroup] : RenderGroups)
    {
        CommandList->SetPipelineState(renderGroup.PipelineStateObject.Get());
        for (auto object : renderGroup.Objects)
        {
            object->Draw(CommandList, ViewMatrix);
        }
    }
}