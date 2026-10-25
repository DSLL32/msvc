#include <windows.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <recon.h>
typedef DWORD (WINAPI *TempPath)(DWORD,LPSTR);
typedef UINT (WINAPI *TempFile)(LPCSTR,LPCSTR,UINT,LPSTR);
typedef BOOL (WINAPI *Environment)(LPCSTR,LPCSTR);
typedef char PathType[__builtin_types_compatible_p(__typeof__(&GetTempPathA),TempPath)?1:-1];
typedef char FileType[__builtin_types_compatible_p(__typeof__(&GetTempFileNameA),TempFile)?1:-1];
typedef char EnvironmentType[__builtin_types_compatible_p(__typeof__(&SetEnvironmentVariableA),Environment)?1:-1];
int main(void)
{
    char *memory=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    unsigned api,row,n;pid_t child;int status;assert(memory!=MAP_FAILED);memset(memory,0xa5,4096);
    for(api=0;api<3;++api)for(row=0;row<3;++row) {
        child=fork();assert(child>=0);
        if(!child) {
            char *output=row==0?NULL:row==1?memory:(char *)1;
            const char *input=row==0?NULL:row==1?"TEMP":(const char *)1;
            if(api==0)GetTempPathA(row==0?0:row==1?MAX_PATH:0xffffffffU,output);
            if(api==1)GetTempFileNameA(input,input,row,output);
            if(api==2)SetEnvironmentVariableA(input,output);
            _exit(42);
        }
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
        for(n=0;n<4096;++n)assert((unsigned char)memory[n]==0xa5);
    }
    assert(!munmap(memory,4096));return 0;
}
