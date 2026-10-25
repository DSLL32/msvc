/* Ordinary caller-owned COFF integers and real buffered file owners. */
#include <string.h>
#include <stddef.h>
#include "../../src/link.h"
#include "../../src/files.h"
#include "../../src/archive.h"
#include "../../src/link-pass.h"
#ifdef RECON_ORIGINAL_DIRECT
#include <recon-original-direct.h>
#endif
#include <recon.h>
#ifdef RECON_ORIGINAL_DIRECT
static ReconOriginalProbe *original_context;
#define AT(rva) RECON_ORIGINAL_ADDRESS(original_context,rva)
#define align_value (*(void (__cdecl *)(unsigned long *,unsigned short))AT(0xee30))
#define endian_probe (*(unsigned char (__cdecl *)(void))AT(0x15020))
#define initialize_files (*(void (__cdecl *)(int,int,int,int,int,int,void (*)(char *,int,char *)))AT(0x2c020))
#define open_owned (*(int (__cdecl *)(const char *,int,int))AT(0x2c470))
#define close_owned (*(int (__cdecl *)(int,unsigned char))AT(0x2c6d0))
#define write_header (*(unsigned (__cdecl *)(int,IMAGE_FILE_HEADER *))AT(0x15060))
#define read_header (*(unsigned (__cdecl *)(int,IMAGE_FILE_HEADER *))AT(0x15040))
#define swap_read (*(unsigned long (__cdecl *)(const void *))AT(0x117a0))
#define swap_encode (*(unsigned long (__cdecl *)(const void *))AT(0x117d0))
#define error_callback (*(void (__cdecl *)(char *,int,char *))AT(0x16b10))
#define flush_streams (*(int (__cdecl *)(FILE *))AT(0x40e00))
#define actual_output_name (*(char **)AT(0x6b76c))
#else
extern char *output_name;
#define align_value align_coff_value
#define endian_probe is_big_endian
#define initialize_files files_initialize
#define open_owned open_file
#define close_owned close_file
#define write_header write_coff_header
#define read_header read_coff_header
#define swap_read read_big_endian_dword
#define swap_encode encode_big_endian_dword
#define error_callback file_failure
#define flush_streams fflush
#define actual_output_name output_name
#endif
extern int open_file(const char *,int,int);
extern int close_file(int,unsigned char);
typedef struct GuardedDword { unsigned long before,value,after; } GuardedDword;
typedef struct GuardedHeader { unsigned long before;IMAGE_FILE_HEADER value;unsigned long after; } GuardedHeader;
typedef char CoffHeader20[sizeof(IMAGE_FILE_HEADER)==20?1:-1];
typedef char CoffHeaderGuards[sizeof(GuardedHeader)==28 && offsetof(GuardedHeader,after)==24?1:-1];
static const unsigned long values[6]={0x200,0x235,0x43,0x40,0x12345679,0x80000007UL};
static const unsigned short alignments[6]={512,512,16,16,1,4};
static const unsigned long expected[6]={0x200,0x200,0x40,0x40,0x12345679,0x80000004UL};
static GuardedDword scalar;
static GuardedHeader header,readback;
static unsigned long report[128];
unsigned long integer_phase;
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
    file=CreateFileA("integers.bin",GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return 0;
    return WriteFile(file,report,sizeof(report),&count,NULL)&&count==sizeof(report)&&CloseHandle(file);
}
static int physical_header(void)
{
    HANDLE file;DWORD count;IMAGE_FILE_HEADER value;unsigned char extra;
    file=CreateFileA("header.obj",GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return 0;
    return ReadFile(file,&value,sizeof(value),&count,NULL)&&count==sizeof(value)&&
        ReadFile(file,&extra,1,&count,NULL)&&!count&&CloseHandle(file)&&
        !memcmp(&value,&header.value,sizeof(value));
}
static int workflow(void)
{
    unsigned i;unsigned long *row;int file;
    memset(report,0,sizeof(report));report[0]=0x31464f43;report[1]=1;
    if(memcmp(actual_output_name,"coff.out",9))return 0;
    for(integer_phase=0;integer_phase<6;++integer_phase){
        scalar.before=0x11223344;scalar.value=values[integer_phase];scalar.after=0x55667788;
        row=report+8+integer_phase*8;row[0]=scalar.value;row[1]=alignments[integer_phase];
        align_value(&scalar.value,alignments[integer_phase]);
        if(scalar.value!=expected[integer_phase]||scalar.before!=0x11223344||scalar.after!=0x55667788)return 0;
        row[2]=scalar.value;floating_state(row+3);
    }
    integer_phase=6;report[60]=endian_probe();
    if(report[60])return 0;
    memset(&header,0,sizeof(header));header.before=readback.before=0x11223344;
    header.after=readback.after=0x55667788;
    header.value.Machine=IMAGE_FILE_MACHINE_I386;
    /* This is caller-provided record data, not a clock reading. */
    header.value.TimeDateStamp=0x01020304;
    report[61]=swap_read(&header.value.TimeDateStamp);
    report[62]=swap_encode(&header.value.TimeDateStamp);
    if(report[61]!=0x04030201||report[62]!=0x04030201)return 0;
    initialize_files(30,16,14,8,6,0,error_callback);
    file=open_owned("header.obj",0x8102,0x180);
    if(file<0||write_header(file,&header.value)!=20||close_owned(file,1)!=0)return 0;
    /* Preserve the original writer's address-test bug even though its actual
     * endian-probe body just returned FALSE. */
    if(header.value.Characteristics!=(IMAGE_FILE_BYTES_REVERSED_LO|IMAGE_FILE_BYTES_REVERSED_HI)||
       header.before!=0x11223344||header.after!=0x55667788)return 0;
    memset(&readback.value,0xa5,sizeof(readback.value));
    file=open_owned("header.obj",0x8000,0);
    if(file<0||read_header(file,&readback.value)!=20||close_owned(file,1)!=0)return 0;
    if(memcmp(&header,&readback,sizeof(header))||!physical_header())return 0;
    report[63]=header.value.Characteristics;floating_state(report+64);
    for(i=0;i<sizeof(header.value);++i)((unsigned char *)(report+80))[i]=((unsigned char *)&header.value)[i];
    return flush_streams(NULL)==0&&write_report();
}
#ifdef RECON_ORIGINAL_DIRECT
__declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *context)
{
    original_context=context;
    if(!context||context->abi_version!=RECON_ORIGINAL_PROBE_VERSION||context->struct_size!=sizeof(*context)||context->marker_rva!=0x16b40)
        recon_original_finish(context,0);
    if(!prepare_catalog())recon_original_finish(context,0);
    recon_original_finish(context,workflow());
}
#else
int run_coff_integer_helpers(void){return workflow();}
#endif
