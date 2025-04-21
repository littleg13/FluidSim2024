
#include "util/3DMath.h"
#include <d3d12.h>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <wrl.h>

typedef Microsoft::WRL::ComPtr<ID3D12Device2> ID3D12DevicePtr;

class ObjectRenderer;
class ShaderCompiler;

struct RenderGroup
{
    Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineStateObject;
    std::vector<ObjectRenderer*> Objects;
};

class Scene
{
public:
    Scene(ID3D12DevicePtr Device, ShaderCompiler& Compiler);

    void AddObject(ObjectRenderer* Object);
    void ReloadShaders();

    void HandleKeyPress(uint64_t wParam, bool isRepeat);

    void Update(double DeltaTime);
    void Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList, const Math::Matrix& ViewMatrix);

private:
    ID3D12DevicePtr D3D12Device;
    ShaderCompiler& Compiler;
    std::unordered_map<std::type_index, RenderGroup> RenderGroups;
};