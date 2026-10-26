#define _GNU_SOURCE
#define RECON_SHIM_IMPL
#include <assert.h>
#include <sys/mman.h>
#include <stdint.h>
#include <recon.h>
int main(int argc,char **argv)
{
    const size_t length=0x40000;void *reservation,*base;unsigned char *view,*ordinary;
    HANDLE file,mapping;DWORD access=FILE_MAP_ALL_ACCESS,bytes=length,low=0;unsigned short cw,sw,after_cw,after_sw;
    reservation=mmap(NULL,length+0x10000,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    assert(reservation!=MAP_FAILED);
    base=(void *)(((uintptr_t)reservation+0xffff)&~(uintptr_t)0xffff);
    assert(!munmap(reservation,length+0x10000));
    file=recon_CreateFileA("view.dat",GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    assert(file!=INVALID_HANDLE_VALUE);
    mapping=recon_CreateFileMappingA(file,NULL,PAGE_READWRITE,0,length,NULL);assert(mapping);
    if(argc>1) {
        if(!strcmp(argv[1],"short")) --bytes;
        else if(!strcmp(argv[1],"access")) access=FILE_MAP_READ;
        else if(!strcmp(argv[1],"offset")) low=0x10000;
    }
    errno=EDOM;recon_doserrno=73;recon_SetLastError(91);
    __asm__ __volatile__("fnstcw %0; fnstsw %1":"=m"(cw),"=m"(sw));
    view=recon_MapViewOfFileEx(mapping,access,0,low,bytes,base);
    assert(view==base && errno==EDOM && recon_doserrno==73 && recon_GetLastError()==91);
    __asm__ __volatile__("fnstcw %0; fnstsw %1":"=m"(after_cw),"=m"(after_sw));
    assert(cw==after_cw && sw==after_sw);
    view[0]=0x12;view[length-1]=0x34;
    /* A live view is a real occupied range: the second request must not replace it. */
    assert(!recon_MapViewOfFileEx(mapping,access,0,0,length,base));
    assert(recon_GetLastError()==ERROR_INVALID_ADDRESS && errno==EDOM && recon_doserrno==73);
    assert(view[0]==0x12 && view[length-1]==0x34);
    assert(!recon_MapViewOfFileEx(mapping,access,0,0,length,view+1));
    assert(recon_GetLastError()==ERROR_MAPPED_ALIGNMENT && errno==EDOM && recon_doserrno==73);
    ordinary=recon_MapViewOfFileEx(mapping,access,0,0,0,NULL);assert(ordinary && ordinary!=view);
    assert(ordinary[0]==0x12 && ordinary[length-1]==0x34);
    assert(recon_CloseHandle(mapping));
    assert(recon_UnmapViewOfFile(view));
    assert(ordinary[0]==0x12 && ordinary[length-1]==0x34);
    assert(recon_FlushViewOfFile(ordinary,0));assert(recon_UnmapViewOfFile(ordinary));
    assert(recon_CloseHandle(file));
    puts("preferred exact view, no-clobber collision, alignment and normal view lifetime pass");
    return 0;
}
