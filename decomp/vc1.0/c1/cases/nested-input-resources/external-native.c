/* Fixed test-only calls from the genuine missing-include fatal boundary. */
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "../../src/source-input.h"
#include "../../src/macros.h"
#include "../../src/diagnostics.h"
#include "../../src/frontend.h"
#include "../../src/pch.h"
#include "../../src/browser.h"
#include <recon.h>
extern int __real_main(int,char **,char **);
extern void __real_recon_exit(int);
static unsigned selected_unit,invoked;
static int report_descriptor;
static SourceFile *owners[5];
static int owner_handles[5];
static void fail(void) { syscall(SYS_exit_group,88); }
static void emit(char *line)
{
    unsigned bytes=strlen(line);
    if(syscall(SYS_write,report_descriptor,line,bytes)!=(int)bytes)fail();
}
static void pending(char *when)
{
    FILE *streams[7];unsigned i,j,count,n;char line[8300];
    static char digits[]="0123456789abcdef";
    streams[0]=expression_file;streams[1]=symbol_file;streams[2]=line_file;
    streams[3]=global_file;streams[4]=debug_file;streams[5]=pch_file;streams[6]=browser_output;
    sprintf(line,"resource-streams %s\n",when);emit(line);
    for(i=0;i<7;++i) {
        n=sprintf(line,"buffer %u ",i);
        if(!streams[i])n+=sprintf(line+n,"unopened\n");
        else {
            count=(unsigned)streams[i]->_ptr-(unsigned)streams[i]->_base;if(count>4096)fail();
            n+=sprintf(line+n,"%u ",count);
            for(j=0;j<count;++j){unsigned byte=(unsigned char)streams[i]->_base[j];line[n++]=digits[byte>>4];line[n++]=digits[byte&15];}line[n++]='\n';line[n]=0;
        }
        emit(line);
    }
}
static void string_value(char *label,char *text)
{
    char line[600];
    if(!text||strlen(text)>=260)fail();
    sprintf(line,"resource-string %s %08x %s\n",label,(unsigned)text,text);emit(line);
}
static void snapshot(char *when)
{
    SourceFile *node=active_input;
    unsigned i,j,*words;
    char line[400],label[32];
    if(include_depth!=4)fail();
    sprintf(line,"resource-state %s depth %d errno %u dos %u\n",when,include_depth,errno,(unsigned)_doserrno);emit(line);
    if(!strcmp(when,"before")&&errno!=ENOENT)fail();
    for(i=0;i<5;++i) {
        if(!node||node->input_kind)fail();
        if(!strcmp(when,"before")) {
            if(node->handle<0||node->capacity!=(i<3?0x1000:i==3?0x2000:0x3000))fail();
            for(j=0;j<i;++j)if(owners[j]==node||owner_handles[j]==node->handle)fail();
            owners[i]=node;owner_handles[i]=node->handle;
        } else if(owners[i]!=node)fail();
        sprintf(line,"resource-node %s %u %08x",when,i,(unsigned)node);emit(line);
        words=(unsigned *)node;
        for(j=0;j<13;++j){sprintf(line," %08x",words[j]);emit(line);}emit("\n");
        sprintf(label,"%s-%u",when,i);string_value(label,node->filename);
        node=node->next;
    }
    if(node)fail();
}
void __wrap_recon_exit(int code)
{
    char line[100];
    if(invoked!=1||selected_unit<2||code!=(selected_unit==2?1:0x8000))fail();
    sprintf(line,"resource-native-exit unit %u code %u\n",selected_unit,(unsigned)code);emit(line);
    if(selected_unit==3)pending("exit");
    __real_recon_exit(code);
    fail();
}
void __wrap_report_fatal(unsigned code,char *kind,char *filename)
{
    unsigned before,after,result=0,i;
    unsigned short control_before,control_after;
    void *allocation;
    char line[160];
    if(invoked++||code!=83||strcmp(kind,"include")||strcmp(filename,"absent-resource.h"))fail();
    snapshot("before");pending("before");string_value("kind",kind);string_value("missing",filename);
    sprintf(line,"resource-context unit %u line %u\n",selected_unit,source_line_number);emit(line);
    __asm { mov before,esp }
    __asm { fnstcw control_before }
    switch(selected_unit) {
    case 0:
        allocation=retry_malloc(96);
        if(!allocation)fail();
        sprintf(line,"resource-allocation bytes96 pointer %08x\n",(unsigned)allocation);emit(line);
        result=1;break;
    case 1:
        result=release_input_handle();if(result!=1)fail();break;
    case 2:
        sprintf(line,"resource-native-report code83 kind %08x file %08x source %08x line %u\n",(unsigned)kind,(unsigned)filename,(unsigned)owners[0]->filename,source_line_number);emit(line);
        report_error(code,kind,filename,owners[0]->filename,source_line_number);fail();break;
    case 3:compiler_signal(2);fail();break;
    default:fail();
    }
    __asm { mov after,esp }
    __asm { fnstcw control_after }
    if(before!=after||control_before!=control_after)fail();
    sprintf(line,"resource-native-return unit %u result %u esp %08x %08x control %04x %04x\n",selected_unit,result,before,after,control_before,control_after);emit(line);
    snapshot("after");pending("after");
    for(i=0;i<5;++i)if(owners[i]->handle!=(selected_unit==1&&i==2?-1:owner_handles[i]))fail();
    syscall(SYS_exit_group,99);fail();
}
int __wrap_main(int argc,char **argv,char **environment)
{
    char *end,*original[2];
    if(argc!=3)return 89;
    selected_unit=strtoul(argv[1],&end,10);if(*end||selected_unit>3)return 89;
    report_descriptor=syscall(SYS_open,argv[2],577,0600);if(report_descriptor<0)return 89;
    original[0]=argv[0];original[1]=0;
    return __real_main(1,original,environment);
}
