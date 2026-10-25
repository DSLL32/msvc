#define RECON_SHIM_IMPL
#include <assert.h>
#include <sys/mman.h>
#include <recon.h>
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
int main(int argc,char **argv)
{
    static const unsigned errors[][3]={{0,0,0},{2,2,0},{111,222,333}};
    static const char *inputs[]={"T:\\run-case\\WLINK.EXE ?","program\t \v\f\r\n@archive.rsp  ","program \"a b\" /q","program","program   ","program ?","\"program\" \"?\"","\"some program\" ?"};
    static const char *tails[]={"?","@archive.rsp  ","\"a b\" /q","","","?","\"?\"","program\" ?"};
    char raw[128],guarded[160],*output=guarded+8,*text=raw;size_t bytes;unsigned row,n,index=0;
    unsigned char before[108],after[108];int live=argc>2;
    LPSTR (WINAPI *provider)(void)=recon_GetCommandLineA;
    char *(__cdecl *command)(char *)=recon_watcom_getcmd;
    assert(argc>1);memset(guarded,0xa5,sizeof(guarded));
    if(argv[1][0]>='0' && argv[1][0]<='7')index=argv[1][0]-'0';
    strcpy(raw,inputs[index]);bytes=strlen(raw)+1;
    if(!strcmp(argv[1],"unbound")){provider();return 42;}
    if(!strcmp(argv[1],"unboundcmd")){command(output);return 42;}
    if(!strcmp(argv[1],"nullbind")){recon_bind_command_line(NULL,bytes);return 42;}
    if(!strcmp(argv[1],"zero")){recon_bind_command_line(raw,0);return 42;}
    if(!strcmp(argv[1],"large")){recon_bind_command_line((char *)1,32768);return 42;}
    if(!strcmp(argv[1],"wrap")){recon_bind_command_line((char *)0xfffffff0U,64);return 42;}
    if(!strcmp(argv[1],"embedded"))raw[1]=0;
    if(!strcmp(argv[1],"nonascii"))raw[1]=(char)0x80;
    if(!strcmp(argv[1],"unterminated"))raw[bytes-1]='X';
    if(!strcmp(argv[1],"leading")){strcpy(raw," program ?");bytes=strlen(raw)+1;}
    if(index==5) {
        char *pages=mmap(NULL,8192,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
        assert(pages!=MAP_FAILED && !mprotect(pages+4096,4096,PROT_NONE));
        text=pages+4096-bytes;memcpy(text,raw,bytes);
    }
    if(live)__asm__ __volatile__("fld1\n\tfldpi":::"st");
    errno=111;recon_doserrno=222;recon_SetLastError(333);fp(before);
    recon_bind_command_line(text,bytes);assert(provider()==text);fp(after);
    assert(!memcmp(before,after,108) && errno==111 && recon_doserrno==222 && recon_GetLastError()==333);
    if(!strcmp(argv[1],"rebind")){recon_bind_command_line(text,bytes);return 42;}
    if(!strcmp(argv[1],"nullout")){command(NULL);return 42;}
    if(!strcmp(argv[1],"overlap")){command(text+1);return 42;}
    if(!strcmp(argv[1],"outwrap")){command((char *)0xffffffffU);return 42;}
    if(!strcmp(argv[1],"mutated")){text[0]=(char)0x80;command(output);return 42;}
    if(!strcmp(argv[1],"missingnul")){text[bytes-1]='X';command(output);return 42;}
    if(!strcmp(argv[1],"alias")) {
        text[bytes-2]='!';assert(provider()==text && provider()[bytes-2]=='!');return 0;
    }
    for(row=0;row<3;++row) {
        memset(guarded,0xa5,sizeof(guarded));
        errno=errors[row][0];recon_doserrno=errors[row][1];recon_SetLastError(errors[row][2]);
        fp(before);assert(command(output)==output);fp(after);
        assert(!memcmp(before,after,108) && !strcmp(output,tails[index]));
        assert(errno==(int)errors[row][0] && recon_doserrno==errors[row][1] && recon_GetLastError()==errors[row][2]);
        for(n=0;n<8;++n)assert((unsigned char)guarded[n]==0xa5);
        for(n=8+strlen(tails[index])+1;n<sizeof(guarded);++n)assert((unsigned char)guarded[n]==0xa5);
        assert(!memcmp(text,raw,bytes));
    }
    if(live)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    return 0;
}
