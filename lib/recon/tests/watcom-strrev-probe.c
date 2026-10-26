#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <recon.h>
#ifdef RECON_CRT_PROFILE_WATCOM_100A
static char *(__cdecl *reverse)(char *)=strrev;
#else
#ifdef strrev
#error Watcom strrev alias leaked
#endif
static char *(__cdecl *reverse)(char *)=recon_watcom_strrev;
#endif
static void snapshot(unsigned char bytes[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])bytes)); }
int main(void)
{
    unsigned char before[108],after[108],buffer[272],saved[272],*shared;
    unsigned length,n,row,pass,mode;pid_t child;int status;
    assert(reverse==recon_watcom_strrev);
    shared=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);assert(shared!=MAP_FAILED);
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    for(pass=0;pass<2;++pass) {
        if(pass)__asm__ __volatile__("fld1\n\tfldpi":::"st");
        for(length=0;length<=255;++length)for(row=0;row<3;++row) {
            memset(buffer,0xa5,sizeof(buffer));for(n=0;n<length;++n)buffer[7+n]=(unsigned char)(n+1);buffer[7+length]=0;
            memcpy(saved,buffer,sizeof(buffer));errno=111+row;*recon_errno()=211+row;recon_doserrno=311+row;SetLastError(411+row);
            snapshot(before);assert(reverse((char *)buffer+7)==(char *)buffer+7);snapshot(after);
            assert(errno==111+(int)row && *recon_errno()==211+(int)row && recon_doserrno==311+row && GetLastError()==411+row);
            assert(!memcmp(before,after,2) && !memcmp(before+4,after+4,2) && !memcmp(before+8,after+8,2));
            if(pass)assert(!memcmp(before+28,after+28,20));
            for(n=0;n<length;++n)assert(buffer[7+n]==saved[7+length-1-n]);
            assert(!memcmp(buffer,saved,7) && !memcmp(buffer+7+length,saved+7+length,sizeof(buffer)-7-length));
            assert(reverse((char *)buffer+7)==(char *)buffer+7 && !memcmp(buffer,saved,sizeof(buffer)));
        }
        if(pass)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    }
    {
        char *edge=mmap(NULL,8192,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0),*text;
        assert(edge!=MAP_FAILED && !mprotect(edge+4096,4096,PROT_NONE));text=edge+4096-256;
        for(n=0;n<255;++n)text[n]=(char)(n+1);text[255]=0;
        assert(reverse(text)==text && text[255]==0);for(n=0;n<255;++n)assert((unsigned char)text[n]==255-n);
        assert(!munmap(edge,8192));
    }
#else
    (void)before;(void)after;(void)buffer;(void)saved;(void)length;(void)row;(void)pass;(void)snapshot;
#endif
    for(mode=0;mode<5;++mode) {
        memset(shared,'x',4096);if(mode==3)shared[256]=0;
        child=fork();assert(child>=0);
        if(!child) {
            char *text=(char *)shared;
            if(mode==0)text=NULL;
            if(mode==1)text=(char *)1;
            if(mode==2)text=(char *)(UINTPTR_MAX-3);
            reverse(text);_exit(42);
        }
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
        for(n=0;n<4096;++n)assert(shared[n]==(mode==3 && n==256?0:'x'));
    }
#ifndef RECON_CRT_PROFILE_WATCOM_100A
    child=fork();assert(child>=0);if(!child){reverse((char[]){'a','b',0});_exit(42);}
    assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
#endif
    assert(!munmap(shared,4096));return 0;
}
