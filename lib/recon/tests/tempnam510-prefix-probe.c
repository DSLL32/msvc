#define RECON_SHIM_IMPL
#include <assert.h>
#include <recon.h>
int main(int argc,char **argv)
{
    char *args[]={"LINK.EXE",NULL},*environment[]={"TMP=.",NULL};
    const char *prefixes[]={NULL,"","x","lnk","RCX1","OTHER","abcdef","XXX. X_",NULL};
    char long_prefix[249],host[512],data[4];char *names[10],*prefix;size_t i,n;
    struct stat info;ShimFILE *stream;unsigned short cw,sw,new_cw,new_sw;
    assert(!setenv("TMP","host-invalid-dir",1));
    assert(!setenv("RECON_MKTEMP_SUFFIX","unused-invalid-suffix",1));
    recon_crt_bind_arguments(1,args,environment);
    if(argc>1) {
        prefix=long_prefix;memset(long_prefix,'A',248);long_prefix[248]=0;
        if(!strcmp(argv[1],"slash"))prefix="bad/prefix";
        else if(!strcmp(argv[1],"colon"))prefix="bad:prefix";
        else if(!strcmp(argv[1],"wild"))prefix="bad*prefix";
        else if(!strcmp(argv[1],"quote"))prefix="bad\"prefix";
        else if(!strcmp(argv[1],"nonascii"))prefix="bad\200prefix";
        else if(!strcmp(argv[1],"newline"))prefix="bad\nprefix";
        else if(!strcmp(argv[1],"low"))prefix=(char *)1;
        else if(!strcmp(argv[1],"wrap"))prefix=(char *)0xfffffffeUL;
        recon_tempnam(NULL,prefix);return 42;
    }
    memset(long_prefix,'p',247);long_prefix[247]=0;prefixes[8]=long_prefix;
    for(i=0;i<9;++i) {
        n=prefixes[i]?strlen(prefixes[i]):0;
        errno=111;recon_doserrno=222;recon_SetLastError(333);
        __asm__ __volatile__("fld1; fnstcw %0; fnstsw %1":"=m"(cw),"=m"(sw));
        names[i]=recon_tempnam(NULL,prefixes[i]);
        __asm__ __volatile__("fnstcw %0; fnstsw %1":"=m"(new_cw),"=m"(new_sw));
        assert(cw==new_cw && sw==new_sw);__asm__ __volatile__("fstp %%st(0)":::"st");
        assert(names[i] && strlen(names[i])==n+8 && !strncmp(names[i],".\\",2));
        if(n)assert(!memcmp(names[i]+2,prefixes[i],n));
        assert(errno==111 && recon_doserrno==222 && recon_GetLastError()==333);
        assert(!recon_host_path(names[i],host,sizeof(host)));
        assert(lstat(host,&info)==-1 && errno==ENOENT);
        stream=recon_fopen(names[i],"w+b");assert(stream);
        assert(recon_fwrite("abc",1,3,stream)==3 && !recon_fclose(stream));
        stream=recon_fopen(names[i],"rb");assert(stream);
        assert(recon_fread(data,1,3,stream)==3 && !memcmp(data,"abc",3));
        assert(!recon_fclose(stream));
    }
    names[9]=recon_tempnam(NULL,"lnk");assert(names[9] && strcmp(names[9],names[3]));
    assert(!recon_host_path(names[9],host,sizeof(host)) && lstat(host,&info)==-1 && errno==ENOENT);
    recon_free(names[9]);
    for(i=0;i<9;++i) {assert(!recon_unlink(names[i]));recon_free(names[i]);}
    assert(!strcmp(getenv("TMP"),"host-invalid-dir") && !strcmp(recon_getenv("TMP"),"."));
    return 0;
}
