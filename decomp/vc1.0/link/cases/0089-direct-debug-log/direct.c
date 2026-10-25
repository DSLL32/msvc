#include <stdint.h>
#include <stdlib.h>
/* External direct diagnostic caller; no application/original graph credit. */
#define RECON_SHIM_IMPL
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "../../src/comdat-diagnostics.h"
#include <recon.h>
extern char debug_flags[100];
extern ShimFILE *debug_log;
unsigned char log_test_names[272];
unsigned log_test_phase;
int log_test_result,*log_test_errno;
static unsigned char rows[9][124];
int probe_main(int argc,char **argv)
{
    unsigned i,values[2],header[3]={0x31474c44,1,9},errors[3],pointer;int fd;char extra;
    if(argc!=3)return 120;
    log_test_errno=&errno;memset(log_test_names,0xa5,sizeof(log_test_names));
    fd=open(argv[1],O_RDONLY);if(fd<0||read(fd,log_test_names,256)!=256||read(fd,&extra,1)!=0||close(fd))return 121;
    for(log_test_phase=0;log_test_phase<9;++log_test_phase){
        switch(log_test_phase){
        case 0:debug_initialize((char *)log_test_names,(char *)log_test_names+32);log_test_result=0;break;
        case 1:log_test_result=open_debug_log((char *)log_test_names+64);break;
        case 2:log_test_result=debug_printf((char *)log_test_names+96);break;
        case 3:log_test_result=open_debug_log((char *)log_test_names+128);break;
        case 4:set_debug_flag(2,1);log_test_result=0;break;
        case 5:log_test_result=debug_printf((char *)log_test_names+160);break;
        case 6:set_debug_flag(2,0);log_test_result=0;break;
        case 7:log_test_result=open_debug_log(NULL);break;
        case 8:log_test_result=debug_printf((char *)log_test_names+192);break;
        }
        values[0]=log_test_phase;values[1]=log_test_result;
        memcpy(rows[log_test_phase],values,8);memcpy(rows[log_test_phase]+8,debug_flags,100);
        /* Copy closed-pointer representation without dereferencing the FILE. */
        memcpy(&pointer,&debug_log,4);values[0]=pointer?(pointer-(unsigned)&recon_iob[0])/32:0xffffffffUL;
        memcpy(rows[log_test_phase]+108,values,4);errors[0]=errno;errors[1]=recon_doserrno;errors[2]=recon_GetLastError();memcpy(rows[log_test_phase]+112,errors,12);
    }
    for(i=256;i<sizeof(log_test_names);++i)
        if(log_test_names[i]!=0xa5)return 123;
    fd=open(argv[2],O_WRONLY|O_CREAT|O_TRUNC,0600);
    if(fd<0||write(fd,header,sizeof(header))!=sizeof(header)||write(fd,log_test_names,256)!=256||write(fd,rows,sizeof(rows))!=sizeof(rows)||close(fd))return 122;
    return 0;
}

/* Direct test completion follows the original external driver return/CRT exit. */
int run_probe(void)
{
    char *args[] = {"LINK.EXE","names.bin","results.bin",NULL};
    int code=probe_main(3,args);
    return code==0;
}
