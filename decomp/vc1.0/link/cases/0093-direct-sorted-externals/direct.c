/* External typed table caller; not a public LINK invocation or graph vertex. */
#include <string.h>
#include "../../src/link-pass.h"
#ifdef RECON_ORIGINAL_DIRECT
#include <recon-original-direct.h>
#else
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/syscall.h>
#endif
#include <recon.h>
#ifdef RECON_ORIGINAL_DIRECT
static ReconOriginalProbe *original_context;
#define AT(rva) RECON_ORIGINAL_ADDRESS(original_context,rva)
#define new_image (*(void (__cdecl *)(Image **))AT(0x7000))
#define lookup (*(External *(__cdecl *)(SymbolTable *,unsigned short,const char *,unsigned char *,unsigned char *))AT(0x15cd0))
#define define (*(void (__cdecl *)(External *,unsigned char,SymbolTable *))AT(0x15e20))
#define invalidate (*(void (__cdecl *)(SymbolTable *))AT(0x164b0))
#define print_sorted (*(void (__cdecl *)(SymbolTable *,ImageOptions *))AT(0x12f70))
#define unresolved (*(unsigned long *)AT(0x4c418))
#else
#define new_image create_image
#define lookup lookup_external
#define define set_external_defined
#define invalidate invalidate_symbol_sorts
#define print_sorted print_defined_undefined_externals
#define unresolved undefined_symbols
int *external_test_errno;
#endif
Image *external_test_image;
External *external_test_symbols[12];
unsigned long external_test_phase;
static unsigned char input[512],unchanged[512],oracle[2048];
static unsigned long report[512],used,failed;
static unsigned char snapshots[12][sizeof(External)];
static int read_exact(const char *name,void *data,unsigned long size)
{
#ifdef RECON_ORIGINAL_DIRECT
    DWORD n;unsigned char extra;
    HANDLE file=CreateFileA(name,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return 0;
    if(!ReadFile(file,data,size,&n,NULL)||n!=size||!ReadFile(file,&extra,1,&n,NULL)||n||!CloseHandle(file))return 0;
#else
    unsigned char extra;int fd=syscall(SYS_openat,AT_FDCWD,name,O_RDONLY,0);
    if(fd<0||syscall(SYS_read,fd,data,size)!=size||syscall(SYS_read,fd,&extra,1)||syscall(SYS_close,fd))return 0;
#endif
    return 1;
}
static void append(const void *data,unsigned long size)
{
    if(used+size>sizeof(report)){++failed;return;}
    memcpy((unsigned char *)report+used,data,size);used+=size;
}
static void diagnostic(unsigned count)
{
    unsigned long i,row[4],before=unresolved;
    ImageOptions options=external_test_image->options;
    for(i=0;i<count;++i)memcpy(snapshots[i],external_test_symbols[i],sizeof(External));
    print_sorted(external_test_image->symbols,&external_test_image->options);
    if(unresolved!=before||memcmp(&options,&external_test_image->options,sizeof(options)))++failed;
    row[0]=external_test_phase;row[1]=count;row[2]=before;row[3]=unresolved;append(row,16);
    for(i=0;i<count;++i){
        External *symbol=external_test_symbols[i];
        if(memcmp(snapshots[i],symbol,sizeof(External)))++failed;
        row[0]=i;row[1]=symbol->flags;row[2]=symbol->symbol.Value;row[3]=symbol->symbol.N.Name.Short!=0;
        append(row,16);append(input+32+i*40+8,32);
    }
}
static int workflow(void)
{
    unsigned i;unsigned char created,imports;
#ifndef RECON_ORIGINAL_DIRECT
    external_test_errno=&errno;
#endif
    if(!read_exact("names.bin",input,sizeof(input))||((unsigned long *)input)[0]!=0x31545845||
       ((unsigned long *)input)[1]!=1||((unsigned long *)input)[2]!=12||((unsigned long *)input)[3]!=40)return 0;
    memcpy(unchanged,input,sizeof(input));memset(report,0,sizeof(report));used=32;failed=0;
    external_test_phase=0;new_image(&external_test_image);
    external_test_phase=1;diagnostic(0);
    /* Real API invalidates the empty cached sort before insertion. */
    invalidate(external_test_image->symbols);
    for(i=0;i<12;++i){
        unsigned long *row=(unsigned long *)(input+32+i*40);
        char *name=(char *)(row+2);
        if(!memchr(name,0,32)||!row[1])return 0;
        external_test_phase=2;
        external_test_symbols[i]=lookup(external_test_image->symbols,strlen(name)<=8,name,&created,&imports);
        if(!external_test_symbols[i]||!created)return 0;
        external_test_symbols[i]->symbol.Value=row[1];
        if(row[0])define(external_test_symbols[i],1,external_test_image->symbols);
    }
    external_test_phase=3;diagnostic(12);
    for(i=0;i<12;++i){external_test_phase=4;define(external_test_symbols[i],!(i%2),external_test_image->symbols);}
    external_test_phase=5;diagnostic(12);
    if(memcmp(input,unchanged,sizeof(input)))++failed;
    report[0]=0x31525845;report[1]=1;report[2]=3;report[3]=failed;report[4]=used;
#ifdef RECON_ORIGINAL_DIRECT
    if(((int (__cdecl *)(void *))AT(0x40e00))(NULL)||
       !recon_original_emit(original_context,".external-results.bin",report,sizeof(report)))return 0;
#else
    if(recon_fflush(NULL)||!read_exact(".external-results.bin",oracle,sizeof(oracle))||memcmp(report,oracle,sizeof(report)))return 0;
#endif
    return failed==0;
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
int run_sorted_externals(void){return workflow();}
#endif
