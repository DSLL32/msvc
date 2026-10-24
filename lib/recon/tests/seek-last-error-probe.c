#define RECON_SHIM_IMPL
#include <assert.h>
#include <recon.h>

static void save_fp(unsigned char bytes[108])
{
    __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])bytes));
}
static DWORD seek(HANDLE file,LONG offset,LONG *high,DWORD origin,DWORD error)
{
    unsigned char before[108],after[108];DWORD result;
    errno=0;recon_doserrno=0;recon_SetLastError(error);
    save_fp(before);result=recon_SetFilePointer(file,offset,high,origin);save_fp(after);
    assert(!memcmp(before,after,108));
    assert(errno==0 && recon_doserrno==0);
    return result;
}
int main(int argc,char **argv)
{
    HANDLE file,mapping;unsigned char *view,before[108],after[108];
    struct stat st;LONG high;int fd;
    file=recon_CreateFileA("seek.dat",GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    assert(file!=INVALID_HANDLE_VALUE);fd=(int)(uintptr_t)file-1;
    mapping=recon_CreateFileMappingA(file,NULL,PAGE_READWRITE,0,8192,NULL);assert(mapping);
    view=recon_MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,0);assert(view);
    memcpy(view+32,"seek",4);
    if(argc>1 && !strcmp(argv[1],"live")) __asm__ __volatile__("fld1":::"st");
    assert(seek(file,4097,NULL,FILE_BEGIN,0)==4097 && recon_GetLastError()==0);
    save_fp(before);assert(!recon_SetEndOfFile(file));save_fp(after);
    assert(!memcmp(before,after,108));
    assert(errno==0 && recon_doserrno==0 && recon_GetLastError()==1224);
    assert(lseek(fd,0,SEEK_CUR)==4097 && !fstat(fd,&st) && st.st_size==8192);
    /* This is the actual incoming error from SetEndOfFile, never reseeded. */
    save_fp(before);assert(recon_SetFilePointer(file,64,NULL,FILE_BEGIN)==64);save_fp(after);
    assert(!memcmp(before,after,108));
    assert(errno==0 && recon_doserrno==0 && recon_GetLastError()==1224);
    assert(lseek(fd,0,SEEK_CUR)==64 && !fstat(fd,&st) && st.st_size==8192);
    assert(!memcmp(view+32,"seek",4));
    /* Existing highword success/failure behavior is outside the correction. */
    high=0;assert(seek(file,64,&high,FILE_BEGIN,1224)==64 && high==0 && recon_GetLastError()==0);
    high=1;assert(seek(file,64,&high,FILE_BEGIN,1224)==0xffffffffUL && high==1 && recon_GetLastError()==ERROR_NOT_SUPPORTED);
    errno=0;recon_doserrno=0;recon_SetLastError(1224);
    save_fp(before);assert(recon_SetFilePointer(file,-1,NULL,FILE_BEGIN)==0xffffffffUL);save_fp(after);
    assert(!memcmp(before,after,108));
    assert(errno==EINVAL && recon_doserrno==0 && recon_GetLastError()==ERROR_NEGATIVE_SEEK);
    errno=111;recon_doserrno=222;recon_SetLastError(1224);
    save_fp(before);assert(recon_SetFilePointer(INVALID_HANDLE_VALUE,0,NULL,FILE_BEGIN)==0xffffffffUL);save_fp(after);
    assert(!memcmp(before,after,108));
    assert(errno==111 && recon_doserrno==222 && recon_GetLastError()==ERROR_INVALID_HANDLE);
    if(argc>1 && !strcmp(argv[1],"live")) __asm__ __volatile__("fstp %%st(0)":::"st");
    assert(recon_UnmapViewOfFile(view) && recon_CloseHandle(mapping) && recon_CloseHandle(file));
    puts("mapped failure then successful NULL-high seek preserves1224; old highword/failures/FP/file effects pass");
    return 0;
}
