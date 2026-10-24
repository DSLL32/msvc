/* Uninstrumented typed external parser caller, not a target graph vertex. */
#define RECON_SHIM_IMPL
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "../src/editor-image.h"
#include <recon.h>
extern int resource_verbose;
unsigned long bind_test_phase;
unsigned char bind_test_names[1040];
char *bind_test_vectors[4][6];
struct {unsigned char before[4];BoundImage image;unsigned char after[16];}bind_test_workspace;
int bind_test_result,*bind_test_errno;
static unsigned long rows[7][11];
int __wrap_main(int argc,char **argv)
{
    unsigned i;int fd;char extra;unsigned long header[3]={0x31444e42,1,7};
    if(argc!=3)return 120;
    bind_test_errno=&errno;memset(bind_test_names,0xa5,sizeof(bind_test_names));
    memset(bind_test_workspace.before,0xa5,4);memset(bind_test_workspace.after,0xa5,16);
    fd=open(argv[1],O_RDONLY);if(fd<0||read(fd,bind_test_names,1024)!=1024||read(fd,&extra,1)!=0||close(fd))return 121;
    bind_test_vectors[0][0]=(char *)bind_test_names;
    for(i=0;i<5;++i)bind_test_vectors[1][i]=(char *)bind_test_names+256+i*32;
    for(i=0;i<3;++i){bind_test_vectors[2][i]=(char *)bind_test_names+512+i*32;bind_test_vectors[3][i]=(char *)bind_test_names+768+i*32;}
    for(bind_test_phase=0;bind_test_phase<7;++bind_test_phase){
        switch(bind_test_phase){
        case 0:bind_test_result=parse_bind_arguments(1,bind_test_vectors[0]);break;
        case 1:bind_test_result=parse_bind_arguments(5,bind_test_vectors[1]);break;
        case 2:bind_test_result=map_bind_image(editor_image_name,&bind_test_workspace.image,0,bind_read_only);break;
        case 3:bind_test_result=recon_UnmapViewOfFile(bind_test_workspace.image.base);break;
        case 4:bind_test_result=recon_CloseHandle(bind_test_workspace.image.file);break;
        case 5:bind_test_result=parse_bind_arguments(3,bind_test_vectors[2]);break;
        case 6:bind_test_result=parse_bind_arguments(3,bind_test_vectors[3]);break;
        }
        rows[bind_test_phase][0]=bind_test_phase;rows[bind_test_phase][1]=bind_test_result;
        rows[bind_test_phase][2]=editor_usage_requested;rows[bind_test_phase][3]=bind_read_only;rows[bind_test_phase][4]=resource_verbose;
        rows[bind_test_phase][5]=editor_image_name?(unsigned)(editor_image_name-(char *)bind_test_names):0xffffffffUL;
        rows[bind_test_phase][6]=bind_search_path?(unsigned)(bind_search_path-(char *)bind_test_names):0xffffffffUL;
        rows[bind_test_phase][7]=errno;rows[bind_test_phase][8]=recon_doserrno;rows[bind_test_phase][9]=recon_GetLastError();
        rows[bind_test_phase][10]=bind_test_phase>=2?bind_test_workspace.image.section_count:0;
    }
    fd=open(argv[2],O_WRONLY|O_CREAT|O_TRUNC,0600);
    if(fd<0||write(fd,header,sizeof(header))!=sizeof(header)||write(fd,bind_test_names,1024)!=1024||write(fd,rows,sizeof(rows))!=sizeof(rows)||close(fd))return 122;
    return 0;
}
