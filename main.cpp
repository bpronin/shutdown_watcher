#include <windows.h>
#include <iostream>

std::wstring suspend_command;
std::wstring resume_command;

void OnApmSuspend()
{
    std::wcout << L"System is suspending. Executing: " << suspend_command << std::endl;
    if (!suspend_command.empty())
        _wsystem(suspend_command.c_str());
}

void OnApmResume()
{
    std::wcout << L"System resumed from suspend. Executing: " << resume_command << std::endl;
    if (!resume_command.empty())
        _wsystem(resume_command.c_str());
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_POWERBROADCAST:
        if (wParam == PBT_APMSUSPEND)
            OnApmSuspend();
        else if (wParam == PBT_APMRESUMESUSPEND)
            OnApmResume();
        return TRUE;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}

void ReadSettings()
{
    const auto settings_ini = L".\\settings.ini";

    wchar_t buffer[256];

    GetPrivateProfileStringW(L"command", L"on_system_suspend", nullptr, buffer, 256, settings_ini);
    suspend_command = std::wstring(buffer);

    GetPrivateProfileStringW(L"command", L"on_system_resume", nullptr, buffer, 256, settings_ini);
    resume_command = std::wstring(buffer);
}

void RunMain(HINSTANCE hInst)
{
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

    if (!hWnd)
    {
        MessageBox(nullptr, "Error starting application", "Shutdown Watcher", MB_ICONERROR);
        return;
    }

    std::wcout << L"Running..." << std::endl;

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
    ReadSettings();
    RunMain(hInst);

    return 0;
}
