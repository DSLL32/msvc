#include <windows.h>
static int state = 1;
static char *copy(char *to, const char *from)
{
    while (*from) *to++ = *from++;
    return to;
}
static int equal(const char *a, const char *b)
{
    while (*a && *a == *b) { ++a; ++b; }
    return *a == *b;
}
static void emit(const char *text)
{
    DWORD size = 0, done;
    while (text[size]) ++size;
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), text, size, &done, NULL);
}
int control_common(int value) { return value + 1; }
int control_pending(void) { return 8; }
void control_child(const char *mode)
{
    if (state != 1 || control_common(4) != 5) ExitProcess(31);
    emit("child\n");
    if (equal(mode, "frontier")) control_pending();
    ExitProcess(7);
}
void control_parent(const char *mode)
{
    char path[MAX_PATH], line[MAX_PATH + 64], *end;
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    DWORD i, status;
    if (state != 1 || control_common(2) != 3) ExitProcess(32);
    state = 9;
    emit("parent\n");
    GetModuleFileNameA(NULL, path, sizeof(path));
    if (equal(mode, "wrong")) copy(path, "wrong.exe")[0] = 0;
    end = line; *end++ = '"'; end = copy(end, path); *end++ = '"';
    end = copy(end, " child "); end = copy(end, mode); *end = 0;
    for (i = 0; i < sizeof(startup); ++i) ((BYTE *)&startup)[i] = 0;
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    if (!CreateProcessA(path, line, NULL, NULL, TRUE, 0, NULL, NULL, &startup, &process)) ExitProcess(33);
    WaitForSingleObject(process.hProcess, INFINITE);
    if (!GetExitCodeProcess(process.hProcess, &status)) ExitProcess(34);
    CloseHandle(process.hThread); CloseHandle(process.hProcess);
    if (status != 7 || state != 9) ExitProcess(35);
    emit("done\n");
    ExitProcess(0);
}
void WINAPI start(void)
{
    char *tail = GetCommandLineA();
    if (*tail == '"') { ++tail; while (*tail && *tail != '"') ++tail; if (*tail) ++tail; }
    else while (*tail && *tail != ' ') ++tail;
    while (*tail == ' ') ++tail;
    if (tail[0] == 'c' && tail[1] == 'h' && tail[2] == 'i' && tail[3] == 'l' && tail[4] == 'd' && tail[5] == ' ')
        control_child(tail + 6);
    else control_parent(tail);
}
