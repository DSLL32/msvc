#ifndef LINK_COMDAT_H
#define LINK_COMDAT_H
#include <windows.h>
#include "full-link.h"
#include <recon.h>

/* A reference holds either an original COFF symbol index or a contribution.
 * Incremental processing may replace its flags with a redirected target. */
typedef struct ComdatReference {
    Contribution *contribution;
    union { DWORD symbol_index; StringNode *symbol; };
    union { DWORD flags; BYTE flag_byte; Contribution *redirected; };
} ComdatReference;
typedef char ComdatReferenceSize[(sizeof(ComdatReference)==12)?1:-1];
typedef char ComdatReferenceFields[(offsetof(ComdatReference,contribution)==0 &&
    offsetof(ComdatReference,symbol_index)==4 && offsetof(ComdatReference,flags)==8)?1:-1];
typedef struct ModuleContributionEnumerator {
    BYTE (__stdcall *next)(struct ModuleContributionEnumerator *);
    void (__stdcall *end)(void *);
    OutputModule *module;
    Contribution *current;
    DWORD index;
} ModuleContributionEnumerator;
typedef struct ComdatReferenceEnumerator {
    BYTE (__stdcall *next)(struct ComdatReferenceEnumerator *);
    void (__stdcall *end)(void *);
    WORD index,reserved0a;
    ComdatReference *current;
    ContributionInfo *block;
} ComdatReferenceEnumerator;
typedef char ContributionEnumeratorSize[(sizeof(ModuleContributionEnumerator)==20)?1:-1];
typedef char ReferenceEnumeratorSize[(sizeof(ComdatReferenceEnumerator)==20)?1:-1];
#ifdef __cplusplus
extern "C" {
#endif
void *__stdcall add_comdat_reference(DWORD,Contribution *,Contribution *);
void __stdcall initialize_module_contribution_enumerator(ModuleContributionEnumerator *,OutputModule *);
BYTE __stdcall next_module_contribution(ModuleContributionEnumerator *);
void __stdcall initialize_comdat_reference_enumerator(ComdatReferenceEnumerator *,ContributionInfo *);
BYTE __stdcall next_comdat_reference(ComdatReferenceEnumerator *);
#ifdef __cplusplus
}
#endif
#endif
