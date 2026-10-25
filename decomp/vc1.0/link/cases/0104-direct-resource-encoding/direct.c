/* No input execution: inspect caller-owned streams through actual CRT APIs. */
#include <string.h>
#include "../../src/resources.h"
#ifdef RECON_ORIGINAL_DIRECT
#include <recon-original-direct.h>
#endif
#include <recon.h>
#ifdef RECON_ORIGINAL_DIRECT
static ReconOriginalProbe *original_context;
#define AT(rva) RECON_ORIGINAL_ADDRESS(original_context,rva)
#define open_stream (*(FILE *(__cdecl *)(const char *,const char *))AT(0x41140))
#define close_stream (*(int (__cdecl *)(FILE *))AT(0x41000))
#define position_stream (*(long (__cdecl *)(FILE *))AT(0x43030))
#define seek_stream (*(int (__cdecl *)(FILE *,long,int))AT(0x42f90))
#define read_byte (*(int (__cdecl *)(FILE *))AT(0x43a80))
#define detect_encoding (*(int (__cdecl *)(FILE *))AT(0x35220))
#define actual_mb_cur_max (*(int *)AT(0x7554c))
#else
#define open_stream fopen
#define close_stream fclose
#define position_stream ftell
#define seek_stream fseek
#define read_byte fgetc
#define detect_encoding detect_resource_encoding
#define actual_mb_cur_max MB_CUR_MAX
#endif
static const char *names[8]={"ascii.bin","ascii.bin","le.bin","le.bin","be.bin","be.bin","controls.bin","empty.bin"};
static const long positions[8]={0,1,0,1,0,1,0,0};
static const int encodings[8]={1,1,2,2,3,3,0,0};
static const int bytes[8]={'A','l','A',0,0,'A',1,EOF};
static unsigned long report[128];
FILE *encoding_stream;
unsigned encoding_phase;
static void floating_state(unsigned long *row)
{
    unsigned short cw,sw;
    __asm { fnstcw cw }
    __asm { fnstsw sw }
    row[0]=cw;row[1]=sw;
}
static int write_report(void)
{
    HANDLE file;DWORD count;
    file=CreateFileA("encoding.bin",GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return 0;
    return WriteFile(file,report,sizeof(report),&count,NULL)&&count==sizeof(report)&&CloseHandle(file);
}
static int workflow(void)
{
    unsigned long *row;int result,next;
    memset(report,0,sizeof(report));report[0]=0x31434e45;report[1]=1;
    if(actual_mb_cur_max!=1)return 0;
    for(encoding_phase=0;encoding_phase<8;++encoding_phase){
        encoding_stream=open_stream(names[encoding_phase],"rb");
        if(!encoding_stream||seek_stream(encoding_stream,positions[encoding_phase],SEEK_SET))return 0;
        row=report+8+encoding_phase*12;row[0]=encoding_phase;
        row[1]=position_stream(encoding_stream);
        if(row[1]!=(unsigned long)positions[encoding_phase])return 0;
        result=detect_encoding(encoding_stream);row[2]=result;
        row[3]=position_stream(encoding_stream);
        if(result!=encodings[encoding_phase]||row[3]!=row[1])return 0;
        next=read_byte(encoding_stream);row[4]=next;
        if(next!=bytes[encoding_phase]||seek_stream(encoding_stream,positions[encoding_phase],SEEK_SET)||
           position_stream(encoding_stream)!=positions[encoding_phase])return 0;
        floating_state(row+5);
        if(close_stream(encoding_stream))return 0;
        /* The stream owner is released here and is never read after close. */
    }
    return write_report();
}
#ifdef RECON_ORIGINAL_DIRECT
__declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *context)
{
    original_context=context;
    if(!context||context->abi_version!=RECON_ORIGINAL_PROBE_VERSION||context->struct_size!=sizeof(*context)||context->marker_rva!=0x16b40)
        recon_original_finish(context,0);
    recon_original_finish(context,workflow());
}
#else
int run_resource_encoding(void){return workflow();}
#endif
