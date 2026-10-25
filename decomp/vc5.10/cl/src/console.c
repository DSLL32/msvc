/* Console control registration, 004057b4..004057c2. */
#include "cl.h"
#include <recon.h>

BOOL install_control_handler(void)
{
    return SetConsoleCtrlHandler(console_control_handler, TRUE);
}

/* 00405816..00405871: visible console height, with a redirected fallback. */
int console_rows(void)
{
    HANDLE console;
    CONSOLE_SCREEN_BUFFER_INFO screen;
    BOOL found;
    int rows;
    console = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE, 0,
                          NULL, OPEN_EXISTING, 0, NULL);
    if (console == INVALID_HANDLE_VALUE)
        return 25;
    found = GetConsoleScreenBufferInfo(console, &screen);
    CloseHandle(console);
    if (!found)
        return 25;
    rows = screen.srWindow.Bottom - screen.srWindow.Top + 1;
    return rows > 0 ? rows : 1;
}

/* 00405800..00405816: preprocessor output and diagnostics share stdout. */
void prepare_execution(void)
{
    if (keep_preprocessor_output)
        _dup2(1, 2);
}
