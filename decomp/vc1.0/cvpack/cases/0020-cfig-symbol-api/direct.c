/* Internal serializer API coverage of unchanged genuine CFIG386 declarations.
   No based-pointer CLI reach or wide-record coverage is claimed. */
#ifndef RECON_ORIGINAL_DIRECT
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#endif
#include <string.h>
#include "files.h"
#include "image.h"
#include "types.h"
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
#define seek_file (*(long (__cdecl *)(int,long,int))AT(0x1a2e0))
#define read_file (*(int (__cdecl *)(int,void *,unsigned long))AT(0x1a400))
#define input_file (*(int *)AT(0x232a4))
#define convert_old_based_symbol (*(unsigned short (__cdecl *)(unsigned char *,unsigned char *))AT(0x13730))
#endif
static unsigned char input[80575], output[512], saved[256];
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
static unsigned short word(const unsigned char *p) { return p[0]|((unsigned short)p[1]<<8); }
static unsigned long dword(const unsigned char *p) { return word(p)|((unsigned long)word(p+2)<<16); }
static void put(HANDLE out,const void *data,unsigned long bytes)
{
    DWORD written;
    if (!WriteFile(out,data,bytes,&written,NULL) || written!=bytes) fail();
}
static void __cdecl at_initialize(void)
{
    unsigned char *debug,*entry,*record,*end;
    unsigned long size,base,directory,offset,record_offset,count=0,total=423;
    unsigned short rows,i,j,module,bytes,converted,counts[3]={0,0,0};
    unsigned char kind;
    HANDLE out;
    if (invoked++ || seek_file(input_file,0,2)!=sizeof(input) ||
        seek_file(input_file,0,0)!=0 || read_file(input_file,input,sizeof(input))!=sizeof(input) ||
        close_file(input_file,1)!=0) fail();
    /* Decode only the authentic file's directory/record framing as test glue;
       no target input byte or count is rewritten. */
    size=dword(input+sizeof(input)-4);
    if (size!=37096 || memcmp(input+sizeof(input)-8,"NB02",4)) fail();
    base=sizeof(input)-size; debug=input+base;
    if (memcmp(debug,"NB02",4)) fail();
    directory=dword(debug+4);
    if (directory>size-10) fail();
    rows=word(debug+directory);
    if (rows!=212 || directory+2+rows*10UL+8!=size) fail();
    out=CreateFileA("c6-symbols.bin",GENERIC_WRITE,0,NULL,CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,NULL);
    if (out==INVALID_HANDLE_VALUE) fail();
    put(out,"C6API1\0\0",8); put(out,&total,4);
    for (i=0;i<rows;++i) {
        entry=debug+directory+2+i*10UL;
        if (word(entry)!=0x104) continue;
        module=word(entry+2); offset=dword(entry+4); bytes=word(entry+8);
        if (offset>directory || bytes>directory-offset) fail();
        record=debug+offset; end=record+bytes;
        while (record<end) {
            if (end-record<2 || record[0]<1 || record+record[0]+1>end) fail();
            bytes=record[0]+1; kind=record[1]&0x7f;
            if (kind==4 || kind==5 || kind==13) {
                if (record[1]&0x80) fail();
                for (j=0;j<sizeof(output);++j) output[j]=0xcc;
                memcpy(saved,record,bytes);
                converted=convert_old_based_symbol(output,record);
                if (converted<4 || converted>sizeof(output)-16 || (converted&3) ||
                    word(output)+2!=converted || memcmp(saved,record,bytes)) fail();
                for (j=converted;j<sizeof(output);++j) if (output[j]!=0xcc) fail();
                if (kind==4) ++counts[0];
                else if (kind==5) ++counts[1];
                else ++counts[2];
                record_offset=record-input;
                put(out,&module,2); put(out,&record_offset,4);
                put(out,&bytes,2); put(out,&converted,2);
                put(out,record,bytes); put(out,output,converted);
                ++count;
            }
            record+=bytes;
        }
    }
    if (count!=total || counts[0]!=307 || counts[1]!=106 || counts[2]!=10) fail();
    put(out,counts,sizeof(counts));
    if (!CloseHandle(out)) fail();
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
    if (!context || context->abi_version!=RECON_ORIGINAL_PROBE_VERSION ||
        context->struct_size!=sizeof(*context) || context->marker_rva!=0x93a0) fail();
    at_initialize();
}
#else
extern int main(int,char **,char **);
static void invoke(void *unused)
{
    char *args[]={"CVPACK.EXE","/nologo","/cvold","CFIG386.EXE",NULL};
    (void)unused;
    main(4,args,(char **)0);
}
int cvpack_direct(void)
{
    int result=recon_native_at_entry((const void *)initialize_type_tables,
                                     (const void *)at_initialize,invoke,NULL);
    recon_record_terminal_status(result==1?0:1);
    syscall(SYS_exit_group,result==1?0:1);
    return 0;
}
#endif
