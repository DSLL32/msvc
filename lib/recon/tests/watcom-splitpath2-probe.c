#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <recon.h>
#ifdef RECON_CRT_PROFILE_WATCOM_100A
static void (__cdecl *split)(const char *,char *,char **,char **,char **,char **)=_splitpath2;
#else
#ifdef _splitpath2
#error Watcom splitpath2 alias leaked
#endif
static void (__cdecl *split)(const char *,char *,char **,char **,char **,char **)=recon_watcom_splitpath2;
#endif
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
int main(int argc,char **argv)
{
    static const char *paths[]={"transform.obj","transform.asm","plain","a.b.c",".hidden","","tail.",NULL};
    static const char *names[]={"transform","transform","plain","a.b","","","tail",NULL};
    static const char *extensions[]={".obj",".asm","",".c",".hidden","",".",".obj"};
    static const unsigned errors[][3]={{0,0,0},{2,2,0},{111,222,333}};
    unsigned path,mask,row,n,used;char longest[260],stem[256],storage[272],*result[4],*buffer=storage+4;
    const char *parts[4];unsigned char before[108],after[108];
    memset(stem,'Q',255);stem[255]=0;memcpy(longest,stem,255);strcpy(longest+255,".obj");paths[7]=longest;names[7]=stem;
    if(argc>1 && !strcmp(argv[1],"denials")) {
        char *shared=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
        char snapshot[4096],longname[261],high[]={ 'x',(char)0x80,0};int status;pid_t child;
        assert(shared!=MAP_FAILED);memset(longname,'A',260);longname[260]=0;
        for(n=0;n<10;++n) {
            memset(shared,0xa5,4096);strcpy(shared,"a.obj");memcpy(snapshot,shared,4096);
            child=fork();assert(child>=0);
            if(!child) {
                const char *input=n==0?"C:a.obj":n==1?"dir/a.obj":n==2?longname:n==3?high:n==4?shared:"a.obj";
                char *out=n==4?shared:n==6?(char *)0xfffffffcU:n==7?NULL:shared+32;
                char **d=n==5?(char **)(shared+32):n==9?(char **)(shared+513):(char **)(shared+512);
                char **dir=n==8?d:(char **)(shared+516);
                split(input,out,d,dir,(char **)(shared+520),(char **)(shared+524));_exit(42);
            }
            assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
            assert(!memcmp(shared,snapshot,4096));
        }
        assert(!munmap(shared,4096));return 0;
    }
    if(argc>1)__asm__ __volatile__("fld1\n\tfldpi":::"st");
    for(path=0;path<8;++path)for(mask=0;mask<16;++mask)for(row=0;row<3;++row) {
        parts[0]=parts[1]="";parts[2]=names[path];parts[3]=extensions[path];
        memset(storage,0xa5,sizeof(storage));for(n=0;n<4;++n)result[n]=(char *)1;
        errno=errors[row][0];recon_doserrno=errors[row][1];recon_SetLastError(errors[row][2]);
        fp(before);split(paths[path],buffer,mask&1?result:NULL,mask&2?result+1:NULL,mask&4?result+2:NULL,mask&8?result+3:NULL);fp(after);
        assert(!memcmp(before,after,108) && errno==(int)errors[row][0] && recon_doserrno==errors[row][1] && recon_GetLastError()==errors[row][2]);
        used=0;for(n=0;n<4;++n)if(mask&(1u<<n)) {
            assert(result[n]==buffer+used && !strcmp(result[n],parts[n]));used+=strlen(parts[n])+1;
        }else assert(result[n]==(char *)1);
        for(n=0;n<4;++n)assert((unsigned char)storage[n]==0xa5);
        for(n=used+4;n<sizeof(storage);++n)assert((unsigned char)storage[n]==0xa5);
    }
    if(argc>1)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    return 0;
}
