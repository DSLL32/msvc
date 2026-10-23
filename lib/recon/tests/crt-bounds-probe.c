/* Original-compiler reference probe; ordinary native checks launch no Wine. */
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <recon.h>
extern unsigned char _osfile[];
static void bounded(int capacity,const char *value)
{
    char buffer[10]; int result,i;
    memset(buffer,'!',sizeof(buffer)); errno=0;
    result=_snprintf(buffer,capacity,"%s",value);
    printf("bound %d %u %d %d ",capacity,strlen(value),result,errno);
    for(i=0;i<10;i++) printf("%02x",(unsigned char)buffer[i]);
    printf("\n");
}
int main(void)
{
    int cap,result,left,right,fd,i,old,mode,err; char buffer[10];
    for(cap=0;cap<=6;cap++) bounded(cap,"abcd");
    bounded(0,""); bounded(1,"");
#if !defined(RECON_NATIVE_TEST) || defined(RECON_PROBE_PERCENT_N)
    /* Explicit original characterization: host %n cannot model these effects. */
    for(cap=0;cap<=6;cap++) {
        memset(buffer,'!',sizeof(buffer)); left=right=99; errno=0;
        result=_snprintf(buffer,cap,"%s%nX%n","abcd",&left,&right);
        printf("count %d %d %d %d %d ",cap,result,left,right,errno);
        for(i=0;i<10;i++) printf("%02x",(unsigned char)buffer[i]);
        printf("\n");
    }
#endif
    fd=_open("mode.tmp",_O_CREAT|_O_TRUNC|_O_RDWR|_O_BINARY,0600);
    for(i=0;i<4;i++) {
        mode=i==0?0x4000:i==1?0x4000:i==2?0x8000:0;
        old=_osfile[fd]; errno=0; result=_setmode(fd,mode); err=errno;
        printf("mode %d %d %d %d %d %d\n",fd,mode,old,result,_osfile[fd],err);
    }
    _close(fd); errno=0; result=_setmode(fd,0);err=errno;
    printf("closed %d %d\n",result,err);
    errno=0; result=_setmode(-1,0x4000);err=errno;
    printf("negative %d %d\n",result,err);
    return 0;
}
