#define _GNU_SOURCE
#define RECON_SHIM_IMPL
#include <recon.h>
#include <assert.h>
#include <unistd.h>
int main(int argc,char **argv,char **env)
{
    ReconNativeEntry *entry;char **reverse;char *empty[]={NULL};unsigned n,count;char guarded[144];
    unsigned short cw,sw,cw2,sw2;
    int prepared=strcmp(argv[1],"legacy") && strcmp(argv[1],"unprepared");
    if(!strcmp(argv[1],"legacy")) {
        assert(!recon_crt_initenv);recon_crt_bind_arguments(0,empty,env);assert(!recon_crt_initenv);return 0;
    }
    if(prepared) {
        entry=recon_native_entry_prepare(argc,argv,env);
        assert(!recon_crt_arguments_bound && !recon_crt_initenv);
        assert(entry->argc==argc && entry->argv!=argv && entry->environment!=env);
        for(count=0;entry->environment[count];++count){}
        reverse=calloc(count+1,sizeof(*reverse));assert(reverse);
        for(n=0;n<count;++n)reverse[n]=entry->environment[count-1-n];
        recon_crt_bind_arguments(0,empty,reverse);
        assert(recon_crt_argc==0 && recon_crt_argv==empty && recon_crt_environment==reverse && recon_crt_initenv==reverse);
        assert(entry->argc==argc && entry->argv[argc]==NULL && entry->environment[count]==NULL);
        assert(!strcmp(recon_native_entry_spawn_environment()[0],env[0]));
        assert(!strcmp(entry->environment[0],env[0]));
        if(!strcmp(argv[1],"rebind")) {recon_crt_bind_arguments(argc,entry->argv,reverse);return 1;}
        if(!strcmp(argv[1],"prepare"))return 0;
    }
    memset(guarded,0xa5,sizeof(guarded));
    if(!strcmp(argv[1],"fake"))recon_osfile[1]=0xc1;
    errno=E2BIG;recon_doserrno=73;recon_SetLastError(91);
    __asm__ volatile("fnstcw %0;fnstsw %1":"=m"(cw),"=m"(sw));
    assert(!recon_watcom_setvbuf(&recon_iob[1],guarded+8,0x0200,!strcmp(argv[1],"wide")?256:128));
    __asm__ volatile("fnstcw %0;fnstsw %1":"=m"(cw2),"=m"(sw2));
    assert(cw==cw2 && sw==sw2);
    assert(errno==E2BIG && recon_doserrno==73 && recon_GetLastError()==91);
    assert(!recon_fputs("tty\n",&recon_iob[1]));
    for(n=0;n<8;++n)assert((unsigned char)guarded[n]==0xa5 && (unsigned char)guarded[136+n]==0xa5);
    return 0;
}
