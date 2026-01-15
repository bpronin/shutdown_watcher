#include <windows.h>
#include <iostream>

void ExecuteCommand(const wchar_t* command)
{
    std::wcout << L"DEBUG - Received command: '" << command << "'" << std::endl;

    /* load action from settings */

    constexpr size_t BUFFER_SIZE = 256;
    wchar_t action[BUFFER_SIZE] = {L'\0'};
    GetPrivateProfileStringW(L"command", command, L"", action, BUFFER_SIZE, L".\\settings.ini");
    if (wcslen(action) == 0) return;

    /* run action process */

    std::wcout << L"DEBUG - Executing action: '" << action << "'" << std::endl;

    // _wsystem(action); sometimes it produces 0xc0000142 error

    STARTUPINFOW si = {sizeof(si)};
    PROCESS_INFORMATION pi = {};
    const BOOL bSuccess = CreateProcessW(
        nullptr,
        action,
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (bSuccess)
    {
        std::wcout << L"DEBUG - Process started successfully" << std::endl;

        if (const DWORD dwWaitResult = WaitForSingleObject(pi.hProcess, 5000); dwWaitResult == WAIT_TIMEOUT)
        {
            std::wcout << L"DEBUG - Command timed out before sleep" << std::endl;
        }
        else
        {
            DWORD dwExitCode;
            GetExitCodeProcess(pi.hProcess, &dwExitCode);

            std::wcout << L"DEBUG - Process finished with exit code " << dwExitCode << std::endl;
        }

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    else
    {
        std::wcout << L"DEBUG - CreateProcess failed. Error: " << GetLastError() << std::endl;
    }
}

HANDLE GetAppMutex()
{
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"ShutdownWatcher");

    if (hMutex && GetLastError() == ERROR_ALREADY_EXISTS)
    {
        MessageBoxW(nullptr, L"Application is already running", L"Shutdown Watch", MB_ICONWARNING);
        CloseHandle(hMutex);
        return nullptr;
    }

    return hMutex;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_POWERBROADCAST:
        if (wParam == PBT_APMSUSPEND)
            ExecuteCommand(L"on_suspend");
        else if (wParam == PBT_APMRESUMEAUTOMATIC) /* AUTOMATIC occurs always */
            ExecuteCommand(L"on_resume");
        return TRUE;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
}

HWND CreateAppWindow(HINSTANCE hInst)
{
    const auto CLASS_NAME = L"ShutdownWatcherHiddenClass";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME;
    RegisterClassW(&wc);

    return CreateWindowExW(
        0, CLASS_NAME, L"ShutdownWatcher", 0, 0, 0, 0, 0,
        HWND_MESSAGE, /* hidden message-only window */
        nullptr, nullptr, nullptr
    );
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
#ifdef _DEBUG
    AllocConsole();
    freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);
    freopen_s(reinterpret_cast<FILE**>(stderr), "CONOUT$", "w", stderr);
    freopen_s(reinterpret_cast<FILE**>(stdin), "CONIN$", "r", stdin);
#endif

    const auto hMutex = GetAppMutex();
    if (hMutex == nullptr) return 0;

    const auto hWnd = CreateAppWindow(hInst);
    const auto hNotify = RegisterSuspendResumeNotification(hWnd, DEVICE_NOTIFY_WINDOW_HANDLE);

    std::wcout << L"DEBUG - Running..." << std::endl;

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    UnregisterSuspendResumeNotification(hNotify);
    CloseHandle(hMutex);

#ifdef _DEBUG
    FreeConsole();
#endif

    return 0;
}
