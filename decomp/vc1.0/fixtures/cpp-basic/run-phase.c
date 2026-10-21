/* Reproduce CL's explicit compiler environment or inherited linker environment. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
int main(int argc, char **argv)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    DWORD result, length;
    char environment[4096], command[1024];
    void *environment_block = NULL;
    if (argc < 2 || argc > 4) return 120;
    strcpy(environment, "MSC_CMD_FLAGS=");
    length = GetEnvironmentVariableA("MSC_CMD_FLAGS", environment + 14, sizeof(environment) - 15);
    if (length >= sizeof(environment) - 15) return 121;
    if (length) {
        environment[14 + length + 1] = 0;
        environment_block = environment;
    }
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    strcpy(command, argv[1]);
    if (argc >= 3 && *argv[2]) { strcat(command, " "); strcat(command, argv[2]); }
    if (argc == 4) {
        if (strlen(argv[3]) >= sizeof(command)) return 124;
        strcpy(command, argv[3]);
    }
    if (!CreateProcessA(argv[1], command, NULL, NULL, TRUE, 0,
                        environment_block, NULL, &si, &pi)) return 122;
    WaitForSingleObject(pi.hProcess, INFINITE);
    if (!GetExitCodeProcess(pi.hProcess, &result)) return 123;
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return result;
}
