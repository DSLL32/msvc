/* External standalone cvtomf caller: real parser state, FILE owners and abort
 * context. No LINK CLI reach or asynchronous unwind claim. */
#include <string.h>
#include <setjmp.h>
#include "../../src/omf.h"
#ifdef RECON_ORIGINAL_DIRECT
#include <recon-original-direct.h>
#endif
#include <recon.h>
#ifdef RECON_ORIGINAL_DIRECT
static ReconOriginalProbe *original_context;
#define AT(rva) RECON_ORIGINAL_ADDRESS(original_context,rva)
#define call_converter (*(int (__cdecl *)(int,char **))AT(0x38010))
#define abort_context (*(jmp_buf *)AT(0x4c43c))
#define module_count (*(unsigned short *)AT(0x67e10))
#define source_name ((char *)AT(0x4c470))
#define target_fopen (*(FILE *(__cdecl *)(const char *,const char *))AT(0x41140))
#define target_fwrite (*(unsigned (__cdecl *)(const void *,unsigned,unsigned,FILE *))AT(0x43910))
#define target_fflush (*(int (__cdecl *)(FILE *))AT(0x40e00))
#else
#define call_converter omf_main
#define abort_context omf_abort
#define module_count omf_module_count
#define source_name omf_source_filename
#define target_fopen fopen
#define target_fwrite fwrite
#define target_fflush fflush
#endif
static unsigned long report[128],jump_rows[3][12];
unsigned long omf_lifecycle_phase;
FILE *omf_lifecycle_stream;
static char tool[]="cvtomf",output_option[]="-o",output_name[]="converted.obj";
static char member_name[]="member.obj",conflict_name[]="conflict.obj";
static char missing_name[]="missing.obj";
static char *positive[]={tool,output_option,output_name,member_name,NULL};
static char *conflict[]={tool,output_option,conflict_name,member_name,member_name,NULL};
static char *empty[]={tool,NULL},*missing[]={tool,missing_name,NULL};
static const char *stream_names[]={"error-conflict.bin","error-empty.bin","error-missing.bin"};
static const char *stream_data[]={"conflicting output\n","missing argument\n","missing input file\n"};
static unsigned long string_length(const char *s)
{
    unsigned long n=0;while(s[n])++n;return n;
}
static void save_fp(unsigned long *row)
{
    unsigned short cw,sw;
    __asm { fnstcw cw }
    __asm { fnstsw sw }
    row[0]=cw;row[1]=sw;
}
static int error_phase(void)
{
    volatile int result;
    unsigned i;unsigned long *row=report+16+16*(omf_lifecycle_phase-1);
#ifdef RECON_ORIGINAL_DIRECT
    unsigned long head,*state=jump_rows[omf_lifecycle_phase-1];
#endif
    omf_lifecycle_stream=target_fopen(stream_names[omf_lifecycle_phase-1],"wb");
    if(!omf_lifecycle_stream||target_fwrite(stream_data[omf_lifecycle_phase-1],1,
       string_length(stream_data[omf_lifecycle_phase-1]),omf_lifecycle_stream)!=
       string_length(stream_data[omf_lifecycle_phase-1]))return 0;
    row[0]=omf_lifecycle_stream->_flag;row[1]=omf_lifecycle_stream->_cnt;
    row[2]=omf_lifecycle_stream->_ptr-omf_lifecycle_stream->_base;
    row[3]=omf_lifecycle_stream->_bufsiz;
#ifdef RECON_ORIGINAL_DIRECT
    result=((int (__cdecl *)(void *))AT(0x42738))(abort_context);
    if(!result){
        __asm { mov eax,fs:[0] }
        __asm { mov head,eax }
        for(i=0;i<8;++i)state[i]=((unsigned long *)abort_context)[i];
        state[8]=head;state[9]=head&&head!=0xffffffffUL?*(unsigned long *)(head+12):0xffffffffUL;
        if(head!=state[6]||state[7]||state[9])return 0;
#else
    result=setjmp(abort_context);
    if(!result){
#endif
        if(omf_lifecycle_phase==1)call_converter(5,conflict);
        else if(omf_lifecycle_phase==2)call_converter(1,empty);
        else call_converter(2,missing);
        return 0;
    }
    if(result!=-1||omf_lifecycle_stream->_flag)return 0;
    row[4]=(unsigned long)result;row[5]=omf_lifecycle_stream->_flag;
    row[6]=module_count;save_fp(row+8);
#ifdef RECON_ORIGINAL_DIRECT
    __asm { mov eax,fs:[0] }
    __asm { mov head,eax }
    state[10]=head;state[11]=head&&head!=0xffffffffUL?*(unsigned long *)(head+12):0xffffffffUL;
    if(state[10]!=state[6]||state[11])return 0;
#endif
    return 1;
}
static int write_report(void)
{
    HANDLE file;DWORD count;
    file=CreateFileA("lifecycle.bin",GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return 0;
    return WriteFile(file,report,sizeof(report),&count,NULL)&&count==sizeof(report)&&CloseHandle(file);
}
static int workflow(void)
{
    memset(report,0,sizeof(report));report[0]=0x31454d4f;report[1]=1;
    omf_lifecycle_phase=0;report[2]=module_count;save_fp(report+4);
    if(call_converter(4,positive))return 0;
    report[3]=module_count;save_fp(report+6);
    if(module_count!=1)return 0;
    for(omf_lifecycle_phase=1;omf_lifecycle_phase<=3;++omf_lifecycle_phase)
        if(!error_phase())return 0;
    if(target_fflush(NULL))return 0;
#ifdef RECON_ORIGINAL_DIRECT
    if(!recon_original_emit(original_context,".original-jump-state.bin",jump_rows,sizeof(jump_rows)))return 0;
#endif
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
int run_omf_header_errors(void){return workflow();}
#endif
