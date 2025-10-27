#include <windows.h>
#include <iostream>

void ExecuteCommand(const wchar_t* command)
{
    constexpr size_t BUFFER_SIZE = 256;
    wchar_t buffer[BUFFER_SIZE] = {L'\0'};
    GetPrivateProfileStringW(L"command", command, nullptr, buffer, BUFFER_SIZE, L".\\settings.ini");
    const std::wstring action(buffer);

    std::wcout << L"DEBUG: Executing command: '"<< command<< "'. " << "Action: '" << action << "'" << std::endl;

    if (!action.empty()) _wsystem(action.c_str());
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_POWERBROADCAST:
        if (wParam == PBT_APMSUSPEND)
        {
            std::wcout << L"DEBUG: System is suspending" << std::endl;

            ExecuteCommand(L"on_suspend");
        }
        else if (wParam == PBT_APMRESUMESUSPEND)
        {
            std::wcout << L"DEBUG: System resumed from suspend" << std::endl;

            ExecuteCommand(L"on_resume");
        }
        return TRUE;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
#ifdef _DEBUG
    AllocConsole();
    freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);
    freopen_s(reinterpret_cast<FILE**>(stderr), "CONOUT$", "w", stderr);
    freopen_s(reinterpret_cast<FILE**>(stdin), "CONIN$", "r", stdin);
#endif

    const auto CLASS_NAME = "ShutdownWatcherHiddenClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    HWND hWnd = CreateWindowEx(
        0, CLASS_NAME, "ShutdownWatcher", 0, 0, 0, 0, 0,
        HWND_MESSAGE, /* hidden message-only window */
        nullptr, nullptr, nullptr
    );
    if (!hWnd) return 1;

    const auto hNotify = RegisterSuspendResumeNotification(hWnd, DEVICE_NOTIFY_WINDOW_HANDLE);

    std::wcout << L"DEBUG: Running..." << std::endl;

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnregisterSuspendResumeNotification(hNotify);

#ifdef _DEBUG
    FreeConsole();
#endif

    return 0;
}
