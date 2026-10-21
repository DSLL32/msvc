/* Launch a phase with precisely CL's single-entry environment block. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
int main(int argc, char **argv)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    DWORD result, length;
    char environment[4096], command[1024];
    if (argc != 2) return 120;
    strcpy(environment, "MSC_CMD_FLAGS=");
    length = GetEnvironmentVariableA("MSC_CMD_FLAGS", environment + 14, sizeof(environment) - 15);
    if (!length || length >= sizeof(environment) - 15) return 121;
    environment[14 + length + 1] = 0;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    strcpy(command, argv[1]);
    if (!CreateProcessA(argv[1], command, NULL, NULL, TRUE, 0,
                        environment, NULL, &si, &pi)) return 122;
    WaitForSingleObject(pi.hProcess, INFINITE);
    if (!GetExitCodeProcess(pi.hProcess, &result)) return 123;
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return result;
}
