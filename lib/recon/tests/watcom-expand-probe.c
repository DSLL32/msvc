#include <malloc.h>
#include <assert.h>
#include <sys/wait.h>
#include <recon.h>
#if defined(RECON_CRT_PROFILE_WATCOM_100A) && !defined(RECON_SHIM_IMPL)
static void *(__cdecl *expand)(void *,size_t)=_expand;
static void *(__cdecl *near_expand)(void *,size_t)=_nexpand;
#else
#ifdef _expand
#error Watcom _expand alias leaked
#endif
#ifdef _nexpand
#error Watcom _nexpand alias leaked
#endif
static void *(__cdecl *expand)(void *,size_t)=recon_watcom_expand;
static void *(__cdecl *near_expand)(void *,size_t)=recon_watcom_expand;
#endif
static void snapshot(unsigned char bytes[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])bytes)); }
int main(void)
{
    const size_t allocations[]={16,33,16392,65537};unsigned row,column,n,mode;
    unsigned char *memory;size_t capacity,request;pid_t child;int status;
    unsigned char before[108],after[108];
    assert(expand==recon_watcom_expand && near_expand==recon_watcom_expand);
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    __asm__ __volatile__("fld1\n\tfldpi":::"st");
    for(row=0;row<4;++row) {
        memory=(unsigned char *)malloc(allocations[row]);assert(memory);
        capacity=malloc_usable_size(memory);assert(capacity>=allocations[row]);
        for(n=0;n<allocations[row];++n)memory[n]=(unsigned char)(n*17+3);
        for(column=0;column<3;++column) {
            request=capacity+(column==0?1:column==1?4096:37);
            if(row==2 && column==2){request=16408;assert(request>capacity);}
            errno=111;*recon_errno()=211;recon_doserrno=311;SetLastError(411);
            snapshot(before);assert(!(column&1?near_expand(memory,request):expand(memory,request)));snapshot(after);
            assert(errno==111 && *recon_errno()==211 && recon_doserrno==311 && GetLastError()==411);
            assert(!memcmp(before,after,2) && !memcmp(before+4,after+4,2) &&
                   !memcmp(before+8,after+8,2) && !memcmp(before+28,after+28,20));
            assert(malloc_usable_size(memory)==capacity);
            for(n=0;n<allocations[row];++n)assert(memory[n]==(unsigned char)(n*17+3));
        }
        memory[0]=1;memory[allocations[row]-1]=2;
        assert(memory[0]==1 && memory[allocations[row]-1]==2);free(memory);
    }
    __asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
#else
    (void)allocations;(void)row;(void)column;(void)n;(void)request;(void)before;(void)after;(void)snapshot;
#endif
    memory=(unsigned char *)malloc(32);assert(memory);capacity=malloc_usable_size(memory);
    for(mode=0;mode<6;++mode) {
        child=fork();assert(child>=0);if(!child) {
            if(mode==0)expand(NULL,1);
            if(mode==1)expand((void *)1,16408);
            if(mode==2)expand((void *)(UINTPTR_MAX-3),32);
            if(mode==3)expand(memory,0);
            if(mode==4)expand(memory,capacity);
            if(mode==5)expand(memory,capacity-1);
            _exit(42);
        }
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
    }
#ifndef RECON_CRT_PROFILE_WATCOM_100A
    child=fork();assert(child>=0);if(!child){expand(memory,capacity+1);_exit(42);}
    assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
#endif
    free(memory);return 0;
}
