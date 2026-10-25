/* Explicit synchronous control-handler caller; never public console delivery.
 * Original ignores its DWORD argument and always invokes real cleanup. */
#include <string.h>
#include <stddef.h>
#include "../../src/link.h"
#include "../../src/files.h"
#include "../../src/arguments.h"
#ifdef RECON_ORIGINAL_DIRECT
#include <recon-original-direct.h>
#endif
#include <recon.h>
typedef char CleanupSlot40[sizeof(FileSlot)==40?1:-1];
typedef char CleanupSlotOffsets[offsetof(FileSlot,name)==0&&offsetof(FileSlot,flags)==4&&
    offsetof(FileSlot,source)==16&&offsetof(FileSlot,storage)==20&&offsetof(FileSlot,contents)==24?1:-1];
#ifdef RECON_ORIGINAL_DIRECT
static ReconOriginalProbe *original_context;
#define AT(rva) RECON_ORIGINAL_ADDRESS(original_context,rva)
#define initialize_files (*(void (__cdecl *)(int,int,int,int,int,int,void (*)(char *,int,char *)))AT(0x2c020))
#define open_owned (*(int (__cdecl *)(const char *,int,int))AT(0x2c470))
#define write_owned (*(unsigned (__cdecl *)(int,const void *,unsigned))AT(0x2ce90))
#define close_owned (*(int (__cdecl *)(int,unsigned char))AT(0x2c6d0))
#define copy_argument (*(void (__cdecl *)(ArgumentList *,char *,char *))AT(0x109f0))
#define failure_callback (*(void (__cdecl *)(char *,int,char *))AT(0x16b10))
#define actual_handler (*(BOOL (WINAPI *)(DWORD))AT(0x170f0))
#define slot_table (*(FileSlot ***)AT(0x4c000))
#define outputs ((FileSlot **)AT(0x5ed40))
#define argument_list (*(ArgumentList *)AT(0x6b7d0))
#define actual_tool_kind (*(int *)AT(0x4c46c))
#else
extern FileSlot **files,*mapped_outputs[16];
extern void copy_argument_node(ArgumentList *,char *,char *);
extern int open_file(const char *,int,int);
extern int close_file(int,unsigned char);
#define initialize_files files_initialize
#define open_owned open_file
#define write_owned write_file
#define close_owned close_file
#define copy_argument copy_argument_node
#define failure_callback file_failure
#define actual_handler control_handler
#define slot_table files
#define outputs mapped_outputs
#define argument_list library_files
#define actual_tool_kind tool_kind
#endif
static unsigned char payload[1536];
static char temporary_name[]="temporary.tmp",rewritten_name[]="rewritten.obj";
static char retained_name[]="retain-input.bin";
int cleanup_test_handles[3];
unsigned long cleanup_test_phase;
static unsigned long report[128];
static int read_payload(void)
{
    unsigned char extra;DWORD count;
    HANDLE file=CreateFileA("payload.bin",GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return 0;
    if(!ReadFile(file,payload,sizeof(payload),&count,NULL)||count!=sizeof(payload)||
       !ReadFile(file,&extra,1,&count,NULL)||count||!CloseHandle(file))return 0;
    return 1;
}
static int precleanup(void)
{
    unsigned i,j;FileSlot *file;Argument *argument;HANDLE output;DWORD written;
    unsigned short cw,sw;
    memset(report,0,sizeof(report));report[0]=0x314e4c43;report[1]=2;report[7]=CTRL_C_EVENT;report[2]=16;
    if(actual_tool_kind!=0||argument_list.count!=2)return 0;
    for(i=0;i<16;++i){
        file=outputs[i];report[8+i]=file==NULL?0:1;
        if(i==0||i==2){
            j=i==0?0:2;
            if(file!=slot_table[cleanup_test_handles[j]]||!(file->flags&8)||!(file->flags&2)||
               file->length!=512||file->allocated!=0x200000||file->cursor.position!=512||
               !file->source.handle||!file->storage.mapping||!file->contents.view||
               memcmp(file->contents.view,payload+j*512,512))return 0;
            for(j=512;j<544;++j)if(file->contents.view[j])return 0;
            report[24+i*4]=file->flags;report[25+i*4]=file->length;
            report[26+i*4]=file->allocated;report[27+i*4]=file->cursor.position;
        }else if(file)return 0;
    }
    file=slot_table[cleanup_test_handles[1]];
    if(file->flags||file->name||file->source.handle||file->storage.mapping||file->contents.view)return 0;
    argument=argument_list.first;
    if(!argument||argument->text!=rewritten_name||argument->original!=temporary_name||
       !argument->next||argument->next->text!=retained_name||argument->next->original!=retained_name||
       argument->next->next||argument_list.last!=argument->next)return 0;
    report[3]=argument_list.count;
    __asm { fnstcw cw }
    __asm { fnstsw sw }
    report[4]=cw;report[5]=sw;report[6]=GetLastError();
    output=CreateFileA("precleanup.bin",GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(output==INVALID_HANDLE_VALUE||!WriteFile(output,report,sizeof(report),&written,NULL)||written!=sizeof(report)||!CloseHandle(output))return 0;
    return 1;
}
static int workflow(void)
{
    unsigned i;const char *names[3];
    if(!read_payload())return 0;
    names[0]="live-a.bin";names[1]="retained-output.bin";names[2]="live-b.bin";
    cleanup_test_phase=0;initialize_files(30,16,14,8,6,1,failure_callback);
    for(i=0;i<3;++i){
        cleanup_test_phase=1+i;cleanup_test_handles[i]=open_owned(names[i],0x8102,0x180);
        if(cleanup_test_handles[i]<0||write_owned(cleanup_test_handles[i],payload+i*512,512)!=512)return 0;
    }
    cleanup_test_phase=4;if(!close_owned(cleanup_test_handles[1],1))return 0;
    cleanup_test_phase=5;copy_argument(&argument_list,rewritten_name,temporary_name);
    copy_argument(&argument_list,retained_name,retained_name);
    cleanup_test_phase=6;if(!precleanup())return 0;
    cleanup_test_phase=7;(void)actual_handler(CTRL_C_EVENT);
    return 0;
}
#ifdef RECON_ORIGINAL_DIRECT
__declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *context)
{
    original_context=context;
    if(!context||context->abi_version!=RECON_ORIGINAL_PROBE_VERSION||context->struct_size!=sizeof(*context)||context->marker_rva!=0x16b40)
        recon_original_finish(context,0);
    (void)workflow();
    recon_original_finish(context,0);
}
#else
int run_control_cleanup(void){return workflow();}
#endif
