#define RECON_SHIM_IMPL
#include <assert.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <signal.h>
#include <recon.h>
extern long recon_osfhnd[256];
static void fp(unsigned char b[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])b)); }
int main(int argc,char **argv)
{
    unsigned char before[108],after[108],lowio[256],mb[257],pending[32],pending_copy[32];
    unsigned short ctype[257],*ctype_owner=recon_pctype;ShimFILE streams[20],original_streams[20];long handles[256];
    char prefix[32],prefix_copy[32];const char *argument=prefix,*mode=argc>1?argv[1]:"actual";
    unsigned n;off_t position;int flags;struct rlimit limit;
    memset(prefix,0x5a,sizeof(prefix));memcpy(prefix,"missing/log.txt",16);
    if(!strcmp(mode,"null")) argument=NULL;
    if(!strcmp(mode,"empty")) prefix[0]=0;
    if(!strcmp(mode,"ascii")) {memset(prefix,0x5a,sizeof(prefix));memcpy(prefix,"A prefix",9);}
    if(!strcmp(mode,"nonascii")) prefix[14]=(char)0x80;
    memcpy(prefix_copy,prefix,sizeof(prefix));
    memcpy(original_streams,recon_iob,sizeof(original_streams));
    for(n=0;n<sizeof(pending);++n)pending[n]=(unsigned char)(n+1);
    memcpy(pending_copy,pending,sizeof(pending));
    /* A pending FILE buffer must not be flushed by lowio stderr output. */
    recon_iob[2]._base=(char *)pending;recon_iob[2]._ptr=(char *)pending+7;
    recon_iob[2]._cnt=25;recon_iob[2]._flag=0x82;recon_iob[2]._bufsiz=32;
    recon_iob[3]._charbuf=0x13572468;recon_iob[3]._bufsiz=19;
    position=lseek(2,0,SEEK_CUR);
    if(!strcmp(mode,"binary"))recon_osfile[2]=1;
    if(!strcmp(mode,"unregistered"))recon_osfile[2]=0;
    if(!strcmp(mode,"handle"))recon_osfhnd[2]=0;
    if(!strcmp(mode,"nhandle"))recon_nhandle=2;
    if(!strcmp(mode,"append")){flags=fcntl(2,F_GETFL);assert(flags>=0);assert(!fcntl(2,F_SETFL,flags|O_APPEND));}
    if(!strcmp(mode,"readonly")){assert(dup2(open("/proc/self/fd/2",O_RDONLY),2)==2);}
    if(!strcmp(mode,"write-failure")){signal(SIGXFSZ,SIG_IGN);limit.rlim_cur=limit.rlim_max=0;assert(!setrlimit(RLIMIT_FSIZE,&limit));}
    if(!strcmp(mode,"mbcp"))recon_mb_codepage=1252;
    if(!strcmp(mode,"mbgate"))recon_mb_codepage_gate=1;
    if(!strcmp(mode,"lccp"))recon_lc_codepage=1252;
    if(!strcmp(mode,"mbmax"))recon_mb_cur_max=2;
    if(!strcmp(mode,"locale"))recon_lc_handle[2]=0x409;
    if(!strcmp(mode,"ctype"))++recon_pctype;
    memcpy(streams,recon_iob,sizeof(streams));memcpy(lowio,recon_osfile,sizeof(lowio));
    memcpy(handles,recon_osfhnd,sizeof(handles));memcpy(mb,recon_mbctype,sizeof(mb));
    memcpy(ctype,ctype_owner-1,sizeof(ctype));
    errno=!strcmp(mode,"errno")?EINVAL:ENOENT;recon_doserrno=!strcmp(mode,"dos")?2:3;
    recon_SetLastError(!strcmp(mode,"last")?2:3);
    if(argc>2)__asm__ __volatile__("fld1\n\tfldpi":::"st");
    fp(before);recon_perror(argument);fp(after);
    assert(errno==ENOENT && recon_doserrno==3 && recon_GetLastError()==3);
    assert(!memcmp(before,after,108));
    assert(!memcmp(streams,recon_iob,sizeof(streams)) && !memcmp(lowio,recon_osfile,sizeof(lowio)));
    assert(!memcmp(handles,recon_osfhnd,sizeof(handles)) && !memcmp(pending,pending_copy,sizeof(pending)));
    assert(!memcmp(prefix,prefix_copy,sizeof(prefix)) && !memcmp(mb,recon_mbctype,sizeof(mb)));
    assert(!memcmp(ctype,recon_pctype-1,sizeof(ctype)));
    assert(lseek(2,0,SEEK_CUR)-position==(argument && *argument?(off_t)strlen(argument)+2:0)+27);
    if(argc>2)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    memcpy(recon_iob,original_streams,sizeof(original_streams));
    puts("perror full FILE/lowio/inputs/tables/errors/108FP PASS");return 0;
}
