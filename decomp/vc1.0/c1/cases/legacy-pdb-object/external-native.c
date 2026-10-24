/* External native PDB units. Never linked into the application graph audit. */
#define _GNU_SOURCE
#include <signal.h>
#include <ucontext.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <pthread.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <io.h>
#include "external.h"
#include <recon.h>

extern int __real_main(int, char **, char **);
extern ProgramDatabase *__real_open_program_database(char *, char *, unsigned);
extern void *__real_malloc(unsigned);
static unsigned selected_unit, allocation_count, tracking, invoked;
static int allocation_hook;
static unsigned allocations[32], allocation_bytes[32];
static int report_descriptor;
static PdbObserver observer;
static ProgramDatabase *database;
static ucontext_t unit_context;
static unsigned char *unit_stack;
static unsigned constructor_stack,constructor_return;
static void *main_stack;
static size_t main_stack_bytes;

static void fail(void)
{
    syscall(SYS_exit_group,88);
}
static int native_read(void *context,unsigned address,void *output,unsigned bytes)
{
    memcpy(output,(void *)address,bytes);
    return 1;
}
static void native_emit(void *context,const char *line,unsigned bytes)
{
    if(syscall(SYS_write,report_descriptor,line,bytes)!=(int)bytes)fail();
}
static void errors(const char *boundary)
{
    char line[120];
    sprintf(line,"pdb-errors %s %08x %08x %08x\n",boundary,errno,(unsigned)_doserrno,(unsigned)GetLastError());
    native_emit(0,line,strlen(line));
}
void *__wrap_malloc(unsigned bytes)
{
    void *result=__real_malloc(bytes);
    char line[120],label[80];
    Dl_info caller;
    int owned=0;
    if(tracking&&!allocation_hook) {
        allocation_hook=1;
        if(!dladdr(__builtin_return_address(0),&caller)||!caller.dli_sname)fail();
        owned=!strcmp(caller.dli_sname,"open_program_database")||!strcmp(caller.dli_sname,"load_program_database");
        allocation_hook=0;
    }
    if(owned) {
        if(!result||allocation_count>=32)fail();
        allocations[allocation_count]=(unsigned)result;allocation_bytes[allocation_count]=bytes;
        sprintf(line,"pdb-allocation %u %u %08x\n",allocation_count,bytes,(unsigned)result);
        native_emit(0,line,strlen(line));
        sprintf(label,"allocation-before-%u",allocation_count++);
        pdb_memory(&observer,label,(unsigned)result,bytes<256?bytes:256);
    }
    return result;
}
ProgramDatabase *__wrap_open_program_database(char *path,char *mode,unsigned signature)
{
    if(tracking)fail();
    tracking=1;
    return __real_open_program_database(path,mode,signature);
}

#include "../pdb-record-interface/external-native.h"
static void execute_unit(void)
{
    unsigned before_stack,after_stack,i;
    unsigned short before_control,after_control;
    int result,handle;
    char line[160],label[80];
    if(allocation_count!=3||allocation_bytes[0]!=116||allocations[0]!=(unsigned)database||
       allocation_bytes[1]!=12||allocation_bytes[2]!=12||
       allocations[1]!=(unsigned)database->records.head||database->records.head!=database->records.tail||database->records.count||
       allocations[2]!=(unsigned)database->blocks.head||database->blocks.head!=database->blocks.tail||database->blocks.count)fail();
    sprintf(line,"pdb-constructor %08x %08x %08x\n",(unsigned)database,constructor_stack,constructor_return);
    native_emit(0,line,strlen(line));
    pdb_database(&observer,"constructor",(unsigned)database,0);errors("constructor");
    __asm { mov before_stack,esp }
    __asm { fnstcw before_control }
    if(before_stack<(unsigned)unit_stack+16||before_stack>=(unsigned)unit_stack+65536+16)fail();
    result=open_program_database_object(database,0,"unit.pdb",selected_unit==1||selected_unit==3?"rc":"wc",0x31415926);
    __asm { mov after_stack,esp }
    __asm { fnstcw after_control }
    sprintf(line,"pdb-native-return 1 %u %08x %08x %04x %04x\n",result,before_stack,after_stack,before_control,after_control);
    native_emit(0,line,strlen(line));
    if(before_stack!=after_stack||before_control!=after_control)fail();
    handle=database->handle;
    if(selected_unit>=4&&selected_unit<=5) {
        if(result!=1||handle<0)fail();
        pdb_database(&observer,"reload-before",(unsigned)database,1);
        if(selected_unit==5&&_chsize(handle,32))fail();
        if(_lseek(handle,0,0))fail();
        __asm { mov before_stack,esp }
        __asm { fnstcw before_control }
        result=reload_program_database(database);
        __asm { mov after_stack,esp }
        __asm { fnstcw after_control }
        sprintf(line,"pdb-native-return 4 %u %08x %08x %04x %04x\n",result,before_stack,after_stack,before_control,after_control);
        native_emit(0,line,strlen(line));
        if(before_stack!=after_stack||before_control!=after_control)fail();
    }
    errors("result");pdb_database(&observer,"result",(unsigned)database,result==1||selected_unit>=4);
    for(i=0;i<allocation_count;++i) {
        sprintf(label,"allocation-after-%u",i);
        pdb_memory(&observer,label,allocations[i],allocation_bytes[i]<256?allocation_bytes[i]:256);
    }
    if(handle>=0) {
        sprintf(line,"pdb-offset %u\n",(unsigned)_lseek(handle,0,1));
        native_emit(0,line,strlen(line));
    }
    if(selected_unit>=6) {
        for(i=0;i<16;++i)
            if(unit_stack[i]!=0xa5||unit_stack[65536+16+i]!=0xa5)fail();
        native_emit(0,"pdb-native-stack-guards intact\n",31);
        execute_query();
    }
    for(i=0;i<16;++i)
        if(unit_stack[i]!=0xa5||unit_stack[65536+16+i]!=0xa5)fail();
    native_emit(0,"pdb-native-stack-guards intact\n",31);
    if(observer.failed)fail();
    syscall(SYS_exit_group,99);
}
static void constructor_trap(int signal,siginfo_t *info,void *opaque)
{
    ucontext_t *context=(ucontext_t *)opaque;
    unsigned entry=(unsigned)create_program_database;
    if(signal!=SIGTRAP||invoked++||!tracking||context->uc_mcontext.gregs[REG_EIP]!=entry+1)fail();
    database=(ProgramDatabase *)context->uc_mcontext.gregs[REG_ECX];
    constructor_stack=context->uc_mcontext.gregs[REG_ESP];
    if(constructor_stack<(unsigned)main_stack||constructor_stack>(unsigned)main_stack+main_stack_bytes-12)fail();
    constructor_return=*(unsigned *)constructor_stack;
    *(unsigned char *)entry=0x55;
    /* Carry the actual reached floating state into the external unit context. */
    *unit_context.uc_mcontext.fpregs=*context->uc_mcontext.fpregs;
    if(setcontext(&unit_context))fail();
    fail();
}
int __wrap_main(int argc,char **argv,char **environment)
{
    struct sigaction action;
    pthread_attr_t attributes;
    char *original_arguments[2],*end;
    unsigned entry=(unsigned)create_program_database;
    if(argc!=3)return 89;
    selected_unit=strtoul(argv[1],&end,10);
    if(*end||selected_unit>14)return 89;
    report_descriptor=syscall(SYS_open,argv[2],577,0600);
    if(report_descriptor<0)return 89;
    if(pthread_getattr_np(pthread_self(),&attributes)||pthread_attr_getstack(&attributes,&main_stack,&main_stack_bytes)||
       pthread_attr_destroy(&attributes))return 89;
    memset(&observer,0,sizeof(observer));observer.read=native_read;observer.emit=native_emit;
    unit_stack=__real_malloc(65536+32);if(!unit_stack)return 89;
    memset(unit_stack,0xa5,65536+32);
    if(getcontext(&unit_context))return 89;
    unit_context.uc_stack.ss_sp=unit_stack+16;unit_context.uc_stack.ss_size=65536;unit_context.uc_link=0;
    makecontext(&unit_context,execute_unit,0);
    memset(&action,0,sizeof(action));action.sa_sigaction=constructor_trap;action.sa_flags=SA_SIGINFO;
    if(sigaction(SIGTRAP,&action,0)||*(unsigned char *)entry!=0x55||
       mprotect((void *)(entry&~4095U),4096,PROT_READ|PROT_WRITE|PROT_EXEC))return 89;
    *(unsigned char *)entry=0xcc;
    original_arguments[0]=argv[0];original_arguments[1]=0;
    return __real_main(1,original_arguments,environment);
}
