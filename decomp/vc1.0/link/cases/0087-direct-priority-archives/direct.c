#include <stdint.h>
#include <stdlib.h>
/* External typed caller; direct constructor setup then genuine LINK main. */
#define RECON_SHIM_IMPL
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "../../src/arguments.h"
#include <recon.h>
extern int main(int,char **);
unsigned long priority_test_phase;
unsigned char priority_test_names[272];
Argument *priority_test_nodes[4];
char **priority_test_argv;
int priority_test_argc,priority_test_main_result,*priority_test_errno;
static unsigned node_index(Argument *node)
{
    unsigned i;if(!node)return 0xffffffffUL;
    for(i=0;i<4;++i)if(priority_test_nodes[i]==node)return i;
    return 0xfffffffeUL;
}
static int write_report(const char *path)
{
    unsigned header[8],row[4],i;int fd;
    Argument *node;
    header[0]=0x31495250;header[1]=1;header[2]=priority_test_main_result;header[3]=other_inputs.count;
    header[4]=node_index(other_inputs.first);header[5]=node_index(other_inputs.last);
    header[6]=node_index(first_ordinary_input);header[7]=node_index(last_priority_input);
    fd=open(path,O_WRONLY|O_CREAT|O_TRUNC,0600);if(fd<0||write(fd,header,32)!=32||write(fd,priority_test_names,256)!=256)return 0;
    for(i=0;i<4;++i){
        node=priority_test_nodes[i];row[0]=i;row[1]=(unsigned)(node->text-(char *)priority_test_names);row[2]=(unsigned)(node->original-(char *)priority_test_names);row[3]=node_index(node->next);
        if(write(fd,row,16)!=16)return 0;
    }
    return !close(fd);
}
int probe_main(int argc,char **argv)
{
    unsigned i;int fd;char extra;
    if(argc!=7)return 120;
    priority_test_errno=&errno;memset(priority_test_names,0xa5,sizeof(priority_test_names));
    fd=open(argv[1],O_RDONLY);if(fd<0||read(fd,priority_test_names,256)!=256||read(fd,&extra,1)!=0||close(fd))return 121;
    for(i=0;i<4;++i){priority_test_phase=i;priority_test_nodes[i]=insert_other_input((char *)priority_test_names+i*32,(char *)priority_test_names+i*32,(unsigned char)(i&1));}
    priority_test_phase=4;priority_test_argc=argc-3;priority_test_argv=argv+3;
    priority_test_main_result=main(priority_test_argc,priority_test_argv);
    priority_test_phase=5;if(!write_report(argv[2]))return 122;
    return priority_test_main_result;
}

/* Direct test completion follows the original external driver return/CRT exit. */
int run_probe(void)
{
    char *args[] = {"LINK.EXE","names.bin","results.bin","R:\\toolchains\\1.0-nt\\bin\\LINK.EXE","-dump","/nologo","/headers",NULL};
    int code=probe_main(7,args);
    return code==0;
}
