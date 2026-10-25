#define RECON_SHIM_IMPL
#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <recon.h>
typedef PIMAGE_NT_HEADERS (WINAPI *Checksum)(LPVOID,DWORD,LPDWORD,LPDWORD);
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
int main(int argc,char **argv)
{
    static const unsigned errors[][3]={{0,0,0},{2,2,0},{111,222,0x12345678}};
    HANDLE file,mapping;unsigned char *image,*snapshot,before[108],after[108];
    DWORD length,expected_old,expected_new,nt,capacity,*output,*old_sum,*new_sum;
    void *base;Checksum checksum;unsigned row,n;int status;pid_t child;
    const char *mode;int positive;
    assert(argc==7);mode=argv[6];
    length=strtoul(argv[2],NULL,0);expected_old=strtoul(argv[3],NULL,0);
    expected_new=strtoul(argv[4],NULL,0);nt=strtoul(argv[5],NULL,0);
    file=recon_CreateFileA(argv[1],GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    assert(file!=INVALID_HANDLE_VALUE);capacity=recon_GetFileSize(file,NULL);
    mapping=recon_CreateFileMappingA(file,NULL,PAGE_READONLY,0,0,NULL);assert(mapping);
    image=recon_MapViewOfFile(mapping,FILE_MAP_READ,0,0,0);assert(image);
    snapshot=malloc(capacity);assert(snapshot);memcpy(snapshot,image,capacity);
    output=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    assert(output!=MAP_FAILED);memset(output,0xa5,4096);
    old_sum=output+4;new_sum=output+8;base=image;
    checksum=(Checksum)recon_GetProcAddress(recon_LoadLibraryA("imagehlp"),"CheckSumMappedFile");assert(checksum);
    positive=!*mode || !strcmp(mode,"live") || !strcmp(mode,"adjacent");
    if(!strcmp(mode,"adjacent"))new_sum=old_sum+1;
    if(!positive) {
        if(!strcmp(mode,"null-base"))base=NULL;
        if(!strcmp(mode,"interior"))base=image+4;
        if(!strcmp(mode,"unmapped"))assert(recon_UnmapViewOfFile(image));
        if(!strcmp(mode,"short"))length=63;
        if(!strcmp(mode,"long"))length=capacity+1;
        if(!strcmp(mode,"null-output"))old_sum=NULL;
        if(!strcmp(mode,"image-output"))old_sum=(DWORD *)(image+nt+0x58);
        if(!strcmp(mode,"tail-output"))old_sum=(DWORD *)(image+((length+3)&~3U));
        if(!strcmp(mode,"same-output"))new_sum=old_sum;
        if(!strcmp(mode,"partial-output"))new_sum=(DWORD *)((char *)old_sum+2);
        if(!strcmp(mode,"wrapped-output"))old_sum=(DWORD *)0xfffffffcU;
        child=fork();assert(child>=0);
        if(!child){checksum(base,length,old_sum,new_sum);_exit(42);}
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
        for(n=0;n<4096;++n)assert(((unsigned char *)output)[n]==0xa5);
        if(strcmp(mode,"unmapped"))assert(!memcmp(image,snapshot,capacity) && recon_UnmapViewOfFile(image));
    } else {
        if(!strcmp(mode,"live"))__asm__ __volatile__("fld1\n\tfldpi":::"st");
        for(row=0;row<3;++row) {
            memset(output,0xa5,4096);
            errno=errors[row][0];recon_doserrno=errors[row][1];recon_SetLastError(errors[row][2]);
            fp(before);assert(checksum(image,length,old_sum,new_sum)==(PIMAGE_NT_HEADERS)(image+nt));fp(after);
            assert(*old_sum==expected_old && *new_sum==expected_new && !memcmp(before,after,108));
            assert(errno==(int)errors[row][0] && recon_doserrno==errors[row][1] && recon_GetLastError()==errors[row][2]);
            assert(!memcmp(image,snapshot,capacity));
            for(n=0;n<1024;++n)if(output+n!=old_sum && output+n!=new_sum)assert(output[n]==0xa5a5a5a5);
        }
        if(!strcmp(mode,"live"))__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
        assert(recon_UnmapViewOfFile(image));
    }
    assert(!munmap(output,4096));free(snapshot);
    assert(recon_CloseHandle(mapping) && recon_CloseHandle(file));return 0;
}
