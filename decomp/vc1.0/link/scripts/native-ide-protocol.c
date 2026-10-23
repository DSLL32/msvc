/* Explicit external minimal-code test caller. This unit and the callback
 * dependency are outside LINK's function graph and remain uninstrumented. */
#define RECON_SHIM_IMPL
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../cases/ide-callback-protocol/sources/ide-probe.h"
#include "../src/ide.h"
#include <recon.h>
IdeTestProbe *ide_test_probe;
unsigned char ide_test_inputs[512],ide_test_out[68];
unsigned long ide_test_values[7],ide_test_returns[9];
char *ide_test_environment;
int *ide_test_errno;
static const char *messages[4]={"entry","source.obj","library.lib","symbol_name"};
static int write_all(int fd,const void *buffer,unsigned long count)
{
    const char *p=buffer;ssize_t n;
    while(count){n=write(fd,p,count);if(n<=0)return 0;p+=n;count-=n;}
    return 1;
}
int __wrap_main(int argc,char **argv)
{
    unsigned long i,head[8];int fd,okay;
    unsigned char original_inputs[512];
    char *flags;
    if(argc!=2)return 120;
    ide_test_probe=GetIdeProbe();
    memset(ide_test_inputs,0xa5,sizeof(ide_test_inputs));
    for(i=0;i<4;++i){memcpy(ide_test_inputs+i*128+32,messages[i],strlen(messages[i])+1);ide_test_values[i+1]=(unsigned long)(ide_test_inputs+i*128+32);}
    memcpy(original_inputs,ide_test_inputs,sizeof(original_inputs));
    ide_test_values[0]=0;ide_test_values[5]=0xfedcba98;ide_test_values[6]=0x12345678;
    memset(ide_test_out,0xa5,sizeof(ide_test_out));ide_test_environment=getenv("_MSC_IDE_FLAGS");ide_test_errno=&errno;
    if(!ide_test_environment || strcmp(ide_test_environment,"alpha.obj,beta.lib"))return 121;
    ide_test_returns[0]=initialize_ide_api((IdeApi *)&ide_test_probe->api);
    for(i=0;i<7;++i)ide_test_returns[i+1]=emit_ide_message(i,ide_test_values[i]);
    ide_test_returns[8]=read_ide_flags((char **)(ide_test_out+32));
    flags=*(char **)(ide_test_out+32);
    if(flags!=ide_test_environment || ide_test_returns[0]!=7 || ide_test_returns[8]!=1 ||
       ide_test_probe->get_calls!=1 || ide_test_probe->last_kind!=2 || ide_test_probe->write_calls!=7 ||
       ide_test_probe->failed || ide_test_probe->assert_calls || memcmp(ide_test_inputs,original_inputs,512))return 122;
    for(i=0;i<32;++i)
        if(ide_test_probe->before[i]!=0xa5 || ide_test_probe->after[i]!=0xa5 || ide_test_out[i]!=0xa5 || ide_test_out[i+36]!=0xa5)return 123;
    for(i=0;i<7;++i)if(ide_test_returns[i+1]!=ide_test_probe->writes[i].count)return 124;
    fd=open(argv[1],O_WRONLY|O_CREAT|O_TRUNC,0600);if(fd<0)return 125;
    head[0]=0x31454449;head[1]=1;head[2]=ide_test_probe->get_calls;head[3]=ide_test_probe->last_kind;
    head[4]=ide_test_probe->write_calls;head[5]=ide_test_probe->used;head[6]=ide_test_probe->assert_calls;head[7]=ide_test_probe->failed;
    okay=write_all(fd,head,sizeof(head)) && write_all(fd,ide_test_probe->writes,7*sizeof(IdeTestWrite)) &&
        write_all(fd,ide_test_probe->sink,ide_test_probe->used) && write_all(fd,ide_test_probe->before,32) &&
        write_all(fd,ide_test_probe->buffer,1024) && write_all(fd,ide_test_probe->after,32) &&
        write_all(fd,ide_test_returns,sizeof(ide_test_returns)) && write_all(fd,flags,strlen(flags)+1);
    if(close(fd) || !okay)return 126;
    return 0;
}
