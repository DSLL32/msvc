#define RECON_SHIM_IMPL
#include <stdint.h>
#include <stdlib.h>
/* Readable external dependency for direct minimal-code tests, not a LINK
 * reconstruction implementation. Original tests load the VC1-compiled DLL;
 * native cached tests link these same callbacks directly. */
#include "case.h"
#include <recon.h>
static IdeTestProbe probe;

TEST_EXPORT unsigned long __cdecl IdeProbeChannel(unsigned long kind)
{
    ++probe.get_calls;
    probe.last_kind=kind;
    if (kind!=2) ++probe.failed;
    return 7;
}
TEST_EXPORT int __cdecl IdeProbeWrite(unsigned long channel,const char *bytes,
                                    unsigned long count)
{
    IdeTestWrite *record;
    unsigned long i;
    if (probe.write_calls>=16 || count>1024 || probe.used+count>sizeof(probe.sink)) {
        ++probe.failed;
        return -1;
    }
    record=&probe.writes[probe.write_calls++];
    record->channel=channel;
    record->count=count;
    record->result=count;
    record->offset=probe.used;
    record->owned_buffer=(bytes==probe.buffer);
    if (channel!=7 || !record->owned_buffer) ++probe.failed;
    for (i=0;i<count;++i) probe.sink[probe.used++]=(unsigned char)bytes[i];
    return count;
}
TEST_EXPORT void __cdecl IdeProbeAssertion(const char *source,unsigned long line)
{
    /* The valid fixture must never invoke an assertion. Record unexpected
     * entry; no assertion/failure-path behavior is being emulated or claimed. */
    ++probe.assert_calls;
    ++probe.failed;
}
TEST_EXPORT IdeTestProbe * __cdecl GetIdeProbe(void)
{
    unsigned long i;
    unsigned char *data=(unsigned char *)&probe;
    for (i=0;i<sizeof(probe);++i) data[i]=0;
    for (i=0;i<32;++i) probe.before[i]=probe.after[i]=0xa5;
    for (i=0;i<sizeof(probe.buffer);++i) probe.buffer[i]=(char)0xa5;
    probe.api.get_channel=IdeProbeChannel;
    probe.api.write=IdeProbeWrite;
    probe.api.assertion=IdeProbeAssertion;
    probe.api.buffer=probe.buffer;
    probe.api.capacity=sizeof(probe.buffer);
    return &probe;
}
#ifndef RECON_NATIVE_TEST
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,void *reserved)
{
    return TRUE;
}
#endif

/* Explicit external minimal-code test caller. This unit and the callback
 * dependency are outside LINK's function graph and remain uninstrumented. */
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "case.h"
#include "../../src/ide.h"
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
int probe_main(int argc,char **argv)
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

/* Direct test completion follows the original external driver return/CRT exit. */
int run_probe(void)
{
    char *args[] = {"LINK.EXE","protocol.bin",NULL};
    int code=probe_main(2,args);
    return code==0;
}
