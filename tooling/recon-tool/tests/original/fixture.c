/* Authored Win32 fixture, independent of any reconstructed application. */
#include <windows.h>

int __cdecl probe_sum(int *values, int count)
{
    int index, total = 0;
    for (index = 0; index < count; ++index)
        total += values[index];
    values[0] = total;
    return total;
}

void WINAPI start(void)
{
    static const char message[] = "sum=7\n";
    static const BYTE bytes[] = {0, 255};
    int values[2];
    HANDLE file;
    DWORD count;
    values[0] = 2;
    values[1] = 5;
    if (probe_sum(values, 2) != 7 || values[0] != 7)
        ExitProcess(9);
    file = CreateFileA("out.bin", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (file == INVALID_HANDLE_VALUE ||
        !WriteFile(file, bytes, sizeof(bytes), &count, NULL) ||
        count != sizeof(bytes) || !CloseHandle(file))
        ExitProcess(8);
    if (!WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), message, sizeof(message) - 1,
                   &count, NULL))
        ExitProcess(7);
    ExitProcess(0);
}
