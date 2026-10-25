/* Owned input/output regions supplied by real resource allocation policies. */
#include <string.h>
#include "../../src/resources.h"
#ifdef RECON_ORIGINAL_DIRECT
#include <recon-original-direct.h>
#endif
#include <recon.h>
#ifdef RECON_ORIGINAL_DIRECT
static ReconOriginalProbe *original_context;
#define AT(rva) RECON_ORIGINAL_ADDRESS(original_context,rva)
#define allocate_initial (*(void *(__cdecl *)(unsigned short))AT(0x32c10))
#define resize_block (*(void *(__cdecl *)(void *,unsigned short))AT(0x32c40))
#define release_block (*(int (__cdecl *)(void *))AT(0x32c70))
#define convert_group (*(unsigned long (__cdecl *)(const unsigned char *,unsigned long,unsigned char *))AT(0x348b0))
#else
extern unsigned long convert_legacy_image_group(const unsigned char *,unsigned long,unsigned char *);
#define allocate_initial resource_allocate
#define resize_block resource_reallocate
#define release_block resource_release
#define convert_group convert_legacy_image_group
#endif
static unsigned char payload[97],unchanged[97],input_before[160],output_before[224];
static unsigned long report[512];
unsigned long resource_allocation_phase;
unsigned char *resource_allocation_input,*resource_allocation_output;
static int read_payload(void)
{
    HANDLE file;DWORD count;unsigned char extra;
    file=CreateFileA("payload.bin",GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return 0;
    if(!ReadFile(file,payload,sizeof(payload),&count,NULL)||count!=sizeof(payload)||
       !ReadFile(file,&extra,1,&count,NULL)||count||!CloseHandle(file))return 0;
    return 1;
}
static int workflow(void)
{
    unsigned long i,j,size,length,position=16,*row,in_size=128,out_size=192;
    unsigned long next_input,next_output,kept_input,kept_output;
    unsigned char *cursor;unsigned short cw,sw;
    HANDLE output;DWORD count;
    if(!read_payload()||((unsigned long *)payload)[0]!=0x31534752)return 0;
    memcpy(unchanged,payload,sizeof(payload));memset(report,0,sizeof(report));
    report[0]=0x31414c52;report[1]=2;report[2]=3;
    __asm { fnstcw cw }
    __asm { fnstsw sw }
    report[3]=cw;report[4]=sw;
    resource_allocation_input=allocate_initial((unsigned short)in_size);
    resource_allocation_output=allocate_initial((unsigned short)out_size);
    if(!resource_allocation_input||!resource_allocation_output||
       resource_allocation_input==resource_allocation_output)return 0;
    for(i=0;i<in_size;++i)if(resource_allocation_input[i])return 0;
    for(i=0;i<out_size;++i)if(resource_allocation_output[i])return 0;
    for(resource_allocation_phase=0;resource_allocation_phase<3;++resource_allocation_phase){
        size=((unsigned long *)payload)[resource_allocation_phase+1];
        if(size>64||position+size>76)return 0;
        next_input=resource_allocation_phase==1?96:160;
        next_output=resource_allocation_phase==1?128:224;
        kept_input=in_size<next_input?in_size:next_input;
        kept_output=out_size<next_output?out_size:next_output;
        memcpy(input_before,resource_allocation_input,kept_input);
        memcpy(output_before,resource_allocation_output,kept_output);
        resource_allocation_input=resize_block(resource_allocation_input,(unsigned short)next_input);
        resource_allocation_output=resize_block(resource_allocation_output,(unsigned short)next_output);
        if(!resource_allocation_input||!resource_allocation_output||
           resource_allocation_input==resource_allocation_output||
           memcmp(input_before,resource_allocation_input,kept_input)||
           memcmp(output_before,resource_allocation_output,kept_output))return 0;
        in_size=next_input;out_size=next_output;
        /* Realloc's grown tail is undefined: initialize before any read or report.
         * These caller writes neither repair nor emulate target allocator state. */
        memset(resource_allocation_input,0xa5,in_size);
        memset(resource_allocation_output,0xa5,out_size);
        memcpy(resource_allocation_input+16,payload+position,size);
        memcpy(input_before,resource_allocation_input,in_size);
        length=convert_group(resource_allocation_input+16,size,resource_allocation_output+16);
        if(length>out_size-16||memcmp(input_before,resource_allocation_input,in_size)||
           memcmp(unchanged,payload,sizeof(payload)))return 0;
        for(i=0;i<out_size;++i)if((i<16||i>=16+length)&&resource_allocation_output[i]!=0xa5)return 0;
        row=report+16+104*resource_allocation_phase;row[0]=size;row[1]=length;
        row[2]=in_size;row[3]=out_size;row[4]=kept_input;row[5]=kept_output;
        cursor=(unsigned char *)(row+8);memcpy(cursor,resource_allocation_input,in_size);
        memcpy(cursor+160,resource_allocation_output,out_size);
        position+=size;
    }
    /* No payload read after either real free. */
    j=release_block(resource_allocation_output);i=release_block(resource_allocation_input);
    resource_allocation_output=resource_allocation_input=NULL;
    if(i||j||release_block(NULL))return 0;
    if(position!=76||memcmp(unchanged,payload,sizeof(payload)))return 0;
    __asm { fnstcw cw }
    __asm { fnstsw sw }
    report[5]=cw;report[6]=sw;
    output=CreateFileA("allocation.bin",GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(output==INVALID_HANDLE_VALUE)return 0;
    return WriteFile(output,report,sizeof(report),&count,NULL)&&count==sizeof(report)&&CloseHandle(output);
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
int run_resource_allocation(void){return workflow();}
#endif
