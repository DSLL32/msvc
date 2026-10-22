#include <windows.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

LRESULT CALLBACK HelloWindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    PAINTSTRUCT paint;
    RECT client;
    HDC dc;
    switch (message) {
    case WM_PAINT:
        dc = BeginPaint(window, &paint);
        GetClientRect(window, &client);
        SetBkMode(dc, TRANSPARENT);
        DrawText(dc, "Hello, world!", -1, &client,
                 DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        EndPaint(window, &paint);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(window, message, wparam, lparam);
}

__declspec(dllexport) int __stdcall HelloMain(HINSTANCE instance, HINSTANCE previous, LPSTR command_line, int show)
{
    WNDCLASS cls = { 0 };
    HWND window;
    MSG message;
    int status;
    cls.lpfnWndProc = HelloWindowProc;
    cls.hInstance = instance;
    cls.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    cls.hCursor = LoadCursor(NULL, IDC_ARROW);
    cls.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    cls.lpszClassName = "HelloWindow";
    if (!RegisterClass(&cls)) return 1;
    window = CreateWindow("HelloWindow", "Hello, world!", WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT, CW_USEDEFAULT, 640, 360,
                          NULL, NULL, instance, NULL);
    if (!window) return 1;
    ShowWindow(window, show);
    UpdateWindow(window);
    while ((status = GetMessage(&message, NULL, 0, 0)) > 0) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
    return status == -1 ? 1 : (int)message.wParam;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    return TRUE;
}
