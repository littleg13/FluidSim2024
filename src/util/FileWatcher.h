#include <filesystem>
#include <functional>
#include <vector>
#include <windows.h>

namespace FileWatcher
{
    template <typename F>
    inline void WatchAnyFiles(std::vector<LPTSTR> FilePaths, F&& Callback, HANDLE KillEvent = INVALID_HANDLE_VALUE)
    {
        std::vector<HANDLE> WaitHandles;
        if (KillEvent != INVALID_HANDLE_VALUE)
        {
            WaitHandles.push_back(KillEvent);
        }
        for (auto& FilePath : FilePaths)
        {
            WaitHandles.push_back(FindFirstChangeNotification(FilePath, false, FILE_NOTIFY_CHANGE_LAST_WRITE));
            if (WaitHandles.back() == INVALID_HANDLE_VALUE)
            {
                RenderUtils::CreateDialogOnLastError();
            }
        }
        while (true)
        {
            DWORD WaitStatus = WaitForMultipleObjects(WaitHandles.size(), WaitHandles.data(), false, INFINITE);
            DWORD WaitObject = WaitStatus - WAIT_OBJECT_0;
            if (KillEvent != INVALID_HANDLE_VALUE && WaitObject == 0)
            {
                return;
            }
            else if (WaitObject >= 0 && WaitObject < WaitHandles.size())
            {
                bool FileInUse = false;
                if (!FileInUse)
                {
                    Callback();
                    if (!FindNextChangeNotification(WaitHandles[WaitObject]))
                    {
                        printf("\n ERROR: FileWatcher::WatchAnyFiles failed to reset wait handle.\n");
                        return;
                    }
                }
                else
                {
                    RenderUtils::CreateDialogOnLastError();
                }
            }
            else
            {
                printf("\n ERROR: FileWatcher::WatchAnyFiles timed out.\n");
                return;
            }
        }
    }
};