/* Shared explicit-original typed helper support, never native fixture code. */
#include "recon-original-direct.h"
#include <windows.h>

/* Patched before launch, then populated only by this normally loaded DLL. */
__declspec(dllexport) ReconOriginalProbe recon_original_probe_state = {
    RECON_ORIGINAL_PROBE_VERSION, sizeof(ReconOriginalProbe)};
typedef LONG(WINAPI *ProbeHandler)(EXCEPTION_POINTERS *);
typedef LPVOID(WINAPI *AddHandler)(ULONG, ProbeHandler);
static int entered;

/* The first instruction is the receipt breakpoint; no target cleanup runs. */
__declspec(dllexport)
__declspec(naked) void __stdcall recon_original_completion_trap(void)
{
    __asm { int 3 }
    __asm
    {
    again:
        jmp again
    }
}

static LONG WINAPI probe_exception(EXCEPTION_POINTERS *exception)
{
    ReconOriginalProbe *probe = &recon_original_probe_state;
    DWORD saved_error = GetLastError();
    if (exception->ExceptionRecord->ExceptionCode != EXCEPTION_BREAKPOINT ||
        (DWORD)exception->ExceptionRecord->ExceptionAddress !=
            probe->image_base + probe->marker_rva ||
        entered)
    {
        SetLastError(saved_error);
        return EXCEPTION_CONTINUE_SEARCH;
    }
    entered = 1;
    probe->original_esp = exception->ContextRecord->Esp;
    probe->original_ecx = exception->ContextRecord->Ecx;
    probe->original_edx = exception->ContextRecord->Edx;
    SetLastError(saved_error);
    case_original_at_marker(probe);
    recon_original_finish(probe, 0);
    return EXCEPTION_CONTINUE_SEARCH;
}

BOOL WINAPI DllMain(HANDLE module, DWORD reason, LPVOID reserved)
{
    ReconOriginalProbe *probe = &recon_original_probe_state;
    DWORD saved_error = GetLastError(), pe, size;
    BYTE *image;
    AddHandler add_handler;
    (void)module;
    (void)reserved;
    if (reason != DLL_PROCESS_ATTACH)
        return TRUE;
    image = (BYTE *)GetModuleHandleA(NULL);
    if (!image || probe->abi_version != RECON_ORIGINAL_PROBE_VERSION ||
        probe->struct_size != sizeof(*probe) || !probe->marker_rva)
        return FALSE;
    pe = *(DWORD *)(image + 0x3c);
    size = *(DWORD *)(image + pe + 24 + 56);
    if (probe->marker_rva >= size || image[probe->marker_rva] != 0xcc)
        return FALSE;
    probe->image_base = (DWORD)image;
    probe->result_capacity = 8 * 1024 * 1024;
    probe->result_buffer = VirtualAlloc(NULL, probe->result_capacity,
                                        MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    add_handler = (AddHandler)GetProcAddress(GetModuleHandleA("kernel32.dll"),
                                             "AddVectoredExceptionHandler");
    if (!probe->result_buffer || !add_handler || !add_handler(1, probe_exception))
        return FALSE;
    SetLastError(saved_error);
    return TRUE;
}

int __stdcall recon_original_emit(ReconOriginalProbe *context, const char *name,
                                  const void *bytes, DWORD size)
{
    DWORD length = 0, total, i;
    BYTE *destination;
    if (!context || context->abi_version != RECON_ORIGINAL_PROBE_VERSION ||
        context->struct_size != sizeof(*context) || !name || (!bytes && size))
        return 0;
    while (name[length])
    {
        if (length >= 4096 || (BYTE)name[length] < 32 || name[length] == ':' ||
            name[length] == '\\')
            return 0;
        ++length;
    }
    if (!length || name[0] == '/')
        return 0;
    for (i = 0; i < length; ++i)
        if (name[i] == '.' && (i == 0 || name[i - 1] == '/') &&
            (i + 1 == length || name[i + 1] == '/' ||
             (name[i + 1] == '.' && (i + 2 == length || name[i + 2] == '/'))))
            return 0;
    total = 8 + length;
    if (size > 0xffffffffUL - total)
        return 0;
    total += size;
    if (context->result_size > context->result_capacity ||
        total > context->result_capacity - context->result_size)
        return 0;
    destination = context->result_buffer + context->result_size;
    CopyMemory(destination, &length, 4);
    CopyMemory(destination + 4, &size, 4);
    CopyMemory(destination + 8, name, length);
    if (size)
        CopyMemory(destination + 8 + length, bytes, size);
    context->result_size += total;
    ++context->result_count;
    return 1;
}

void __stdcall recon_original_finish(ReconOriginalProbe *context, int passed)
{
    if (context && context->abi_version == RECON_ORIGINAL_PROBE_VERSION &&
        context->struct_size == sizeof(*context))
    {
        context->passed = passed != 0;
        context->completed = RECON_ORIGINAL_PROBE_COMPLETE;
    }
    recon_original_completion_trap();
    for (;;)
        DebugBreak();
}
