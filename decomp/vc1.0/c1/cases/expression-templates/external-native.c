/* External native unit runner. This object is excluded from the app graph. */
#include <ucontext.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "external.h"
#include <recon.h>

extern int __real_main(int, char **, char **);
extern void __real_finish_declarations(void);
extern IrToken *make_reusable_zero(void);
extern IrToken *__stdcall copy_expression_tree(IrToken *, int);
static unsigned selected_unit;
static int report_descriptor, invoked;
static TemplateObserver observer;
static TemplateInput input;
static unsigned slot_address;
static ucontext_t caller_context, unit_context;
static unsigned char *unit_stack;

static unsigned decimal(char *text)
{
    unsigned value=0;
    while(*text>='0' && *text<='9')value=value*10+*text++-'0';
    if(*text)abort();
    return value;
}
static int native_read(void *context,unsigned address,void *output,unsigned bytes)
{
    memcpy(output,(void *)address,bytes);
    return 1;
}
static void native_emit(void *context,const char *bytes,unsigned length)
{
    if(syscall(SYS_write,report_descriptor,bytes,length)!=(int)length)abort();
}
static void observe(const char *boundary)
{
    char label[100];
    sprintf(label,"input-%s",boundary);
    template_memory(&observer,label,(unsigned)&input,sizeof(input));
    sprintf(label,"pool-%s",boundary);
    template_memory(&observer,label,(unsigned)&compiler_arena.pools[2],sizeof(ArenaPool));
    sprintf(label,"slot-%s",boundary);
    if(slot_address)template_memory(&observer,label,slot_address,80);
    else template_hex(&observer,label,0,0,0);
    sprintf(label,"arena-%s",boundary);
    template_memory(&observer,label,(unsigned)&compiler_arena,8);
    if(compiler_arena.free_blocks) {
        sprintf(label,"free-%s",boundary);
        template_memory(&observer,label,(unsigned)compiler_arena.free_blocks,8);
    }
    template_owners(&observer,boundary);
}
static void execute_unit(void)
{
    IrToken *result;
    char line[100];
    unsigned before_stack,after_stack;
    unsigned short before_control,after_control;
    observe("before");
    __asm { mov before_stack,esp }
    __asm { fnstcw before_control }
    if(!selected_unit)result=make_reusable_zero();
    else result=copy_expression_tree((IrToken *)&input,template_mode(selected_unit));
    __asm { mov after_stack,esp }
    __asm { fnstcw after_control }
    sprintf(line,"expr-native-return %08x %08x %04x %04x\n",before_stack,after_stack,before_control,after_control);
    native_emit(0,line,strlen(line));
    if(before_stack!=after_stack || before_control!=after_control)abort();
    if(!slot_address)slot_address=(unsigned)result;
    observe("after");
    template_output(&observer,(unsigned)result);
    if(observer.failed)abort();
}
void __wrap_finish_declarations(void)
{
    FrontendSymbol *symbol;
    ArenaPool *pool=&compiler_arena.pools[2];
    unsigned index,bucket;
    char line[100];
    if(invoked++)abort();
    memset(&observer,0,sizeof(observer));
    observer.read=native_read;observer.emit=native_emit;
    observer.owners[0]=(unsigned)type_0064;
    observer.owners[1]=(unsigned)type_0108;
    symbol=0;
    for(bucket=0;bucket<=global_scope->bucket_mask && !symbol;++bucket) {
        FrontendSymbol *entry;
        for(entry=global_scope->buckets[bucket];entry;entry=entry->next)
            if(entry->name && !strcmp(entry->name,"template_text")) { symbol=entry;break; }
    }
    if(!symbol || (pool->remaining<80 && (pool->remaining || pool->next || pool->first || pool->last)))abort();
    observer.owners[2]=(unsigned)symbol->type;
    slot_address=(unsigned)pool->next;
    if(!slot_address) {
        if(!compiler_arena.free_blocks)abort();
        slot_address=(unsigned)compiler_arena.free_blocks->data;
    }
    template_build_input(selected_unit,(unsigned)&input,observer.owners,&input);
    sprintf(line,"expr-unit %u mode %u input %08x\n",selected_unit,selected_unit?template_mode(selected_unit):0,(unsigned)&input);
    native_emit(0,line,strlen(line));
    unit_stack=malloc(65536+32);if(!unit_stack)abort();
    memset(unit_stack,0xa5,65536+32);
    if(getcontext(&unit_context))abort();
    unit_context.uc_stack.ss_sp=unit_stack+16;
    unit_context.uc_stack.ss_size=65536;
    unit_context.uc_link=&caller_context;
    makecontext(&unit_context,execute_unit,0);
    if(swapcontext(&caller_context,&unit_context))abort();
    for(index=0;index<16;++index)
        if(unit_stack[index]!=0xa5 || unit_stack[65536+16+index]!=0xa5)abort();
    native_emit(0,"expr-stack-guards a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5\n",51);
    free(unit_stack);
    __real_finish_declarations();
}
int __wrap_main(int argc,char **argv,char **environment)
{
    char *original_arguments[2];
    if(argc!=3)return 89;
    selected_unit=decimal(argv[1]);
    report_descriptor=syscall(SYS_open,argv[2],577,0600);
    if(report_descriptor<0)return 89;
    if(selected_unit>5)return 89;
    original_arguments[0]=argv[0];original_arguments[1]=0;
    return __real_main(1,original_arguments,environment);
}
