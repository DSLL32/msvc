#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <recon.h>
typedef unsigned short *(WINAPI *ProcessRelocations)(void *,unsigned long,unsigned short *,long);
static unsigned char page[8192];
static void relocation(ProcessRelocations process,long delta,unsigned count)
{
    unsigned short records[3]; DWORD left=0xf0000000UL,right=0xfffff000UL; unsigned short *end; unsigned n;
    memset(page,0xa5,sizeof(page)); memcpy(page+1,&left,4); memcpy(page+9,&right,4);
    records[0]=0x3001; records[1]=0; records[2]=0x3009;
#ifdef RECON_NATIVE_TEST
    if(count==99) { records[0]=0x1001; count=1; }
#endif
    errno=123; SetLastError(456); end=process(page,count,records,delta);
    printf("reloc %ld %u %ld %d %lu ",delta,count,(long)((char *)end-(char *)records),errno,GetLastError());
    for(n=0;n<16;++n) printf("%02x",page[n]);
    printf("\n");
}
int main(int argc,char **argv)
{
    HMODULE module; ProcessRelocations process; DWORD result; char output[260],*basename; unsigned n;
    module=GetModuleHandleA("ntdll");
    process=(ProcessRelocations)GetProcAddress(module,"LdrProcessRelocationBlock");
    if(!module || !process) return 3;
#ifdef RECON_NATIVE_TEST
    if(argc>1 && !strcmp(argv[1],"unsupported")) relocation(process,1,99);
#else
    (void)argc; (void)argv;
#endif
    relocation(process,0x20000000,3); relocation(process,-0x1000,3); relocation(process,1,0);
    memset(output,0xa5,sizeof(output)); basename=NULL;
    errno=123; SetLastError(456);
    result=GetFullPathNameA("client.exe",sizeof(output),output,&basename);
    printf("fullpath %lu %ld %d %lu ",result,(long)(basename-output),errno,GetLastError());
    for(n=0;n<sizeof(output);++n) printf("%02x",(unsigned char)output[n]);
    printf("\n");
    return 0;
}
