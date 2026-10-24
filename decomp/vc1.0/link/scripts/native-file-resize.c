/* External direct file-subsystem test caller, outside LINK's function graph. */
#define RECON_SHIM_IMPL
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "../src/link.h"
#include "../src/files.h"
#include <recon.h>
extern int open_file(const char *,int,int);
extern unsigned read_file(int,void *,unsigned);
extern long seek_file(int,long,int);
extern int close_file(int,unsigned char);
extern int resize_file(int,long);
unsigned long resize_test_phase;
int resize_test_handle,resize_test_returns[15];
int *resize_test_errno;
unsigned char resize_test_payload[12289],resize_test_buffer[64];
int __wrap_main(int argc,char **argv)
{
    unsigned i;int mapped,report;
    if(argc!=2)return 120;
    mapped=!strcmp(argv[1],"mapped");if(!mapped&&strcmp(argv[1],"buffered"))return 120;
    resize_test_errno=&errno;
    for(i=0;i<sizeof(resize_test_payload);++i)resize_test_payload[i]=(unsigned char)(i*37+11);
    resize_test_phase=0;files_initialize(30,16,14,8,6,mapped,file_failure);
    resize_test_returns[0]=0;
    resize_test_phase=1;resize_test_returns[1]=resize_test_handle=open_file("work.bin",0x8102,0x180);
    resize_test_phase=2;resize_test_returns[2]=write_file(resize_test_handle,resize_test_payload,sizeof(resize_test_payload));
    resize_test_phase=3;resize_test_returns[3]=seek_file(resize_test_handle,9000,0);
    resize_test_phase=4;resize_test_returns[4]=resize_file(resize_test_handle,sizeof(resize_test_payload));
    resize_test_phase=5;resize_test_returns[5]=resize_file(resize_test_handle,0x500001);
    resize_test_phase=6;resize_test_returns[6]=seek_file(resize_test_handle,0,0);
    resize_test_phase=7;resize_test_returns[7]=read_file(resize_test_handle,resize_test_buffer,64);
    if(resize_test_returns[7]!=64||memcmp(resize_test_buffer,resize_test_payload,64))return 121;
    resize_test_phase=8;resize_test_returns[8]=resize_file(resize_test_handle,4097);
    resize_test_phase=9;resize_test_returns[9]=seek_file(resize_test_handle,0,0);
    resize_test_phase=10;resize_test_returns[10]=read_file(resize_test_handle,resize_test_buffer,64);
    if(resize_test_returns[10]!=64||memcmp(resize_test_buffer,resize_test_payload,64))return 122;
    resize_test_phase=11;resize_test_returns[11]=close_file(resize_test_handle,1);
    resize_test_phase=12;resize_test_returns[12]=resize_test_handle=open_file("work.bin",0x8000,0x180);
    resize_test_phase=13;resize_test_returns[13]=read_file(resize_test_handle,resize_test_buffer,64);
    if(resize_test_returns[13]!=64||memcmp(resize_test_buffer,resize_test_payload,64))return 123;
    resize_test_phase=14;resize_test_returns[14]=close_file(resize_test_handle,1);
    report=open("results.bin",O_WRONLY|O_CREAT|O_TRUNC,0600);
    if(report<0||write(report,resize_test_returns,sizeof(resize_test_returns))!=sizeof(resize_test_returns)||close(report))return 124;
    return 0;
}
