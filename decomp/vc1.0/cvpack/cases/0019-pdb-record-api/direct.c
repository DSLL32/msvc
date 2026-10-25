/* Internal provider API coverage using the genuine compiler-produced PDB.
   The real constructor and mode path define every owner/control field. */
#ifndef RECON_ORIGINAL_DIRECT
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#endif
#include <stdio.h>
#include <string.h>
#include "pdb.h"
#ifdef RECON_ORIGINAL_DIRECT
#include <recon-original-direct.h>
#else
#include <recon-native.h>
#endif
#include <recon.h>
#ifdef RECON_ORIGINAL_DIRECT
static ReconOriginalProbe *probe;
#define AT(rva) RECON_ORIGINAL_ADDRESS(probe,rva)
#define close_file (*(int (__cdecl *)(int,unsigned char))AT(0x19ed0))
#define input_file (*(int *)AT(0x232a4))
#define type_server_open (*(TypeServer *(__cdecl *)(const char *,const char *))AT(0x1b968))
#define type_server_close (*(int (__cdecl *)(TypeServer *))AT(0x1bbe8))
#define type_server_first (*(unsigned short (__cdecl *)(TypeServer *))AT(0x1bcc0))
#define type_server_limit (*(unsigned short (__cdecl *)(TypeServer *))AT(0x1bccc))
#define type_server_record (*(unsigned char *(__cdecl *)(TypeServer *,unsigned short))AT(0x1badc))
#define type_server_signature (*(unsigned long (__cdecl *)(TypeServer *))AT(0x1bca8))
#define type_server_age (*(unsigned long (__cdecl *)(TypeServer *))AT(0x1bcb4))
#endif
static unsigned invoked;
static unsigned char seen[0xefff];
static void fail(void)
{
#ifdef RECON_ORIGINAL_DIRECT
    recon_original_finish(probe,0);
#else
    recon_native_entry_finish(0);
#endif
    for (;;) ;
}
static void put(HANDLE out,const void *data,unsigned long bytes)
{
    DWORD written;
    if (!WriteFile(out,data,bytes,&written,NULL) || written != bytes) fail();
}
static void __cdecl at_initialize(void)
{
    TypeServer *server;
    TypeServerRecord *node;
    unsigned char *record, *end;
    unsigned short first,limit,type,length,bucket,chain;
    unsigned long signature,age,bytes=0,nodes=0;
    HANDLE out;
    if (invoked++ || close_file(input_file,1) != 1) fail();
    server=type_server_open("SUPERPAD.PDB","r");
    if (!server || server->writing || server->incremental != 1 || server->loaded != 1 ||
        !server->type_offsets || !server->type_hash || server->descriptor != -1) fail();
    first=type_server_first(server);
    limit=type_server_limit(server);
    signature=type_server_signature(server);
    age=type_server_age(server);
    if (first != 0x1000 || limit != 0x2a8b || server->type_bytes != 234542) fail();
    out=CreateFileA("pdb-records.bin",GENERIC_WRITE,0,NULL,CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,NULL);
    if (out == INVALID_HANDLE_VALUE) fail();
    put(out,"PDBAPI1\0",8);
    put(out,&signature,4); put(out,&age,4);
    put(out,&first,2); put(out,&limit,2);
    put(out,&server->type_bytes,4);
    end=server->type_storage.last->cursor;
    for (type=first;type<limit;++type) {
        record=type_server_record(server,type);
        if (record != server->type_offsets[type-0x1000]+2 || record+2>end) fail();
        length=*(unsigned short *)record;
        if (length<2 || record+2+length>end) fail();
        put(out,&type,2); put(out,record,(unsigned long)length+2);
        bytes+=(unsigned long)length+4;
    }
    if (bytes != server->type_bytes || server->index_storage.count != (limit-first)*12UL) fail();
    /* Traverse only nodes created from actual stored records. Record IDs and
       chain order are serialized; no pointer values or unwritten padding escape. */
    for (bucket=0;bucket<4096;++bucket) {
        chain=0;
        for (node=server->type_hash[bucket];node;node=node->next) {
            if (++chain>limit-first || node->type<first || node->type>=limit ||
                seen[node->type-first]++ ||
                node->record != server->type_offsets[node->type-0x1000] ||
                *(unsigned short *)node->record != bucket) fail();
            ++nodes;
        }
        put(out,&chain,2);
        for (node=server->type_hash[bucket];node;node=node->next) put(out,&node->type,2);
    }
    if (nodes != limit-first || !CloseHandle(out) || !type_server_close(server)) fail();
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
    char *args[]={"CVPACK.EXE","/nologo","/mapio","/cvold","SUPERPAD.EXE",NULL};
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
