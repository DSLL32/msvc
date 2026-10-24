/* Native case host only, outside CL's reconstructed graph. No child execution. */
#define RECON_SHIM_IMPL
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>

char **link_initial_environment;
WCHAR *link_initial_os_environment;
unsigned link_initial_environment_count, link_initial_os_units;
char link_response_identity[65];

static const unsigned char *cursor, *limit;
static unsigned clock_expected_calls;
static void clock_finish(void)
{
    if (recon_local_clock_call_count()!=clock_expected_calls)
        recon_unsupported("CL timing clock call inventory");
}
static unsigned read_word(void)
{
    unsigned value;
    if ((unsigned)(limit-cursor)<4)
        recon_unsupported("truncated CL LINK environment binding");
    memcpy(&value,cursor,4);cursor+=4;
    return value;
}

char **link_bind_entry(int argc,char **argv,char **envp)
{
    const char *path=getenv("CL_LINK_BINDING");
    const char *identity=getenv("CL_LINK_RESPONSE_ID");
    const char *profile=getenv("CL_ENTRY_PROFILE");
    const char *clock_count=getenv("CL_CLOCK_EXPECTED_CALLS");
    const char *clock=getenv("RECON_LOCAL_CLOCK");
    int timing=profile && !strcmp(profile,"link-timing-canonical-msvcrt-v1");
    unsigned length,i,count;
    unsigned char *bytes;
    FILE *file;
    long size;
    (void)argv;
    if (!path && !identity) {
        if (profile && !strcmp(profile,"link-object-canonical-msvcrt-v1"))
            recon_unsupported("unbound CL LINK native case");
        if (clock_count) recon_unsupported("unbound CL timing clock inventory");
        return envp;
    }
    if (!path || !identity || strlen(identity)!=64 || !profile ||
        (timing ? argc!=9 : strcmp(profile,"link-object-canonical-msvcrt-v1") || argc!=8) ||
        link_initial_environment)
        recon_unsupported("unknown CL LINK environment binding");
    if (timing) {
        if (!clock_count || !clock || !*clock ||
            (strcmp(clock_count,"1") && strcmp(clock_count,"2")) ||
            recon_local_clock_call_count()!=0)
            recon_unsupported("unknown CL timing clock contract");
        clock_expected_calls=(unsigned)(clock_count[0]-'0');
        if (atexit(clock_finish)) recon_unsupported("CL clock finish registration");
    } else if (clock_count) recon_unsupported("clock inventory outside timing case");
    for (i=0;i<64;++i)
        if (!((identity[i]>='0' && identity[i]<='9') ||
              (identity[i]>='a' && identity[i]<='f')))
            recon_unsupported("invalid CL LINK response identity");
    file=fopen(path,"rb");
    if (!file || fseek(file,0,SEEK_END) || (size=ftell(file))<76 || size>262144 ||
        fseek(file,0,SEEK_SET))
        recon_unsupported("invalid CL LINK binding file");
    bytes=malloc((unsigned)size);
    if (!bytes || fread(bytes,1,(unsigned)size,file)!=(unsigned)size || fclose(file))
        recon_unsupported("incomplete CL LINK binding file");
    cursor=bytes;limit=bytes+size;
    if (memcmp(cursor,"RCL1",4)) recon_unsupported("unknown CL LINK binding format");
    cursor+=4;
    if (read_word()!=1 || memcmp(cursor,identity,64))
        recon_unsupported("different CL LINK response identity");
    memcpy(link_response_identity,cursor,64);cursor+=64;
    count=read_word();
    if (!count || count>4096) recon_unsupported("invalid CL LINK environment count");
    link_initial_environment=calloc(count+1,sizeof(char *));
    if (!link_initial_environment) recon_unsupported("CL LINK environment allocation");
    link_initial_environment_count=count;
    for (i=0;i<count;++i) {
        length=read_word();
        if (!length || length>32767 || (unsigned)(limit-cursor)<length || memchr(cursor,0,length))
            recon_unsupported("invalid CL LINK environment string");
        link_initial_environment[i]=malloc(length+1);
        if (!link_initial_environment[i]) recon_unsupported("CL LINK environment string allocation");
        memcpy(link_initial_environment[i],cursor,length);link_initial_environment[i][length]=0;cursor+=length;
    }
    count=read_word();
    if (count<2 || count>65536 || (unsigned)(limit-cursor)!=count*2)
        recon_unsupported("invalid CL LINK OS environment length");
    link_initial_os_environment=malloc(count*2);
    if (!link_initial_os_environment) recon_unsupported("CL LINK OS environment allocation");
    memcpy(link_initial_os_environment,cursor,count*2);link_initial_os_units=count;
    free(bytes);cursor=limit=NULL;
    recon_crt_bind_arguments(argc,argv,link_initial_environment);
    *recon_p_initenv()=link_initial_environment;
    recon_crt_bind_os_environment(link_initial_os_environment,link_initial_os_units,
        "e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210");
    return link_initial_environment;
}
