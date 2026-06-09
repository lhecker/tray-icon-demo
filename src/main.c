#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <shellapi.h>

#include <stdbool.h>

#include "resource.h"

enum UserMessages {
    WM_NOTIFICATION_ICON_CALLBACK = WM_USER,
};

static HMENU s_menu;
static NOTIFYICONDATAW s_notification_data = {
    .cbSize = sizeof(NOTIFYICONDATAW),
    .uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP,
    .uCallbackMessage = WM_NOTIFICATION_ICON_CALLBACK,
    .szTip = L"Tray Icon Demo",
    .uVersion = NOTIFYICON_VERSION_4,
};
static UINT s_wm_taskbar_created;

static LRESULT CALLBACK window_callback(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message) {
    case WM_NOTIFICATION_ICON_CALLBACK:
        switch (LOWORD(lparam)) {
        case WM_CONTEXTMENU: {
            const bool right_align = GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0;
            UINT flags = TPM_RIGHTBUTTON;
            flags |= right_align ? TPM_RIGHTALIGN : TPM_LEFTALIGN;
            SetForegroundWindow(hwnd);
            TrackPopupMenuEx(s_menu, flags, LOWORD(wparam), HIWORD(wparam), hwnd, NULL);
            PostMessageW(hwnd, WM_NULL, 0, 0);
            break;
        }
        case WM_LBUTTONDOWN:
            // Main action (open window).
            break;
        case WM_LBUTTONDBLCLK:
            MessageBoxW(hwnd, L"Double click", L"Tray icon clicked!", MB_OK);
            break;
        default:
            break;
        }
        return 0;
    case WM_COMMAND: {
        switch (LOWORD(wparam)) {
        case ID_CONTEXTMENU_FORCEDARKMODE:
            MessageBoxW(hwnd, L"Force dark mode", L"Menu item selected!", MB_OK);
            break;
        case ID_CONTEXTMENU_FORCELIGHTMODE:
            MessageBoxW(hwnd, L"Force light mode", L"Menu item selected!", MB_OK);
            break;
        case ID_CONTEXTMENU_SETTINGS:
            MessageBoxW(hwnd, L"Settings", L"Menu item selected!", MB_OK);
            return 0;
        case ID_CONTEXTMENU_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        default:
            return 0;
        }
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        if (message == s_wm_taskbar_created) {
            Shell_NotifyIconW(NIM_ADD, &s_notification_data);
            Shell_NotifyIconW(NIM_SETVERSION, &s_notification_data);
            return 0;
        }
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }
}

static void menu_init(HINSTANCE instance)
{
    // If you need an actual window:
    // * Pass WS_OVERLAPPEDWINDOW, etc., as the 4th param to CreateWindowExW
    // * Replace HWND_MESSAGE with null

    const WNDCLASSEXW wcex = {
        .cbSize = sizeof(WNDCLASSEX),
        .lpfnWndProc = window_callback,
        .cbWndExtra = sizeof(void*),
        .hInstance = instance,
        .hIcon = LoadIconW(instance, MAKEINTRESOURCE(IDI_ICON1)),
        .hCursor = LoadCursorW(instance, IDC_ARROW),
        .lpszClassName = L"Tray Icon Demo",
    };
    RegisterClassExW(&wcex);

    const HWND hwnd = CreateWindowExW(
        0,
        L"Tray Icon Demo",
        L"Tray Icon Demo",
        0,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        HWND_MESSAGE,
        NULL,
        instance,
        NULL
    );

    s_menu = GetSubMenu(LoadMenuW(NULL, MAKEINTRESOURCEW(IDR_MENU1)), 0);
    s_wm_taskbar_created = RegisterWindowMessageW(L"TaskbarCreated");

    s_notification_data.hWnd = hwnd;
    s_notification_data.hIcon = wcex.hIcon;
    Shell_NotifyIconW(NIM_ADD, &s_notification_data);
    Shell_NotifyIconW(NIM_SETVERSION, &s_notification_data);
}

static void menu_deinit()
{
    Shell_NotifyIconW(NIM_DELETE, &s_notification_data);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev_instance, PWSTR cmd_line, int cmd_show)
{
    // Make sure the context menu supports dark mode (Windows 10, 1903 or later).
    // An official API does not exist and there's no alternative. Thanks, shell team.
    // Leaked here: https://github.com/ysc3839/win32-darkmode/blob/master/win32-darkmode/DarkMode.h
    const HANDLE uxtheme = LoadLibraryExW(L"uxtheme.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    bool(WINAPI* const SetPreferredAppMode)(int) = (void*)GetProcAddress(uxtheme, MAKEINTRESOURCEA(135));
    SetPreferredAppMode(1); // PreferredAppMode::AllowDark

    menu_init(instance);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    menu_deinit();
    return 0;
}
