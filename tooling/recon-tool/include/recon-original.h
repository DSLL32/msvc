#ifndef RECON_ORIGINAL_H
#define RECON_ORIGINAL_H
#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif
    /* Explicit original-only direct probe. No fixed signature for target functions:
     * typed case glue supplies ECX/EDX and stack argument words deliberately. */
    typedef struct ReconOriginalRegisters
    {
        uint32_t eax, ebx, ecx, edx, esi, edi, ebp, esp, eip, eflags;
        uint8_t
            floating_save[112]; /* Win32 FLOATING_SAVE_AREA, explicit original bytes. */
    } ReconOriginalRegisters;
    typedef struct ReconOriginalCallResult
    {
        ReconOriginalRegisters entry, returned;
    } ReconOriginalCallResult;
    typedef struct ReconOriginal ReconOriginal;
    struct ReconOriginal
    {
        uint32_t abi_version, struct_size, image_base;
        void *opaque;
        int (*read)(ReconOriginal *, uint32_t address, void *bytes, uint32_t size);
        int (*write)(ReconOriginal *, uint32_t address, const void *bytes,
                     uint32_t size);
        uint32_t (*allocate)(ReconOriginal *, uint32_t size);
        int (*release)(ReconOriginal *, uint32_t address);
        int (*until)(ReconOriginal *, uint32_t entry_rva, ReconOriginalRegisters *);
        int (*call)(ReconOriginal *, uint32_t entry_rva, uint32_t ecx, uint32_t edx,
                    const uint32_t *stack_words, uint32_t count,
                    ReconOriginalCallResult *);
        /* Save a narrow result as a fixture input file for native assertions.
         * Names are relative paths; colliding with an existing input is rejected. */
        int (*emit)(ReconOriginal *, const char *name, const void *bytes,
                    uint32_t size);
    };
    /* Optional export from direct.c. Loaded only by explicit original capture. */
    int case_original_direct(ReconOriginal *);
#ifdef __cplusplus
}
#endif
#endif
