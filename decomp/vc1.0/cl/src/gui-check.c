/* Exercise the generated GUI without changing its normal message loop. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static DWORD child_id;
static HWND child_window;

BOOL CALLBACK find_window(HWND window, LPARAM unused)
{
    DWORD process;
    char name[64];
    GetWindowThreadProcessId(window, &process);
    GetClassNameA(window, name, sizeof(name));
    if (process == child_id && IsWindowVisible(window) && !strcmp(name, "HelloWindow")) {
        child_window = window;
        return FALSE;
    }
    return TRUE;
}

static void put32(unsigned char *where, DWORD value)
{
    where[0] = (BYTE)value;
    where[1] = (BYTE)(value >> 8);
    where[2] = (BYTE)(value >> 16);
    where[3] = (BYTE)(value >> 24);
}

int main(int argc, char **argv)
{
    STARTUPINFOA startup;
    PROCESS_INFORMATION child;
    RECT client;
    HDC screen, memory;
    HBITMAP bitmap, old;
    BITMAPINFO info;
    unsigned char header[14], *pixels, *p;
    unsigned size, dark = 0;
    int width, height, i, x, y, min_x, min_y, max_x = 0, max_y = 0;
    DWORD status;
    char command[1024], title[64];
    FILE *file;
    if (argc != 4) return 2;
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    sprintf(command, "\"%s\"", argv[1]);
    if (!CreateProcessA(argv[1], command, NULL, NULL, TRUE, 0, NULL, NULL, &startup, &child)) return 3;
    child_id = child.dwProcessId;
    WaitForInputIdle(child.hProcess, 15000);
    for (i = 0; i < 150 && !child_window; ++i) {
        EnumWindows(find_window, 0);
        if (!child_window) Sleep(100);
    }
    if (!child_window) { TerminateProcess(child.hProcess, 4); return 4; }
    GetWindowTextA(child_window, title, sizeof(title));
    if (strcmp(title, "Hello, world!")) { TerminateProcess(child.hProcess, 5); return 5; }
    InvalidateRect(child_window, NULL, TRUE);
    UpdateWindow(child_window);
    GetClientRect(child_window, &client);
    width = client.right;
    height = client.bottom;
    screen = GetDC(child_window);
    memory = CreateCompatibleDC(screen);
    bitmap = CreateCompatibleBitmap(screen, width, height);
    old = SelectObject(memory, bitmap);
    if (!BitBlt(memory, 0, 0, width, height, screen, 0, 0, SRCCOPY)) return 6;
    SelectObject(memory, old);
    memset(&info, 0, sizeof(info));
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    size = width * height * 4;
    pixels = malloc(size);
    if (!pixels || GetDIBits(screen, bitmap, 0, height, pixels, &info, DIB_RGB_COLORS) != height) return 7;
    DeleteObject(bitmap);
    DeleteDC(memory);
    ReleaseDC(child_window, screen);
    min_x = width;
    min_y = height;
    for (y = 0; y < height; ++y) for (x = 0; x < width; ++x) {
        p = pixels + (y * width + x) * 4;
        if (p[0] < 128 && p[1] < 128 && p[2] < 128) {
            ++dark;
            if (x < min_x) min_x = x;
            if (x > max_x) max_x = x;
            if (y < min_y) min_y = y;
            if (y > max_y) max_y = y;
        }
    }
    memset(header, 0, sizeof(header));
    header[0] = 'B'; header[1] = 'M';
    put32(header + 2, sizeof(header) + sizeof(BITMAPINFOHEADER) + size);
    put32(header + 10, sizeof(header) + sizeof(BITMAPINFOHEADER));
    file = fopen(argv[2], "wb");
    if (!file) return 8;
    fwrite(header, 1, sizeof(header), file);
    fwrite(&info.bmiHeader, 1, sizeof(BITMAPINFOHEADER), file);
    fwrite(pixels, 1, size, file);
    fclose(file);
    free(pixels);
    PostMessageA(child_window, WM_CLOSE, 0, 0);
    if (WaitForSingleObject(child.hProcess, 15000) != WAIT_OBJECT_0) {
        TerminateProcess(child.hProcess, 9); return 9;
    }
    GetExitCodeProcess(child.hProcess, &status);
    CloseHandle(child.hThread);
    CloseHandle(child.hProcess);
    file = fopen(argv[3], "w");
    if (!file) return 10;
    fprintf(file, "{\"width\":%d,\"height\":%d,\"dark_pixels\":%u,\"bounds\":[%d,%d,%d,%d],\"exit_status\":%lu}\n",
            width, height, dark, min_x, min_y, max_x, max_y, status);
    fclose(file);
    if (status || dark < 20 || min_x > width / 2 || max_x < width / 2 ||
        min_y > height / 2 || max_y < height / 2 || max_x - min_x > 300 || max_y - min_y > 50) return 11;
    return 0;
}
