/* Authored Win32 debugger fixture, NOT reconstructed Watcom application code.
   Narrow naked assembly makes the tested PUSH-register prologues and RET12 ABI
   explicit. Reader increments a marker so rejected pre-entry execution can be
   checked against unguarded disposable runs of this same authored program. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <recon.h>

__declspec(dllexport) volatile DWORD fixture_marker;
__declspec(dllexport) int fixture_mode;

__declspec(dllexport) __declspec(naked) void fixture_reader(void)
{
    __asm {
        push ecx
        inc dword ptr fixture_marker
        pop ecx
        xor eax,eax
        ret 12
    }
}

__declspec(dllexport) __declspec(naked) void fixture_owner(void)
{
    __asm {
        push ebx
        xor eax,eax
        mov ax,ds
        cmp fixture_mode,1
        jne selector_ready
        xor eax,1
selector_ready:
        mov ebx,128
        cmp fixture_mode,3
        jne capacity_ready
        mov ebx,64
capacity_ready:
        push ebx
        push eax
        push edx
        call fixture_reader
        pop ebx
        ret
    }
}

__declspec(dllexport) __declspec(naked) void fixture_scope(void)
{
    __asm {
        sub esp,128
        mov edx,esp
        cmp fixture_mode,2
        jne pointer_ready
        add edx,4
pointer_ready:
        call fixture_owner
        mov edx,esp
        call fixture_owner
        add esp,128
        ret
    }
}

__declspec(dllexport) __declspec(naked) void fixture_unexpected(void)
{
    __asm {
        int 3
        ret
    }
}

__declspec(dllexport) __declspec(naked) void __stdcall fixture_final_exit(DWORD status)
{
    __asm jmp dword ptr [ExitProcess]
}

static DWORD WINAPI fixture_thread(void *unused)
{
    fixture_scope();
    return 0;
}

static int fixture_exception(EXCEPTION_POINTERS *info)
{
    /* Only the fixture's explicit named INT3 is handled. Other exceptions are
       never swallowed to make a rejected test appear successful. */
    return info->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT &&
           info->ExceptionRecord->ExceptionAddress == (void *)fixture_unexpected ?
           EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
}

int main(int argc, char **argv)
{
    FILE *out;
    HANDLE thread;
    if (argc != 3) return 120;
    if (!strcmp(argv[1], "selector")) fixture_mode = 1;
    else if (!strcmp(argv[1], "pointer")) fixture_mode = 2;
    else if (!strcmp(argv[1], "capacity")) fixture_mode = 3;
    else if (!strcmp(argv[1], "thread")) fixture_mode = 4;
    else if (!strcmp(argv[1], "unexpected")) fixture_mode = 5;
    else if (strcmp(argv[1], "positive")) return 121;
    if (fixture_mode == 4) {
        thread = CreateThread(NULL, 0, fixture_thread, NULL, 0, NULL);
        if (!thread || WaitForSingleObject(thread, 5000) != WAIT_OBJECT_0) return 122;
        CloseHandle(thread);
    } else if (fixture_mode == 5) {
        __try { fixture_unexpected(); }
        __except (fixture_exception(GetExceptionInformation())) { fixture_scope(); }
    } else fixture_scope();
    out = fopen(argv[2], "wb");
    if (!out || fwrite((void *)&fixture_marker, 4, 1, out) != 1 || fclose(out)) return 123;
    fixture_final_exit(0);
    return 124;
}
