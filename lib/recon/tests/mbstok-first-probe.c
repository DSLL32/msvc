#define RECON_SHIM_IMPL
#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <recon.h>
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
int main(int argc,char **argv)
{
    const char *mode=argc>1?argv[1]:"normal";unsigned char storage[4128],expected[4128],*text=storage+16,*result;
    const unsigned char *delimiters=(const unsigned char *)" \t";unsigned char before[108],after[108];
    unsigned n,row,pass;static const char *inputs[]={"LIBRARY hello"," \tLIBRARY\thello","LIBRARY"," \t ",""};
    static const int starts[]={0,2,0,-1,-1},ends[]={7,9,-1,-1,-1};
    unsigned char *(__cdecl *tokenize)(unsigned char *,const unsigned char *)=recon_mbstok;
    if(strcmp(mode,"normal") && strcmp(mode,"prewrite") && strcmp(mode,"edge")){
        memset(storage,0xa5,sizeof(storage));strcpy((char *)text,"LIBRARY hello");
        if(!strcmp(mode,"null"))text=NULL;
        if(!strcmp(mode,"low"))text=(unsigned char *)1;
        if(!strcmp(mode,"input-wrap"))text=(unsigned char *)0xfffffff0U;
        if(!strcmp(mode,"null-delimiters"))delimiters=NULL;
        if(!strcmp(mode,"low-delimiters"))delimiters=(const unsigned char *)1;
        if(!strcmp(mode,"delimiter-wrap"))delimiters=(const unsigned char *)0xffffffffU;
        if(!strcmp(mode,"other-delimiters"))delimiters=(const unsigned char *)",";
        if(!strcmp(mode,"overlap")){text[0]=' ';text[1]='\t';text[2]=0;delimiters=text;}
        if(!strcmp(mode,"high"))text[10]=128;
        if(!strcmp(mode,"unterminated"))memset(text,'A',4096);
        if(!strcmp(mode,"mb"))recon_mb_codepage=1252;
        if(!strcmp(mode,"gate"))recon_mb_codepage_gate=1;
        if(!strcmp(mode,"width"))recon_mb_cur_max=2;
        if(!strcmp(mode,"lc"))recon_lc_codepage=1252;
        if(!strcmp(mode,"handle"))recon_lc_handle[0]=1033;
        tokenize(text,delimiters);return 42;
    }
#ifndef RECON_CRT_PROFILE_MSVCRT_510
    tokenize(text,delimiters);return 42;
#else
    if(!strcmp(mode,"prewrite")){
        unsigned char *shared=(unsigned char *)mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);pid_t child;int status;
        assert(shared!=MAP_FAILED);strcpy((char *)shared,"LIBRARY hello");shared[10]=128;child=fork();assert(child>=0);
        if(!child){tokenize(shared,delimiters);_exit(42);}
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125 && shared[7]==' ');assert(!munmap(shared,4096));return 0;
    }
    if(!strcmp(mode,"edge")){
        unsigned char *memory=(unsigned char *)mmap(NULL,8192,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
        assert(memory!=MAP_FAILED && !mprotect(memory+4096,4096,PROT_NONE));memset(memory,'A',4095);memory[4095]=0;
        assert(tokenize(memory,delimiters)==memory && memory[4095]==0);memory[4094]='\t';
        assert(tokenize(memory,delimiters)==memory && !memory[4094] && !memory[4095]);assert(!munmap(memory,8192));return 0;
    }
    for(pass=0;pass<2;++pass){if(pass)__asm__ __volatile__("fld1\n\tfldpi":::"st");
        for(row=0;row<5;++row){memset(storage,0xa5,sizeof(storage));strcpy((char *)text,inputs[row]);memcpy(expected,storage,sizeof(storage));
            if(ends[row]>=0)expected[16+ends[row]]=0;
            errno=111+row;recon_doserrno=222+row;recon_SetLastError(333+row);fp(before);result=tokenize(text,delimiters);fp(after);
            assert(result==(starts[row]<0?NULL:text+starts[row]) && !memcmp(storage,expected,sizeof(storage)));
            assert(errno==111+(int)row && recon_doserrno==222+row && recon_GetLastError()==333+row && !memcmp(before,after,108));
            for(n=0;n<16;++n)assert(storage[n]==0xa5 && storage[4112+n]==0xa5);
        }
        if(pass)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    }return 0;
#endif
}
