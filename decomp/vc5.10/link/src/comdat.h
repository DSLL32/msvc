#ifndef LINK_COMDAT_H
#define LINK_COMDAT_H
#include <windows.h>
#include "full-link.h"
#include <recon.h>

/* A reference holds either an original COFF symbol index or a contribution.
 * Incremental processing may replace its flags with a redirected target. */
typedef struct ComdatReference {
    DWORD symbol_index;
    Contribution *contribution;
    union { DWORD flags; BYTE flag_byte; Contribution *redirected; };
} ComdatReference;
typedef char ComdatReferenceSize[(sizeof(ComdatReference)==12)?1:-1];
#ifdef __cplusplus
extern "C" {
#endif
void *__stdcall add_comdat_reference(DWORD,Contribution *,Contribution *);
#ifdef __cplusplus
}
#endif
#endif
