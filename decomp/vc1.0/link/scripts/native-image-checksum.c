/* External direct-support caller; no target graph or original-caller credit. */
#define RECON_SHIM_IMPL
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "../src/editor-image.h"
#include <recon.h>
unsigned long checksum_test_phase,checksum_test_outputs[4];
struct { unsigned char before[4]; BoundImage image; unsigned char after[16]; } checksum_test_workspace;
#define checksum_test_image checksum_test_workspace.image
unsigned char checksum_test_names[64];
int checksum_test_result,*checksum_test_errno;
static unsigned long checksum_test_report[10][7];
static int save_stage(const char *directory,unsigned phase)
{
    char path[640],buffer[65536];int source,destination,n,left,written;
    if(phase!=0&&phase!=4&&phase!=5&&phase!=6&&phase!=7&&phase!=8&&phase!=9)return 1;
    snprintf(path,sizeof(path),"%s/phase%u.exe",directory,phase);
    source=open("client.exe",O_RDONLY);if(source<0)return 0;
    destination=open(path,O_WRONLY|O_CREAT|O_TRUNC,0600);if(destination<0){close(source);return 0;}
    while((n=read(source,buffer,sizeof(buffer)))>0){
        left=n;while(left){written=write(destination,buffer+n-left,left);if(written<=0)return 0;left-=written;}
    }
    if(close(source)||close(destination))return 0;return n==0;
}
void checksum_test_after_call(void) { }
int __wrap_main(int argc,char **argv)
{
    unsigned phase;int report;
    if(argc!=3)return 120;
    checksum_test_errno=&errno;
    memset(checksum_test_outputs,0xa5,sizeof(checksum_test_outputs));
    memset(&checksum_test_workspace,0xa5,sizeof(checksum_test_workspace));
    memset(&checksum_test_image,0,sizeof(checksum_test_image));
    memset(checksum_test_names,0xa5,sizeof(checksum_test_names));strcpy((char *)checksum_test_names,"client.exe");strcpy((char *)checksum_test_names+32,"missing.exe");
    for(phase=0;phase<10;++phase){
        checksum_test_phase=phase;
        switch(phase){
        case 0:case 5:case 7:
            checksum_test_result=checksum_image_file((char *)checksum_test_names,checksum_test_outputs+1,checksum_test_outputs+3);break;
        case 1:checksum_test_result=map_bind_image((char *)checksum_test_names,&checksum_test_image,0,0);break;
        case 2:make_import_section_read_only(&checksum_test_image);checksum_test_result=0;break;
        case 3:checksum_test_result=recon_UnmapViewOfFile(checksum_test_image.base);break;
        case 4:checksum_test_result=recon_CloseHandle(checksum_test_image.file);break;
        case 6:case 8:
            checksum_test_result=checksum_image_file((char *)checksum_test_names,checksum_test_outputs+1,NULL);break;
        case 9:checksum_test_result=checksum_image_file((char *)checksum_test_names+32,checksum_test_outputs+1,checksum_test_outputs+3);break;
        }
        checksum_test_after_call();
        checksum_test_report[phase][0]=phase;
        checksum_test_report[phase][1]=checksum_test_result;
        checksum_test_report[phase][2]=checksum_test_outputs[1];
        checksum_test_report[phase][3]=checksum_test_outputs[3];
        checksum_test_report[phase][4]=errno;
        checksum_test_report[phase][5]=recon_doserrno;
        checksum_test_report[phase][6]=recon_GetLastError();
        if(!save_stage(argv[1],phase))return 121;
    }
    checksum_test_phase=10;
    report=open(argv[2],O_WRONLY|O_CREAT|O_TRUNC,0600);
    if(report<0||write(report,checksum_test_report,sizeof(checksum_test_report))!=sizeof(checksum_test_report)||close(report))return 122;
    return 0;
}
