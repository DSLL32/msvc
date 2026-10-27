#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <windows.h>
#include <imagehlp.h>
#include <recon.h>
typedef BOOL (WINAPI *BindType)(DWORD,LPSTR,LPSTR,LPSTR,PIMAGEHLP_STATUS_ROUTINE);
typedef BOOL (WINAPI *RebaseType)(LPSTR,LPSTR,BOOL,BOOL,BOOL,ULONG,ULONG *,ULONG *,ULONG *,ULONG *,ULONG);
typedef BOOL (WINAPI *CallbackType)(IMAGEHLP_STATUS_REASON,LPSTR,LPSTR,ULONG,ULONG);
typedef PIMAGE_NT_HEADERS (WINAPI *ChecksumType)(LPVOID,DWORD,LPDWORD,LPDWORD);
typedef char BindSignature[__builtin_types_compatible_p(__typeof__(&BindImageEx),BindType)?1:-1];
typedef char RebaseSignature[__builtin_types_compatible_p(__typeof__(&ReBaseImage),RebaseType)?1:-1];
typedef char CallbackSignature[__builtin_types_compatible_p(PIMAGEHLP_STATUS_ROUTINE,CallbackType)?1:-1];
typedef char ChecksumSignature[__builtin_types_compatible_p(__typeof__(&CheckSumMappedFile),ChecksumType)?1:-1];
typedef DWORD (WINAPI *MapChecksumType)(LPSTR,LPDWORD,LPDWORD);
typedef char MapChecksumSignature[__builtin_types_compatible_p(__typeof__(&MapFileAndCheckSumA),MapChecksumType)?1:-1];
typedef char EnumWidth[sizeof(IMAGEHLP_STATUS_REASON)==4?1:-1];
typedef char UlongWidth[sizeof(ULONG)==4?1:-1];
static volatile ULONG *callback_count;
static BOOL WINAPI callback(IMAGEHLP_STATUS_REASON reason,LPSTR image,LPSTR dll,ULONG address,ULONG parameter)
{
    (void)reason;(void)image;(void)dll;(void)address;(void)parameter;
    ++*callback_count;return TRUE;
}
int main(void)
{
    unsigned char *owned;unsigned mode,n;pid_t child;int status;
    BindType bind=BindImageEx;RebaseType rebase=ReBaseImage;PIMAGEHLP_STATUS_ROUTINE typed=callback;
    assert(BindOutOfMemory==0 && BindSymbolsNotUpdated==13 && IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT==11);
    assert(IMAGE_FILE_LARGE_ADDRESS_AWARE==0x20 && IMAGE_SCN_MEM_SHARED==0x10000000);
    assert(BIND_NO_BOUND_IMPORTS==1 && BIND_NO_UPDATE==2 && BIND_ALL_IMAGES==4 && IMAGE_SEPARATION==65536);
    owned=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);assert(owned!=MAP_FAILED);
    memset(owned,0xa5,4096);callback_count=(ULONG *)owned;
    for(mode=0;mode<9;++mode) {
        child=fork();assert(child>=0);
        if(!child) {
            if(mode==0)bind(1,NULL,NULL,NULL,NULL);
            else if(mode==1)bind(0xffffffff,(char *)1,(char *)1,(char *)1,(PIMAGEHLP_STATUS_ROUTINE)1);
            else if(mode==2)BindImageEx(BIND_NO_BOUND_IMPORTS,(char *)owned,NULL,NULL,typed);
            else if(mode==3)rebase(NULL,NULL,1,1,0,0,NULL,NULL,NULL,NULL,0);
            else if(mode==4)rebase((char *)1,(char *)1,1,1,1,0xffffffff,(ULONG *)1,(ULONG *)1,(ULONG *)1,(ULONG *)1,0xffffffff);
            else if(mode==5)ReBaseImage((char *)owned,NULL,1,1,0,0,(ULONG *)(owned+16),(ULONG *)(owned+20),(ULONG *)(owned+24),(ULONG *)(owned+28),123);
            else if(mode==6)MapFileAndCheckSumA(NULL,NULL,NULL);
            else if(mode==7)MapFileAndCheckSumA((char *)1,(DWORD *)1,(DWORD *)1);
            else MapFileAndCheckSumA((char *)owned,(DWORD *)(owned+16),(DWORD *)(owned+20));
            _exit(42);
        }
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
        for(n=0;n<4096;++n)assert(owned[n]==0xa5);
    }
    assert(!munmap(owned,4096));return 0;
}
