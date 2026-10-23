#define RECON_SHIM_IMPL
#include <assert.h>
#include <recon.h>
int main(int argc,char **argv)
{
    HANDLE file,mapping;unsigned char *full,*implicit;DWORD length=0x80000,access=FILE_MAP_ALL_ACCESS,high=0,low=0;
    void *base=NULL;unsigned n;unsigned char before[108],after[108];char bytes[4];DWORD got;
    file=recon_CreateFileA("mapped.dat",GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);assert(file!=INVALID_HANDLE_VALUE);
    mapping=recon_CreateFileMappingA(file,NULL,PAGE_READWRITE,0,length,NULL);assert(mapping);
    if(argc>1){
        if(!strcmp(argv[1],"short"))--length;
        else if(!strcmp(argv[1],"large"))++length;
        else if(!strcmp(argv[1],"base"))base=(void *)0x30000000;
        else if(!strcmp(argv[1],"high"))high=1;
        else if(!strcmp(argv[1],"low"))low=4096;
        else if(!strcmp(argv[1],"foreign"))mapping=(HANDLE)123;
        else if(!strcmp(argv[1],"access"))access=0x80000000;
        else return 9;
    }
    errno=2;recon_doserrno=2;recon_SetLastError(123);
    memset(before,0,108);memset(after,0,108);
    __asm__ __volatile__("fld1\n\tfnsave %0\n\tfrstor %0":"=m"(before));
    full=recon_MapViewOfFileEx(mapping,access,high,low,length,base);
    __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(after));
    assert(!memcmp(before,after,108));
    __asm__ __volatile__("fstp %%st(0)":::"st");
    if(argc>1){assert(!full && recon_GetLastError()==ERROR_NOT_SUPPORTED);return 7;}
    assert(full && errno==2 && recon_doserrno==2 && recon_GetLastError()==123);
    implicit=recon_MapViewOfFileEx(mapping,FILE_MAP_ALL_ACCESS,0,0,0,NULL);assert(implicit && implicit!=full);
    for(n=0;n<length;++n)assert(!full[n] && !implicit[n]);
    assert(recon_CloseHandle(mapping));
    full[0]=0x12;full[length-1]=0x34;assert(implicit[0]==0x12 && implicit[length-1]==0x34);
    assert(recon_FlushViewOfFile(full,0));assert(recon_UnmapViewOfFile(full));
    assert(implicit[0]==0x12 && implicit[length-1]==0x34);assert(recon_UnmapViewOfFile(implicit));
    assert(recon_SetFilePointer(file,0,NULL,FILE_BEGIN)==0);
    assert(recon_ReadFile(file,bytes,4,&got,NULL) && got==4 && bytes[0]==0x12);
    assert(recon_CloseHandle(file));
    puts("full explicit/implicit views, zero extent, closed mapping lease and persisted data pass");return 0;
}
