#include <stdint.h>
#include <stdlib.h>
/* Uninstrumented external test driver, not target graph/source implementation. */
#define RECON_SHIM_IMPL
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "../../src/link-pass.h"
#include <recon.h>
extern int main(int,char **);
extern void add_default_libraries(char *,LibraryList *);
extern void print_module_debug_info(void);
extern ModuleDebugInfo *module_debug_info;
extern unsigned long output_module_index;
unsigned long module_test_phase;
int module_test_result,*module_test_errno;
unsigned char module_test_input[256],module_test_guards[16];
Image *module_test_image;
static int write_all(int fd,const void *data,unsigned count)
{
    const char *p=data;int n;
    while(count){n=write(fd,p,count);if(n<=0)return 0;p+=n;count-=n;}
    return 1;
}
static int name_report(int fd,const char *name)
{
    unsigned length=strlen(name)+1;
    return write_all(fd,&length,4)&&write_all(fd,name,length);
}
static int write_report(void)
{
    unsigned header[4],i,count;int fd,okay;
    ModuleDebugInfo *row;DefaultLibrary *library;
    fd=open("diagnostics.bin",O_WRONLY|O_CREAT|O_TRUNC,0600);if(fd<0)return 0;
    header[0]=0x31444d4c;header[1]=1;header[2]=module_test_result;header[3]=output_module_index;
    okay=write_all(fd,header,16)&&write_all(fd,module_test_input,256);
    for(i=0;i<output_module_index&&okay;++i){
        row=&module_debug_info[i];okay=name_report(fd,row->library_name)&&name_report(fd,row->module_name)&&name_report(fd,row->source_name);
    }
    count=0;for(library=module_test_image->libraries.default_libraries;library;library=library->next)++count;
    okay=okay&&write_all(fd,&count,4);
    for(library=module_test_image->libraries.default_libraries;library&&okay;library=library->next)
        okay=write_all(fd,&library->enabled,1)&&name_report(fd,library->name);
    if(close(fd))okay=0;return okay;
}
void module_test_before_crt(void)
{
    /* Named observer boundary only; no application graph or original-caller claim. */
    module_test_phase=6;
}
int probe_main(int argc,char **argv)
{
    int fd;unsigned i;unsigned char extra;
    if(argc!=5)return 120;
    module_test_errno=&errno;
    fd=open(argv[1],O_RDONLY);if(fd<0||read(fd,module_test_input,256)!=256||read(fd,&extra,1)!=0||close(fd))return 121;
    memset(module_test_guards,0xa5,sizeof(module_test_guards));
    module_test_phase=0;module_test_result=main(argc-2,argv+2);
    if(module_test_result)return module_test_result;
    module_test_phase=1;create_image((Image **)module_test_guards);module_test_image=*(Image **)module_test_guards;
    module_test_phase=2;add_default_libraries((char *)module_test_input+32,&module_test_image->libraries);
    module_test_phase=3;add_default_libraries((char *)module_test_input+160,&module_test_image->libraries);
    module_test_phase=4;print_module_debug_info();
    module_test_phase=5;if(recon_fflush(&recon_iob[1]))return 122;
    if(!write_report())return 123;
    for(i=4;i<sizeof(module_test_guards);++i)if(module_test_guards[i]!=0xa5)return 124;
    module_test_phase=6;module_test_before_crt();return module_test_result;
}

/* Direct test completion follows the original external driver return/CRT exit. */
int run_probe(void)
{
    char *args[] = {"LINK.EXE","libraries.bin","R:\\toolchains\\1.0-nt\\bin\\LINK.EXE","-link","@modules.rsp",NULL};
    int code=probe_main(5,args);
    return code==0;
}
