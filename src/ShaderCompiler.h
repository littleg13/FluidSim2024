#pragma once

#include <condition_variable>
#include <d3d12.h>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <wrl.h>

#define SHADER_COMPILATION_THREADS 2

class IDxcUtils;
class IDxcCompiler3;

struct ShaderDesc
{
    LPCWSTR FileName;
    LPCWSTR Target;
    LPCWSTR EntryPoint = L"main";
};

struct CompileWorkItem
{
    ShaderDesc Desc;
    bool ErrorOnFail;
    std::function<void(bool)> Completion;
};

class ShaderCompiler
{
public:
    ShaderCompiler();
    ~ShaderCompiler();

    Microsoft::WRL::ComPtr<ID3DBlob> GetShader(const ShaderDesc& ShaderDesc);
    void CompileShader(const ShaderDesc& Desc, std::function<void(bool)> Completion, bool ErrorOnFail = false);
    void CompileShaderNonAsync(const ShaderDesc& Desc, bool ErrorOnFail = false);

private:
    bool CompileShaderFromFile(const ShaderDesc& ShaderDesc, bool ErrorOnFail = false);
    void CompilationThreadRunner();
    std::vector<std::thread> WorkerThreads;
    std::condition_variable WorkCV;
    std::mutex CVMutex;

    std::queue<CompileWorkItem> WorkQueue;
    std::mutex QueueMutex;

    Microsoft::WRL::ComPtr<IDxcUtils> DxcUtils;
    Microsoft::WRL::ComPtr<IDxcCompiler3> DxcCompiler;

    std::unordered_map<LPCWSTR, Microsoft::WRL::ComPtr<ID3DBlob>> LastKnownGoodShaders;
    std::recursive_mutex LastKnownGoodMutex;
};