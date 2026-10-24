/* Explicit external typed caller, not a reconstructed application vertex. */
#define RECON_SHIM_IMPL
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include "../src/editor-image.h"
#include <recon.h>
unsigned char reloc_test_input[4112];
unsigned char *reloc_test_page;
unsigned short *reloc_test_words,*reloc_test_result;
unsigned reloc_test_phase;
int *reloc_test_errno;
static unsigned char rows[4][4196];
int __wrap_main(int argc,char **argv)
{
    unsigned char *page_region,*record_region,*row;
    unsigned header[5]={0x31424652,1,4,4096,16},values[5],count;
    unsigned short *words;
    long delta;
    int fd;
    char extra;
    if(argc!=3)return 120;
    reloc_test_errno=&errno;
    fd=open(argv[1],O_RDONLY);
    if(fd<0||read(fd,reloc_test_input,sizeof(reloc_test_input))!=sizeof(reloc_test_input)||read(fd,&extra,1)!=0||close(fd))return 121;
    page_region=mmap(NULL,12288,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    record_region=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if(page_region==MAP_FAILED||record_region==MAP_FAILED)return 122;
    reloc_test_page=page_region+4096;
    reloc_test_words=(unsigned short *)(record_region+16);
    memset(reloc_test_page-16,0xa5,4128);
    memcpy(reloc_test_page,reloc_test_input,4096);
    memset(record_region,0xa5,48);
    memcpy(reloc_test_words,reloc_test_input+4096,16);
    for(reloc_test_phase=0;reloc_test_phase<4;++reloc_test_phase){
        count=reloc_test_phase==0?0:reloc_test_phase==3?1:7;
        words=reloc_test_words+(reloc_test_phase==3?7:0);
        delta=reloc_test_phase==1?0x18004:reloc_test_phase==2?-0x18004:0;
        reloc_test_result=fallback_process_relocations(reloc_test_page,count,words,delta);
        if(reloc_test_result&&((unsigned)reloc_test_result<(unsigned)reloc_test_words||(unsigned)reloc_test_result>(unsigned)reloc_test_words+16))return 123;
        values[0]=reloc_test_phase;
        values[1]=reloc_test_result?(unsigned)reloc_test_result-(unsigned)reloc_test_words:0xffffffffUL;
        values[2]=errno;values[3]=recon_doserrno;values[4]=recon_GetLastError();
        row=rows[reloc_test_phase];memcpy(row,values,20);
        memcpy(row+20,reloc_test_page,4096);memcpy(row+4116,reloc_test_words,16);
        memcpy(row+4132,reloc_test_page-16,16);memcpy(row+4148,reloc_test_page+4096,16);
        memcpy(row+4164,record_region,16);memcpy(row+4180,record_region+32,16);
    }
    /* Both caller-owned mappings remain live through the real CRT exit. */
    fd=open(argv[2],O_WRONLY|O_CREAT|O_TRUNC,0600);
    if(fd<0||write(fd,header,sizeof(header))!=sizeof(header)||write(fd,reloc_test_input,sizeof(reloc_test_input))!=sizeof(reloc_test_input)||write(fd,rows,sizeof(rows))!=sizeof(rows)||close(fd))return 124;
    return 0;
}
