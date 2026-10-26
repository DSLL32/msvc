#include <string.h>
#include <recon-tool.h>
#include "direct.h"
#include "breakout.h"
#include <recon.h>
#define CHECK(x) do { if (!(x)) return 0; } while (0)
extern char StartupDirectiveFound;
static int queued_contract(int token,int distance,int os,char *argument,const char **expected,unsigned count)
{
    struct asm_tok rows[2],before[2],*saved_tokens[2];
    module_info original,configured;
    input_queue *previous,*owned;
    char previous_flag,buffer[256];
    unsigned i;
    original=ModuleInfo;previous_flag=StartupDirectiveFound;
    saved_tokens[0]=AsmBuffer[0];saved_tokens[1]=AsmBuffer[1];
    memset(rows,0,sizeof(rows));rows[0].token=T_DIRECTIVE;rows[0].value=token;
    rows[1].token=T_NUM;rows[1].string_ptr=argument;
    memcpy(before,rows,sizeof(rows));AsmBuffer[0]=&rows[0];AsmBuffer[1]=&rows[1];
    memset(&ModuleInfo,0,sizeof(ModuleInfo));ModuleInfo.init=1;
    ModuleInfo.distance=distance;ModuleInfo.ostype=os;configured=ModuleInfo;
    StartupDirectiveFound=0;previous=line_queue;owned=PushLineQueue();
    CHECK(owned != NULL && owned->next == previous);
    CHECK(Startup(0) == 1);
    CHECK(memcmp(&ModuleInfo,&configured,sizeof(configured)) == 0);
    CHECK(memcmp(rows,before,sizeof(rows)) == 0 && AsmBuffer[0] == &rows[0] && AsmBuffer[1] == &rows[1]);
    CHECK(StartupDirectiveFound == (token==T_STARTUP));
    for(i=0;i<count;++i) {
        CHECK(line_queue == owned && owned->head != NULL);
        CHECK(strcmp(owned->head->line,expected[i]) == 0);
        CHECK(input_get(buffer) == buffer && strcmp(buffer,expected[i]) == 0);
    }
    CHECK(owned->head == NULL && owned->tail == NULL && owned->next == previous);
    PopLineQueue();CHECK(line_queue == previous);
    ModuleInfo=original;StartupDirectiveFound=previous_flag;
    AsmBuffer[0]=saved_tokens[0];AsmBuffer[1]=saved_tokens[1];
    return 1;
}
static const char *near_lines[]={
    "`symbol_reserved_for_start_address`:"," mov     dx,DGROUP"," mov     ds,dx",
    " mov     bx,ss"," sub     bx,dx"," shl     bx,1"," shl     bx,1",
    " shl     bx,1"," shl     bx,1"," cli     "," mov     ss,dx"," add     sp,bx"," sti     "
};
static const char *far_lines[]={"`symbol_reserved_for_start_address`:"," mov     dx,DGROUP"," mov     ds,dx"};
static int dos_near(void) { return queued_contract(T_STARTUP,1,0,NULL,near_lines,13); }
static int dos_far(void) { return queued_contract(T_STARTUP,0,0,NULL,far_lines,3); }
static int dos_exit(void)
{
    char argument[]="7";
    const char *lines[]={" mov    al, 7"," mov     ah,4ch"," int     21h"};
    CHECK(queued_contract(T_EXIT,1,0,argument,lines,3));CHECK(strcmp(argument,"7") == 0);
    return 1;
}
static int os2_exit(void)
{
    char argument[]="0";
    const char *lines[]={" mov    al, 0"," xor ah, ah"," push 01h"," push ax"," call far DOSEXIT"};
    CHECK(queued_contract(T_EXIT,0,1,argument,lines,5));CHECK(strcmp(argument,"0") == 0);
    return 1;
}
int startup_smoke(void) { return dos_far(); }
const ReconTest startup_tests[]={
    {"DOS-near-stack-template-and-owner",1,dos_near},
    {"DOS-far-stack-template-and-owner",1,dos_far},
    {"DOS-exit-return-value-template",1,dos_exit},
    {"OS2-exit-return-value-template",1,os2_exit}
};
