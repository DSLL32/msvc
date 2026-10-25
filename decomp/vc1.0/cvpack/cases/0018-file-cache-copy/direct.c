/* Internal file-API coverage. Genuine CLI initialization owns all pools/slots;
   no parser continuation follows this exact-ABI entry hook. */
#ifndef RECON_ORIGINAL_DIRECT
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#endif
#include <string.h>
#include "files.h"
#include "image.h"
#ifdef RECON_ORIGINAL_DIRECT
#include <recon-original-direct.h>
#else
#include <recon-native.h>
#endif
#include <recon.h>
#ifdef RECON_ORIGINAL_DIRECT
static ReconOriginalProbe *probe;
#define AT(rva) RECON_ORIGINAL_ADDRESS(probe,rva)
#define input_file (*(int *)AT(0x232a4))
#define file_slots (*(FileSlot ***)AT(0x23000))
#define page_limit (*(int *)AT(0x28d0c))
#define mapped_files (*(unsigned char *)AT(0x28d28))
#define closed_files (*(FileSlot **)AT(0x28d1c))
#define closed_file_count (*(int *)AT(0x28d08))
#define open_file (*(int (__cdecl *)(const char *,int,...))AT(0x19cf0))
#define close_file (*(int (__cdecl *)(int,unsigned char))AT(0x19ed0))
#define seek_file (*(long (__cdecl *)(int,long,int))AT(0x1a2e0))
#define read_file (*(int (__cdecl *)(int,void *,unsigned long))AT(0x1a400))
#define write_file (*(int (__cdecl *)(int,const void *,unsigned long))AT(0x1a670))
#endif
static unsigned char buffer[65536], verify[65536];
static unsigned invoked;
static void fail(void)
{
#ifdef RECON_ORIGINAL_DIRECT
    recon_original_finish(probe,0);
#else
    recon_native_entry_finish(0);
#endif
    for (;;) ;
}
static void __cdecl at_initialize(void)
{
    int source, pdb, copy, cached;
    unsigned long offset;
    FileSlot *first;
    if (invoked++ || !mapped_files || page_limit != 2 || closed_file_count ||
        close_file(input_file,1) != 1) fail();
    source = open_file("SUPERPAD.EXE",0x8000,0);
    if (source < 0 || read_file(source,buffer,64) != 64 ||
        buffer[0] != 'M' || buffer[1] != 'Z') fail();
    first = file_slots[source];
    if (!(first->flags & 8) || first->length != 4194304) fail();
    cached = source;
    if (close_file(source,0) || closed_file_count != 1 || closed_files != first) fail();
    source = open_file("SUPERPAD.EXE",0x8000,0);
    if (source != cached || file_slots[source] != first || closed_file_count ||
        read_file(source,verify,64) != 64 || memcmp(buffer,verify,64) ||
        close_file(source,0)) fail();
    pdb = open_file("SUPERPAD.PDB",0x8000,0);
    if (pdb < 0 || read_file(pdb,buffer,64) != 64 ||
        memcmp(buffer,"Microsoft C/C++ program database 1.00",35) || close_file(pdb,0)) fail();
    /* Actual cache threshold is page_limit: adding the second distinct file
       evicts the oldest, leaving one. These are observations, never state writes. */
    if (closed_file_count != 1 || closed_files != file_slots[pdb] ||
        first->flags || first->name) fail();
    source = open_file("SUPERPAD.EXE",0x8000,0);
    copy = open_file("copied.exe",0x8102,0600);
    if (source < 0 || copy < 0 || file_slots[copy]->allocated != 0x200000) fail();
    for (offset=0; offset<4194304; offset+=sizeof(buffer)) {
        if (read_file(source,buffer,sizeof(buffer)) != sizeof(buffer) ||
            write_file(copy,buffer,sizeof(buffer)) != sizeof(buffer)) fail();
    }
    if (file_slots[copy]->length != 4194304 || file_slots[copy]->allocated < 4194304 ||
        seek_file(source,0,0) != 0 || seek_file(copy,0,0) != 0) fail();
    for (offset=0; offset<4194304; offset+=sizeof(buffer)) {
        if (read_file(source,buffer,sizeof(buffer)) != sizeof(buffer) ||
            read_file(copy,verify,sizeof(verify)) != sizeof(verify) ||
            memcmp(buffer,verify,sizeof(buffer))) fail();
    }
    if (close_file(source,1) != 1 || close_file(copy,1) != 1) fail();
#ifdef RECON_ORIGINAL_DIRECT
    recon_original_finish(probe,1);
#else
    recon_native_entry_finish(1);
#endif
}
#ifdef RECON_ORIGINAL_DIRECT
__declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *context)
{
    probe=context;
    if (!context || context->abi_version != RECON_ORIGINAL_PROBE_VERSION ||
        context->struct_size != sizeof(*context) || context->marker_rva != 0x93a0) fail();
    at_initialize();
}
#else
extern int main(int,char **,char **);
static void invoke(void *unused)
{
    char *args[]={"CVPACK.EXE","/nologo","/mapio","/cvold","embedded.exe",NULL};
    (void)unused;
    main(5,args,(char **)0);
}
int cvpack_direct(void)
{
    int result=recon_native_at_entry((const void *)initialize_type_tables,
                                     (const void *)at_initialize,invoke,NULL);
    recon_record_terminal_status(result == 1 ? 0 : 1);
    syscall(SYS_exit_group,result == 1 ? 0 : 1);
    return 0;
}
#endif
