#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <recon.h>
typedef PIMAGE_NT_HEADERS (WINAPI *Checksum)(LPVOID,DWORD,LPDWORD,LPDWORD);
typedef char Signature[__builtin_types_compatible_p(__typeof__(&CheckSumMappedFile),Checksum)?1:-1];
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
int main(int argc,char **argv)
{
    HMODULE module;Checksum checksum;unsigned char before[108],after[108];
    int live=argc>1 && !strcmp(argv[1],"live");
    if(argc>1 && !strcmp(argv[1],"preload")) {
        GetProcAddress((HMODULE)4,"CheckSumMappedFile");return 42;
    }
    if(live)__asm__ __volatile__("fld1\n\tfldpi":::"st");
    errno=111;recon_doserrno=222;SetLastError(333);fp(before);
    module=LoadLibraryA("imagehlp");assert(module);
    checksum=(Checksum)GetProcAddress(module,"CheckSumMappedFile");
    assert(checksum==CheckSumMappedFile && LoadLibraryA("IMAGEHLP.DLL")==module);
    assert((Checksum)GetProcAddress(module,"CheckSumMappedFile")==checksum);
    fp(after);assert(!memcmp(before,after,108));
    assert(errno==111 && recon_doserrno==222 && GetLastError()==333);
    if(argc>1 && !live) {
        if(!strcmp(argv[1],"unknown")){GetProcAddress(module,"UnknownExport");return 42;}
        if(!strcmp(argv[1],"ordinal")){GetProcAddress(module,(LPCSTR)1);return 42;}
        if(!strcmp(argv[1],"null-name")){GetProcAddress(module,NULL);return 42;}
        if(!strcmp(argv[1],"free")){FreeLibrary(module);return 42;}
        if(!strcmp(argv[1],"call")){checksum(NULL,0,NULL,NULL);return 42;}
        if(!strcmp(argv[1],"invalid")) {
            checksum((void *)1,0xffffffff,(DWORD *)1,(DWORD *)1);return 42;
        }
        if(!strcmp(argv[1],"untouched")) {
            unsigned char *shared=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
            pid_t child;int status;unsigned n;
            assert(shared!=MAP_FAILED);memset(shared,0xa5,4096);
            child=fork();assert(child>=0);
            if(!child){checksum(shared,4096,(DWORD *)(shared+4),(DWORD *)(shared+8));_exit(42);}
            assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
            for(n=0;n<4096;++n)assert(shared[n]==0xa5);
            assert(!munmap(shared,4096));return 0;
        }
        return 43;
    }
    if(live)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    return 0;
}
