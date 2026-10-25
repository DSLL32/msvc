#ifndef DEMANGLE_REFERENCE_PROBE_H
#define DEMANGLE_REFERENCE_PROBE_H
/* External test dependency; these are not LINK application vertices. */
#include "../../src/demangle.h"
#include <recon.h>
#ifdef RECON_NATIVE_TEST
#define PROBE_EXPORT
#else
#define PROBE_EXPORT __declspec(dllexport)
#endif
typedef struct ReferenceFunctions {
    char *(__cdecl *symbol)(char *,const char *,int,DemangleAllocate,DemangleRelease,unsigned short);
    DemangleName *(__fastcall *empty)(DemangleName *,void *);
    DemangleName *(__fastcall *text)(DemangleName *,void *,const char *);
    DemangleName *(__fastcall *status)(DemangleName *,void *,int);
    DemangleName *(__fastcall *assign_text)(DemangleName *,void *,const char *);
    int (__fastcall *length)(DemangleName *,void *);
    char *(__fastcall *write)(DemangleName *,void *,char *,int);
    DemangleName *(__fastcall *plus_reference)(const DemangleName *,void *,DemangleName *,DemangleName *);
    DemangleName *(__fastcall *append_reference)(DemangleName *,void *,DemangleName *);
    DemangleName *(__fastcall *assign_reference)(DemangleName *,void *,DemangleName *);
    DemangleName *(__fastcall *merge_error)(DemangleName *,void *,const DemangleName *);
    DemangleHeap *heap;
} ReferenceFunctions;
typedef struct ReferenceRow {
    unsigned phase,returned_owner,source_unchanged,guards,length;
    unsigned flags[16],head_kind[16],reference_owner[16];
    unsigned char output[64];
} ReferenceRow;
typedef struct ReferenceProbe {
    ReferenceFunctions functions;
    unsigned failed,active,phase,done,rows_used;
    DemangleName *names;
    unsigned char *before,*after;
    unsigned allocation_count,free_count;
    void *allocations[16];
    unsigned sizes[16],freed[16],free_order[16];
    unsigned char input[256],public_output[64];
    ReferenceRow rows[12];
} ReferenceProbe;
typedef char reference_table_size[(sizeof(ReferenceFunctions)==48)?1:-1];
typedef char reference_row_size[(sizeof(ReferenceRow)==276)?1:-1];
typedef char reference_probe_size[(sizeof(ReferenceProbe)==3976)?1:-1];
PROBE_EXPORT ReferenceProbe *__cdecl GetReferenceProbe(void);
PROBE_EXPORT int __cdecl RunReferenceProbe(const ReferenceFunctions *,const unsigned char *);
PROBE_EXPORT void *__cdecl ReferenceAllocate(unsigned);
PROBE_EXPORT void __cdecl ReferenceRelease(void *);
#endif
