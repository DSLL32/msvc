#ifndef RECON_ORIGINAL_DIRECT_H
#define RECON_ORIGINAL_DIRECT_H
/* Original-only typed probe DLL. This header is compatible with the original
 * Win32 compiler; native direct glue uses recon-native.h instead. */
#include <windows.h>
#ifdef __cplusplus
extern "C"
{
#endif
#define RECON_ORIGINAL_PROBE_VERSION 1UL
#define RECON_ORIGINAL_PROBE_COMPLETE 0x31504452UL

    typedef struct ReconOriginalProbe
    {
        DWORD abi_version, struct_size;
        DWORD image_base, marker_rva;
        DWORD original_esp, original_ecx, original_edx;
        BYTE *result_buffer;
        DWORD result_capacity, result_size, result_count;
        volatile DWORD completed, passed;
    } ReconOriginalProbe;

/* Resolve genuine original storage/functions; do not manufacture owner state. */
#define RECON_ORIGINAL_ADDRESS(context, rva)                                           \
    ((void *)((context)->image_base + (DWORD)(rva)))

    /* Save a narrow independently observed result as capture input for native
     * assertions. Names must be relative; existing fixture inputs cannot collide. */
    int __stdcall recon_original_emit(ReconOriginalProbe *, const char *, const void *,
                                      DWORD);
    /* Publish assertions then enter the broker-owned helper completion breakpoint.
     * Never returns or performs application CRT cleanup. */
    void __stdcall recon_original_finish(ReconOriginalProbe *, int);
/* Case-owned export; compiler-generated calls preserve actual target ABIs. */
#ifndef RECON_ORIGINAL_BROKER
    __declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *);
#endif
#ifdef __cplusplus
}
#endif
#endif
