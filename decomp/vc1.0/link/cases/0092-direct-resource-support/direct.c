/* Explicit resource-support caller; not a LINK graph vertex or public CLI path. */
#include <string.h>
#include <setjmp.h>
#include "../../src/resources.h"
#ifdef RECON_ORIGINAL_DIRECT
#include <recon-original-direct.h>
#else
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>
#endif
#include <recon.h>

#ifdef RECON_ORIGINAL_DIRECT
static ReconOriginalProbe *original_context;
#define AT(rva) RECON_ORIGINAL_ADDRESS(original_context,rva)
#define copy_identifier (*(void (__cdecl *)(unsigned char **,const unsigned char **))AT(0x343d0))
#define convert_group (*(unsigned long (__cdecl *)(const unsigned char *,unsigned long,unsigned char *))AT(0x348b0))
#define call_converter (*(int (__cdecl *)(int,char **))AT(0x32870))
#define abort_context (*(jmp_buf *)AT(0x4c590))
#else
extern void resource_copy_ansi_identifier(unsigned char **,const unsigned char **);
extern unsigned long convert_legacy_image_group(const unsigned char *,unsigned long,unsigned char *);
extern jmp_buf resource_abort;
#define copy_identifier resource_copy_ansi_identifier
#define convert_group convert_legacy_image_group
#define call_converter convert_resources
#define abort_context resource_abort
#endif

#ifndef RECON_ORIGINAL_DIRECT
int *resource_test_errno;
#endif
static unsigned char input[97],unchanged[97],oracle[4096];
static unsigned long report[1024],report_used,failed;
static unsigned char *report_cursor;
static union {unsigned long alignment;unsigned char bytes[192];} output;
static unsigned long group_phase,alignment_phase;
static int read_exact(const char *name,void *data,unsigned long size)
{
#ifdef RECON_ORIGINAL_DIRECT
    DWORD n;
    unsigned char extra;
    HANDLE file=CreateFileA(name,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return 0;
    if(!ReadFile(file,data,size,&n,NULL)||n!=size||
       !ReadFile(file,&extra,1,&n,NULL)||n||!CloseHandle(file))return 0;
#else
    unsigned char extra;
    int fd=syscall(SYS_openat,AT_FDCWD,name,O_RDONLY,0);
    if(fd<0)return 0;
    if(syscall(SYS_read,fd,data,size)!=size||syscall(SYS_read,fd,&extra,1)!=0||
       syscall(SYS_close,fd))return 0;
#endif
    return 1;
}
static void append(const void *data,unsigned long size)
{
    if(report_used+size>sizeof(report)){++failed;return;}
    memcpy(report_cursor,data,size);report_cursor+=size;report_used+=size;
}
static void formats(void)
{
    const unsigned char *source,*read_cursor;
    unsigned char *destination,*write_cursor;
    unsigned long size,length,row[4],i,at=16;
    for(group_phase=0;group_phase<3;++group_phase){
        size=((unsigned long *)input)[group_phase+1];source=input+at;
        for(alignment_phase=0;alignment_phase<4;++alignment_phase){
            memset(output.bytes,0xa5,sizeof(output.bytes));destination=output.bytes+32+alignment_phase;
            length=convert_group(source,size,destination);
            if(length>96||memcmp(input,unchanged,sizeof(input)))++failed;
            for(i=0;i<sizeof(output.bytes);++i)
                if((i<32+alignment_phase||i>=32+alignment_phase+length)&&output.bytes[i]!=0xa5)++failed;
            row[0]=group_phase;row[1]=alignment_phase;row[2]=length;row[3]=failed;
            append(row,sizeof(row));append(output.bytes,sizeof(output.bytes));
        }
        at+=size;
    }
    if(at!=76)++failed;
    for(group_phase=0;group_phase<2;++group_phase){
        size=((unsigned long *)(input+76))[group_phase];
        source=input+84+(group_phase?3:0);
        for(alignment_phase=0;alignment_phase<4;++alignment_phase){
            memset(output.bytes,0xa5,sizeof(output.bytes));destination=output.bytes+32+alignment_phase;
            read_cursor=source;write_cursor=destination;
            copy_identifier(&write_cursor,&read_cursor);
            length=write_cursor-destination;
            if(length>32||read_cursor-source!=size||memcmp(input,unchanged,sizeof(input)))++failed;
            for(i=0;i<sizeof(output.bytes);++i)
                if((i<32+alignment_phase||i>=32+alignment_phase+length)&&output.bytes[i]!=0xa5)++failed;
            row[0]=group_phase;row[1]=alignment_phase;row[2]=read_cursor-source;row[3]=length;
            append(row,sizeof(row));append(output.bytes,96);
        }
    }
}
/* Establish a real caller context, never fabricated jump-buffer words. The
 * usage phase is held if actual original registration/scope fails the existing
 * no-SEH-unwind contract. The saved raw context remains separate evidence. */
static int usage(void)
{
    char tool[]="cvtres";
    char *arguments[2];
    volatile int result;
    unsigned long eligible;
#ifdef RECON_ORIGINAL_DIRECT
    unsigned long head,context_words[12];
    unsigned i;
    result=((int (__cdecl *)(void *))AT(0x42738))(abort_context);
    if(!result){
        __asm { mov eax,fs:[0] }
        __asm { mov head,eax }
        for(i=0;i<8;++i)context_words[i]=((unsigned long *)abort_context)[i];
        context_words[8]=head;
        context_words[9]=head&&head!=0xffffffffUL?*(unsigned long *)(head+12):0xffffffffUL;
        context_words[10]=0;context_words[11]=0;
        eligible=head==context_words[6]&&context_words[7]==0&&context_words[9]==0;
        if(!recon_original_emit(original_context,".original-jump-state.bin",context_words,sizeof(context_words))||
           !recon_original_emit(original_context,".usage-gate.bin",&eligible,4))return 0;
        if(!eligible){report[4]=0;return 1;}
#else
    if(!read_exact(".usage-gate.bin",&eligible,4)||eligible>1)return 0;
    if(!eligible){report[4]=0;return 1;}
    result=setjmp(abort_context);
    if(!result){
#endif
        report[4]=1;arguments[0]=tool;arguments[1]=NULL;
        call_converter(1,arguments);
        return 0;
    }
    if(result!=-1)return 0;
    report[5]=(unsigned long)result;
    /* Explicit driver flush, not an invented application caller/CRT edge. */
#ifdef RECON_ORIGINAL_DIRECT
    {
        unsigned long returned[4];
        __asm { mov eax,fs:[0] }
        __asm { mov head,eax }
        returned[0]=head;
        returned[1]=head&&head!=0xffffffffUL?*(unsigned long *)(head+12):0xffffffffUL;
        returned[2]=((unsigned long *)abort_context)[6];
        returned[3]=((unsigned long *)abort_context)[7];
        if(!recon_original_emit(original_context,".original-jump-return.bin",returned,sizeof(returned))||
           returned[0]!=returned[2]||returned[1]||returned[3])return 0;
    }
    return ((int (__cdecl *)(void *))AT(0x40e00))(NULL)==0;
#else
    return recon_fflush(NULL)==0;
#endif
}
static int workflow(void)
{
#ifndef RECON_ORIGINAL_DIRECT
    resource_test_errno=&errno;
#endif
    memset(report,0,sizeof(report));failed=0;report_used=32;
    report_cursor=(unsigned char *)report+32;
    if(!read_exact("payload.bin",input,sizeof(input))||
       ((unsigned long *)input)[0]!=0x31534752||
       ((unsigned long *)input)[1]!=6||((unsigned long *)input)[2]!=20||
       ((unsigned long *)input)[3]!=34)return 0;
    memcpy(unchanged,input,sizeof(input));formats();
    if(!usage())++failed;
    report[0]=0x3153524c;report[1]=1;report[2]=20;report[3]=failed;report[6]=report_used;
#ifdef RECON_ORIGINAL_DIRECT
    if(!recon_original_emit(original_context,".resource-results.bin",report,sizeof(report)))return 0;
#else
    if(!read_exact(".resource-results.bin",oracle,sizeof(oracle))||memcmp(oracle,report,sizeof(report)))return 0;
#endif
    return !failed&&memcmp(input,unchanged,sizeof(input))==0;
}
#ifdef RECON_ORIGINAL_DIRECT
__declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *context)
{
    original_context=context;
    if(!context||context->abi_version!=RECON_ORIGINAL_PROBE_VERSION||
       context->struct_size!=sizeof(*context)||context->marker_rva!=0x16b40)
        recon_original_finish(context,0);
    recon_original_finish(context,workflow());
}
#else
int run_resource_support(void){return workflow();}
#endif
