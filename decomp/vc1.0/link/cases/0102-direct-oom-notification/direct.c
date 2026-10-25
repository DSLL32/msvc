/* Explicit failure notification with real owners; no induced allocation failure. */
#include <string.h>
#include <stddef.h>
#include "../../src/link.h"
#include "../../src/files.h"
#include "../../src/arguments.h"
#include "../../src/link-pass.h"
#ifdef RECON_ORIGINAL_DIRECT
#include <recon-original-direct.h>
#endif
#include <recon.h>
#ifdef RECON_ORIGINAL_DIRECT
static ReconOriginalProbe *original_context;
#define AT(rva) RECON_ORIGINAL_ADDRESS(original_context,rva)
#define initialize_files (*(void (__cdecl *)(int,int,int,int,int,int,void (*)(char *,int,char *)))AT(0x2c020))
#define open_owned (*(int (__cdecl *)(const char *,int,int))AT(0x2c470))
#define write_owned (*(unsigned (__cdecl *)(int,const void *,unsigned))AT(0x2ce90))
#define close_owned (*(int (__cdecl *)(int,unsigned char))AT(0x2c6d0))
#define copy_argument (*(void (__cdecl *)(ArgumentList *,char *,char *))AT(0x109f0))
#define failure_callback (*(void (__cdecl *)(char *,int,char *))AT(0x16b10))
#define notify_oom (*(void (__cdecl *)(void))AT(0x11990))
#define slot_table (*(FileSlot ***)AT(0x4c000))
#define argument_list (*(ArgumentList *)AT(0x6b7d0))
#define actual_tool_kind (*(int *)AT(0x4c46c))
#define actual_output_name (*(char **)AT(0x6b76c))
#else
extern FileSlot **files;
extern char *output_name;
extern int open_file(const char *,int,int);
extern int close_file(int,unsigned char);
#define initialize_files files_initialize
#define open_owned open_file
#define write_owned write_file
#define close_owned close_file
#define copy_argument copy_argument_node
#define failure_callback file_failure
#define notify_oom out_of_memory
#define slot_table files
#define argument_list library_files
#define actual_tool_kind tool_kind
#define actual_output_name output_name
#endif
static unsigned char payload[512];
static char temporary_name[]="temporary.tmp",rewritten_name[]="rewritten.obj",retained_name[]="retain-input.bin";
static unsigned long report[64];
static int output_handle,retained_handle;
#ifdef RECON_ORIGINAL_DIRECT
/* The explicit original runner stages its private executable above the case
 * cwd. Supply only its owned sibling catalog from the exact fixture input. */
static int prepare_catalog(void)
{
    static unsigned char catalog[6426];
    char path[MAX_PATH];unsigned char extra;
    HANDLE file;DWORD size,count;unsigned i,last=0;
    const char *name="LINK.EXE";
    file=CreateFileA("link.err",GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return 0;
    if(!ReadFile(file,catalog,sizeof(catalog),&count,NULL)||count!=sizeof(catalog)||
       !ReadFile(file,&extra,1,&count,NULL)||count||!CloseHandle(file))return 0;
    size=GetModuleFileNameA(NULL,path,sizeof(path));
    if(size<16||size>=sizeof(path)||(path[0]!='T'&&path[0]!='t')||
       path[1]!=':'||path[2]!='\\'||memcmp(path+3,"run-",4))return 0;
    for(i=7;i<size;++i){
        if(path[i]=='\\'){last=i;break;}
        if(!((path[i]>='a'&&path[i]<='z')||(path[i]>='A'&&path[i]<='Z')||
             (path[i]>='0'&&path[i]<='9')))return 0;
    }
    if(last<8||size-last!=9)return 0;
    for(i=0;i<9;++i){
        char letter=path[last+1+i];
        if(letter>='a'&&letter<='z')letter-=32;
        if(letter!=name[i])return 0;
    }
    memcpy(path+last+1,"link.err",9);
    file=CreateFileA(path,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return 0;
    if(!WriteFile(file,catalog,sizeof(catalog),&count,NULL)||count!=sizeof(catalog)||!CloseHandle(file))return 0;
    return 1;
}
#endif
static int read_payload(void)
{
    HANDLE file;DWORD count;unsigned char extra;
    file=CreateFileA("payload.bin",GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return 0;
    return ReadFile(file,payload,sizeof(payload),&count,NULL)&&count==sizeof(payload)&&
        ReadFile(file,&extra,1,&count,NULL)&&!count&&CloseHandle(file);
}
static int precleanup(void)
{
    HANDLE file;DWORD count;Argument *argument;FileSlot *slot;unsigned short cw,sw;
    memset(report,0,sizeof(report));report[0]=0x314d4f4f;report[1]=1;
    if(actual_tool_kind!=0||memcmp(actual_output_name,"coff.out",9)||argument_list.count!=2)return 0;
    argument=argument_list.first;
    if(!argument||argument->text!=rewritten_name||argument->original!=temporary_name||
       !argument->next||argument->next->text!=retained_name||argument->next->original!=retained_name||
       argument->next->next||argument_list.last!=argument->next)return 0;
    slot=slot_table[output_handle];
    if(!slot||slot->flags&8||!slot->name||strcmp(slot->name,"coff.out")||slot->source.descriptor<0)return 0;
    report[2]=argument_list.count;report[3]=slot->flags;
    /* Buffered length may update only when its genuine dirty page is flushed. */
    report[4]=slot->length;report[5]=slot->allocated;
    __asm { fnstcw cw }
    __asm { fnstsw sw }
    report[6]=cw;report[7]=sw;
    file=CreateFileA("precleanup.bin",GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return 0;
    return WriteFile(file,report,sizeof(report),&count,NULL)&&count==sizeof(report)&&CloseHandle(file);
}
static int workflow(void)
{
    if(!read_payload())return 0;
    initialize_files(30,16,14,8,6,0,failure_callback);
    output_handle=open_owned("coff.out",0x8102,0x180);
    if(output_handle<0||write_owned(output_handle,payload,sizeof(payload))!=sizeof(payload))return 0;
    retained_handle=open_owned("retained-output.bin",0x8102,0x180);
    if(retained_handle<0||write_owned(retained_handle,payload,sizeof(payload))!=sizeof(payload)||
       close_owned(retained_handle,1)!=0)return 0;
    copy_argument(&argument_list,rewritten_name,temporary_name);
    copy_argument(&argument_list,retained_name,retained_name);
    if(!precleanup())return 0;
    notify_oom();
    return 0;
}
#ifdef RECON_ORIGINAL_DIRECT
__declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *context)
{
    original_context=context;
    if(!context||context->abi_version!=RECON_ORIGINAL_PROBE_VERSION||context->struct_size!=sizeof(*context)||context->marker_rva!=0x16b40)
        recon_original_finish(context,0);
    if(!prepare_catalog())recon_original_finish(context,0);
    (void)workflow();
    recon_original_finish(context,0);
}
#else
int run_oom_notification(void){return workflow();}
#endif
