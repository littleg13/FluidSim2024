#include "ShaderCompiler.h"

#include "dxc/dxcapi.h"
#include "util/RenderUtils.h"

ShaderCompiler::ShaderCompiler()
{
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&DxcUtils));
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&DxcCompiler));

    for (int i = 0; i < SHADER_COMPILATION_THREADS; i++)
    {
        WorkerThreads.emplace_back(std::thread(&ShaderCompiler::CompilationThreadRunner, this));
    }
}
ShaderCompiler::~ShaderCompiler()
{
    WorkCV.notify_all();
    for (auto& Thread : WorkerThreads)
    {
        Thread.join();
    }
}

Microsoft::WRL::ComPtr<ID3DBlob> ShaderCompiler::GetShader(const ShaderDesc& ShaderDesc)
{
    std::lock_guard<std::recursive_mutex> Lock(LastKnownGoodMutex);
    if (auto LastKnownGood = LastKnownGoodShaders.find(ShaderDesc.FileName); LastKnownGood != LastKnownGoodShaders.end())
    {
        return LastKnownGood->second;
    }
    if (CompileShaderFromFile(ShaderDesc, true))
    {
        return LastKnownGoodShaders[ShaderDesc.FileName];
    }
    return nullptr;
}

void ShaderCompiler::CompileShader(const ShaderDesc& Desc, std::function<void(bool)> Completion, bool ErrorOnFail)
{
    CompileWorkItem WorkItem = {
        Desc,
        ErrorOnFail,
        Completion};

    std::lock_guard<std::mutex> Lock(QueueMutex);
    WorkQueue.push(WorkItem);
    WorkCV.notify_one();
}

void ShaderCompiler::CompileShaderNonAsync(const ShaderDesc& Desc, bool ErrorOnFail)
{
    CompileShaderFromFile(Desc, ErrorOnFail);
}

void ShaderCompiler::CompilationThreadRunner()
{
    CompileWorkItem WorkItem;
    while (true)
    {
        // Grab work if available
        std::unique_lock<std::mutex> QueueLock(QueueMutex);
        if (!WorkQueue.empty())
        {
            WorkItem = WorkQueue.front();
            WorkQueue.pop();
            QueueLock.unlock();

            // Work work
            WorkItem.Completion(CompileShaderFromFile(WorkItem.Desc, WorkItem.ErrorOnFail));

            // Continue to churn through work if there is work to do
            continue;
        }
        else
        {
            QueueLock.unlock();
        }

        // Wait for more work
        std::unique_lock<std::mutex> CVLock(CVMutex);
        WorkCV.wait(CVLock);

        // If we were notified and there is no work that means we are done
        {
            std::lock_guard<std::mutex> QueueLock(QueueMutex);
            if (WorkQueue.empty())
            {
                return;
            }
        }
    }
}

bool ShaderCompiler::CompileShaderFromFile(const ShaderDesc& ShaderDesc, bool ErrorOnFail)
{
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> IncludeHandler;
    DxcUtils->CreateDefaultIncludeHandler(&IncludeHandler);

    HANDLE ReadHandle = CreateFileW(ShaderDesc.FileName, GENERIC_READ, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    while (ReadHandle == INVALID_HANDLE_VALUE)
    {
        if (GetLastError() != ERROR_SHARING_VIOLATION)
        {
            RenderUtils::CreateDialogOnLastError();
            return false;
        }
        ReadHandle = CreateFileW(ShaderDesc.FileName, GENERIC_READ, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    };
    LARGE_INTEGER FileSize;
    if (!GetFileSizeEx(ReadHandle, &FileSize))
    {
        RenderUtils::CreateDialogOnLastError();
        return false;
    }
    char* FileData = new char[FileSize.QuadPart];
    DWORD BytesRead;
    if (!ReadFile(ReadHandle, FileData, FileSize.QuadPart, &BytesRead, NULL))
    {
        RenderUtils::CreateDialogOnLastError();
        delete[] FileData;
        return false;
    }
    CloseHandle(ReadHandle);
    Microsoft::WRL::ComPtr<IDxcBlobEncoding> SourceBlob;
    DxcUtils->CreateBlob(FileData, BytesRead, DXC_CP_ACP, &SourceBlob);
    DxcBuffer SourceBuffer = {SourceBlob->GetBufferPointer(), SourceBlob->GetBufferSize(), DXC_CP_ACP};

    LPCWSTR CompileArgs[] = {
        ShaderDesc.FileName,
        L"-E", ShaderDesc.EntryPoint,
        L"-T", ShaderDesc.Target,
        L"-Zs"};

    Microsoft::WRL::ComPtr<IDxcResult> CompileResult;
    DxcCompiler->Compile(&SourceBuffer, CompileArgs, _countof(CompileArgs), IncludeHandler.Get(), IID_PPV_ARGS(&CompileResult));

    Microsoft::WRL::ComPtr<IDxcBlobUtf8> ErrorBlob;
    CompileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&ErrorBlob), nullptr);

    if (ErrorBlob != NULL && ErrorBlob->GetBufferSize())
    {
        if (ErrorOnFail)
        {
            RenderUtils::CreateDialogOnError(ErrorBlob);
        }
        return false;
    }

    HRESULT HResult;
    CompileResult->GetStatus(&HResult);
    if (FAILED(HResult))
    {
        if (ErrorOnFail)
        {
            RenderUtils::CreateDialogIfFailed(HResult);
        }
        return false;
    }

    Microsoft::WRL::ComPtr<ID3DBlob> ShaderBlob;
    Microsoft::WRL::ComPtr<IDxcBlobWide> ShaderName;
    CompileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&ShaderBlob), ShaderName.GetAddressOf());

    std::lock_guard<std::recursive_mutex> Lock(LastKnownGoodMutex);
    LastKnownGoodShaders[ShaderDesc.FileName] = ShaderBlob;
    delete[] FileData;
    return true;
}