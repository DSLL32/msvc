/* Readable external callback workload, linked into native tests and compiled
 * as a DLL for explicit original-only capture. No reconstructed function here. */
#ifdef RECON_NATIVE_TEST
#define RECON_SHIM_IMPL
#endif
#include <stdlib.h>
#include <string.h>
#include "case.h"
#include <recon.h>
#ifdef RECON_ORIGINAL_DIRECT
#include <recon-original-direct.h>
static ReconOriginalProbe *original_context;
#define ORIGINAL(rva) RECON_ORIGINAL_ADDRESS(original_context,rva)
#endif
static ReferenceProbe probe;
static void record(unsigned phase,DemangleName *result,DemangleName *expected,
    const DemangleName *source,const DemangleName *saved)
{
    ReferenceRow *row=&probe.rows[phase];
    DemangleName *names=probe.names;
    DemangleReferenceNode *reference;
    unsigned i,j,guard=1;
    row->phase=phase;
    row->returned_owner=result==expected;
    row->source_unchanged=!source || !memcmp(source,saved,sizeof(*source));
    for (i=0;i<16;++i) {
        if (probe.before[i]!=0xa5 || probe.after[i]!=0xa5) guard=0;
        row->flags[i]=names[i].flags;
        row->head_kind[i]=names[i].head?names[i].head->kind:0xffffffffUL;
        row->reference_owner[i]=0xffffffffUL;
        if (names[i].head && names[i].head->kind==DEMANGLE_REFERENCE) {
            reference=(DemangleReferenceNode *)names[i].head;
            row->reference_owner[i]=0xfffffffeUL;
            for (j=0;j<16;++j)
                if (reference->name==&names[j]) row->reference_owner[i]=j;
        }
    }
    row->guards=guard;
    row->length=probe.functions.length(result,NULL);
    memset(row->output,0xa5,sizeof(row->output));
    if (row->length>=sizeof(row->output)-1 ||
        probe.functions.write(result,NULL,(char *)row->output,63)!=(char *)row->output)
        ++probe.failed;
    if (!row->returned_owner || !row->source_unchanged || !guard) ++probe.failed;
    ++probe.rows_used;
}
static void composition(void)
{
    struct { unsigned char before[16];DemangleName names[16];unsigned char after[16]; } owned;
    ReferenceFunctions *f=&probe.functions;
    DemangleName saved,*value,*n=owned.names;
    unsigned i;
    memset(&owned,0,sizeof(owned));
    memset(owned.before,0xa5,16);memset(owned.after,0xa5,16);
    probe.names=n;probe.before=owned.before;probe.after=owned.after;probe.active=1;
    /* First callback precedes parser-global setup. Only the genuinely
     * initialized allocator/list and these explicit caller owners are used. */
    for (i=0;i<16;++i) f->empty(&n[i],NULL);
    f->text(&n[0],NULL,(char *)probe.input+64);
    f->text(&n[1],NULL,(char *)probe.input+96);
    f->status(&n[6],NULL,1);f->status(&n[7],NULL,2);f->status(&n[8],NULL,3);
    probe.phase=0;saved=n[1];value=f->plus_reference(&n[0],NULL,&n[2],&n[1]);record(0,value,&n[2],&n[1],&saved);
    if (!n[2].head || n[2].head->kind!=DEMANGLE_REFERENCE || !n[2].head->next ||
        n[2].head->next->kind!=DEMANGLE_REFERENCE ||
        ((DemangleReferenceNode *)n[2].head->next)->name!=&n[1]) ++probe.failed;
    probe.phase=1;value=f->assign_text(&n[1],NULL,(char *)probe.input+128);record(1,&n[2],&n[2],NULL,NULL);
    if (value!=&n[1]) ++probe.failed;
    /* Version2 keeps the observed live-head rejection above, then uses the
     * genuine empty constructor before a valid source text assignment. */
    f->empty(&n[1],NULL);f->assign_text(&n[1],NULL,(char *)probe.input+128);
    probe.phase=2;saved=n[1];value=f->plus_reference(&n[3],NULL,&n[4],&n[1]);record(2,value,&n[4],&n[1],&saved);
    if (!n[4].head || n[4].head->kind!=DEMANGLE_REFERENCE ||
        ((DemangleReferenceNode *)n[4].head)->name!=&n[1]) ++probe.failed;
    probe.phase=3;saved=n[6];value=f->merge_error(&n[0],NULL,&n[6]);record(3,value,&n[0],&n[6],&saved);
    probe.phase=4;value=f->append_reference(&n[4],NULL,NULL);record(4,value,&n[4],NULL,NULL);
    probe.phase=5;value=f->assign_reference(&n[5],NULL,NULL);record(5,value,&n[5],NULL,NULL);
    probe.phase=6;saved=n[1];value=f->assign_reference(&n[8],NULL,&n[1]);record(6,value,&n[8],&n[1],&saved);
    f->text(&n[9],NULL,(char *)probe.input+64);
    probe.phase=7;saved=n[6];value=f->append_reference(&n[9],NULL,&n[6]);record(7,value,&n[9],&n[6],&saved);
    f->text(&n[10],NULL,(char *)probe.input+64);
    probe.phase=8;saved=n[7];value=f->append_reference(&n[10],NULL,&n[7]);record(8,value,&n[10],&n[7],&saved);
    probe.phase=9;saved=n[1];value=f->merge_error(&n[4],NULL,&n[1]);record(9,value,&n[4],&n[1],&saved);
    probe.phase=10;saved=n[6];value=f->merge_error(&n[8],NULL,&n[6]);record(10,value,&n[8],&n[6],&saved);
    probe.phase=11;value=f->plus_reference(&n[11],NULL,&n[12],NULL);record(11,value,&n[12],NULL,NULL);
    probe.active=0;
    /* Raw stack-owner pointers are no longer valid after return. The target's
     * remaining parser/cleanup has no edge to these local names or references. */
    probe.names=NULL;probe.before=probe.after=NULL;
}
PROBE_EXPORT void *__cdecl ReferenceAllocate(unsigned size)
{
    void *memory;
    unsigned index=probe.allocation_count;
    if (index==16) { ++probe.failed;return NULL; }
#ifdef RECON_ORIGINAL_DIRECT
    /* Version3 caller allocator: target CRT has completed genuine startup.
     * The normally loaded probe does not pretend to initialize its own CRT. */
    memory=((void *(__cdecl *)(unsigned))ORIGINAL(0x41160))(size);
#else
    memory=malloc(size);
#endif
    if (!memory) { ++probe.failed;return NULL; }
    probe.allocations[index]=memory;probe.sizes[index]=size;++probe.allocation_count;
    if (!probe.done) {
        probe.done=1;
        if (size!=512 || probe.functions.heap->head || probe.functions.heap->tail ||
            probe.functions.heap->remaining || probe.functions.heap->allocate!=ReferenceAllocate ||
            probe.functions.heap->release!=ReferenceRelease) ++probe.failed;
        else composition();
    }
    return memory;
}
PROBE_EXPORT void __cdecl ReferenceRelease(void *memory)
{
    unsigned i;
    for (i=0;i<probe.allocation_count;++i) if (probe.allocations[i]==memory) break;
    if (i==probe.allocation_count || probe.freed[i] || probe.active || probe.free_count==16) {
        ++probe.failed;return;
    }
    probe.freed[i]=1;probe.free_order[probe.free_count++]=i;
#ifdef RECON_ORIGINAL_DIRECT
    ((void (__cdecl *)(void *))ORIGINAL(0x40fa0))(memory);
#else
    free(memory);
#endif
}
PROBE_EXPORT ReferenceProbe *__cdecl GetReferenceProbe(void)
{
    memset(&probe,0,sizeof(probe));
    return &probe;
}
PROBE_EXPORT int __cdecl RunReferenceProbe(const ReferenceFunctions *functions,
    const unsigned char *input)
{
    char *result;
    unsigned i;
    probe.functions=*functions;memcpy(probe.input,input,256);
    memset(probe.public_output,0xa5,sizeof(probe.public_output));
    result=functions->symbol((char *)probe.public_output,(char *)probe.input,63,
        ReferenceAllocate,ReferenceRelease,0x800);
    if (result!=(char *)probe.public_output || !probe.done || probe.active ||
        probe.rows_used!=12 || probe.free_count!=probe.allocation_count ||
        functions->heap->head || functions->heap->tail) ++probe.failed;
    for (i=0;i<probe.allocation_count;++i) if (!probe.freed[i]) ++probe.failed;
    if (memcmp(probe.input,input,256)) ++probe.failed;
    return probe.failed?1:0;
}

#ifndef RECON_ORIGINAL_DIRECT
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#endif
/* The existing public demangler installs the real allocator callbacks before
 * the direct reference workload runs. No parser globals are supplied here. */
#ifndef RECON_ORIGINAL_DIRECT
ReferenceProbe *reference_test_probe;
int *reference_test_errno;
#endif
static int run_reference_workflow(const unsigned char *input)
{
    ReferenceFunctions f;
    unsigned report[964];
    unsigned header[8]={0x31524644,1,0,0,0,0,0,0};
    unsigned char *at=(unsigned char *)report;
    ReferenceProbe *state;
    int status;
#ifdef RECON_ORIGINAL_DIRECT
    f.symbol=(void *)ORIGINAL(0x2d8b0);f.empty=(void *)ORIGINAL(0x2fe60);
    f.text=(void *)ORIGINAL(0x2ff80);f.status=(void *)ORIGINAL(0x30150);
    f.assign_text=(void *)ORIGINAL(0x307e0);f.length=(void *)ORIGINAL(0x30200);
    f.write=(void *)ORIGINAL(0x30230);f.plus_reference=(void *)ORIGINAL(0x303f0);
    f.append_reference=(void *)ORIGINAL(0x30630);f.assign_reference=(void *)ORIGINAL(0x30860);
    f.merge_error=(void *)ORIGINAL(0x30770);f.heap=(void *)ORIGINAL(0x5ed80);
#else
    f.symbol=demangle_symbol;f.empty=demangle_name_empty;f.text=demangle_name_text;
    f.status=demangle_name_status;f.assign_text=demangle_name_assign_text;
    f.length=demangle_name_length;f.write=demangle_name_write;
    f.plus_reference=demangle_name_plus_reference;f.append_reference=demangle_name_append_reference;
    f.assign_reference=demangle_name_assign_reference;f.merge_error=demangle_name_merge_error;
    f.heap=&demangle_heap;
#endif
    state=GetReferenceProbe();
#ifndef RECON_ORIGINAL_DIRECT
    reference_test_probe=state;reference_test_errno=&errno;
#endif
    status=RunReferenceProbe(&f,input);
    header[2]=state->failed;header[3]=state->rows_used;
    header[4]=state->allocation_count;header[5]=state->free_count;
    header[6]=state->active;header[7]=state->done;
#define COPY(part,size) do{memcpy(at,(part),(size));at+=(size);}while(0)
    COPY(header,32);COPY(state->sizes,64);COPY(state->freed,64);
    COPY(state->free_order,64);COPY(state->input,256);COPY(state->public_output,64);
    COPY(state->rows,sizeof(state->rows));
#undef COPY
#ifdef RECON_ORIGINAL_DIRECT
    if (!recon_original_emit(original_context,".reference-results.bin",report,sizeof(report))) status=1;
#else
    { int fd=open("results.bin",O_WRONLY|O_CREAT|O_TRUNC,0600);
      if(fd<0||write(fd,report,sizeof(report))!=sizeof(report)||close(fd))status=1; }
#endif
    return status==0;
}
#ifdef RECON_ORIGINAL_DIRECT
__declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *context)
{
    unsigned char input[256];
    HANDLE file;
    DWORD count;
    unsigned char extra;
    original_context=context;
    if(!context||context->abi_version!=RECON_ORIGINAL_PROBE_VERSION||
       context->struct_size!=sizeof(*context)||context->marker_rva!=0x16b40)
        recon_original_finish(context,0);
    file=CreateFileA("strings.bin",GENERIC_READ,FILE_SHARE_READ,NULL,
        OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE||!ReadFile(file,input,256,&count,NULL)||count!=256||
       !ReadFile(file,&extra,1,&count,NULL)||count||!CloseHandle(file))
        recon_original_finish(context,0);
    recon_original_finish(context,run_reference_workflow(input));
}
#else
int run_reference_case(void)
{
    unsigned char input[256],extra;
    int fd=open("strings.bin",O_RDONLY);
    if(fd<0||read(fd,input,256)!=256||read(fd,&extra,1)!=0||close(fd))return 0;
    return run_reference_workflow(input);
}
#endif
