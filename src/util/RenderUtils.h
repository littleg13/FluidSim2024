#include <Windows.h>
#include <d3d12.h>
#include <dxc/dxcapi.h>
#include <exception>
#include <wrl.h>

namespace RenderUtils
{
    /* Returns whether or not there was an error */
    inline bool CreateDialogIfFailed(HRESULT HResult)
    {
        if (FAILED(HResult))
        {
            LPTSTR ErrorText = NULL;
            FormatMessage(
                FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                HResult,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&ErrorText,
                0,
                NULL);

            MessageBox(NULL, (LPCTSTR)ErrorText, TEXT("Error"), MB_OK);
            return true;
        }
        return false;
    }

    /* Returns whether or not there was an error */
    inline bool CreateDialogIfFailed(HRESULT HResult, const Microsoft::WRL::ComPtr<ID3DBlob>& ErrorBlob)
    {
        if (FAILED(HResult))
        {
            LPTSTR ErrorText = NULL;
            FormatMessage(
                FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                HResult,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&ErrorText,
                0,
                NULL);
            LPTSTR MessageText = NULL;
            if (ErrorBlob != NULL)
            {
                if (size_t ErrorLen = ErrorBlob->GetBufferSize())
                {
                    DWORD_PTR Args[2] = {(DWORD_PTR)ErrorText, (DWORD_PTR)ErrorBlob->GetBufferPointer()};
                    FormatMessage(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        TEXT("Error: %1\nAdditional Information:\n%2"),
                        NULL, // Ignored
                        NULL, // Ignored
                        (LPTSTR)&MessageText, 0, (va_list*)Args);
                }
            }
            else
            {
                MessageText = ErrorText;
            }
            MessageBox(NULL, (LPCTSTR)MessageText, TEXT("Error"), MB_OK);
            return true;
        }
        return false;
    }

    inline bool CreateDialogOnError(const Microsoft::WRL::ComPtr<IDxcBlobUtf8>& ErrorBlob)
    {
        LPTSTR MessageText = NULL;
        if (ErrorBlob != NULL)
        {
            if (size_t ErrorLen = ErrorBlob->GetBufferSize())
            {
                DWORD_PTR Args[1] = {(DWORD_PTR)ErrorBlob->GetBufferPointer()};
                FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                    TEXT("Error:\n %1\n"),
                    NULL, // Ignored
                    NULL, // Ignored
                    (LPTSTR)&MessageText, 0, (va_list*)Args);
            }
            MessageBox(NULL, (LPCTSTR)MessageText, TEXT("Error"), MB_OK);
            return true;
        }
        return false;
    }

    /* Returns whether or not there was an error */
    inline bool CreateDialogAndThrowIfFailed(HRESULT HResult)
    {
        if (CreateDialogIfFailed(HResult))
        {
            // throw std::exception();
            return true;
        }
        return false;
    }

    /* Returns whether or not there was an error */
    inline bool CreateDialogOnLastError()
    {
        HRESULT HResult = GetLastError();
        return CreateDialogIfFailed(HResult);
    }
};