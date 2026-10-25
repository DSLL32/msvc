/* Retained production guard/layout tests; no reconstructed functions are replaced. */
#define RECON_SHIM_IMPL
#include <stddef.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "command.h"
#include "directives.h"
#include <recon-tool.h>
#include <recon-native.h>
#include <recon.h>
extern int rcdll_native_invoke(int, char **);

static int trapped(void (*invoke)(void))
{
    int status;
    pid_t child=fork();
    if(child<0)return 0;
    if(!child){ invoke(); _exit(17); }
    if(waitpid(child,&status,0)!=child)return 0;
    return WIFSIGNALED(status)&&WTERMSIG(status)==SIGTRAP;
}
static void class_boundary(void){rc_character_class(256);}
static void unknown_directive(void){WCHAR name[]={0x101,0};rc_directive_code(name);}
static void integer_fallback(void){unsigned value=1;rc_heap=recon_HeapCreate(1,0,0);rc_store_integer(3,&value);}
static void defined_mode(void){rc_number_buffer_active=1;rc_evaluate_defined(L"defined");}
static void number_mode(void){WCHAR source[]={L' ',0};rc_source_cursor=source;rc_number_buffer_active=2;rc_read_number(L'1');}
static void workspace_boundary(void)
{
    rc_number_buffer=(WCHAR *)((char *)rc_string_buffer+2049);
    rc_token_value=L"winresrc.h";rc_cpp_path_characters=L"\\:/";
    rc_cpp_includes.remaining=99;rc_cpp_includes.items[99]=L"include";
    rc_search_include_directories();
}
static void marker_boundary(void)
{
    WCHAR source[300];
    const char *prefix="include \"";
    unsigned i=0,n;
    while(*prefix)source[i++]=(unsigned char)*prefix++;
    for(n=0;n<257;++n)source[i++]=L'a';
    source[i++]=L'"';source[i++]=L'\n';source[i]=0;
    rc_source_cursor=source;rc_cpp_active=1;
    rc_handle_directive();
}
static int guards(void)
{
    return trapped(class_boundary)&&trapped(unknown_directive)&&
        trapped(integer_fallback)&&trapped(defined_mode)&&trapped(number_mode)&&
        trapped(workspace_boundary)&&trapped(marker_boundary);
}
static int string_layout(void)
{
    unsigned word;
    rc_token_extra.identifier.hash=0x34;rc_token_extra.identifier.length=0x12;
    word=rc_token_extra.string_length;
    rc_token_extra.string_length=0xabcd;
    return sizeof(rc_token_extra)==2 &&
        offsetof(RcTokenExtra,identifier.hash)==0 &&
        offsetof(RcTokenExtra,identifier.length)==1 &&
        offsetof(RcTokenExtra,string_length)==0 && sizeof(rc_string_buffer)==4090 &&
        (char *)rc_number_buffer-(char *)rc_string_buffer==2048 &&
        word==0x1234 && rc_token_extra.identifier.hash==0xcd &&
        rc_token_extra.identifier.length==0xab;
}
static unsigned parameter_boundary_mode;
static void parameter_boundary(void)
{
    WCHAR input[]=L"quote)\n";
    rc_macro_parameter_count=0;rc_source_cursor=input;
    switch(parameter_boundary_mode){
    case 0:rc_macro_replacement=(WCHAR *)((DWORD)rc_string_buffer-2);break;
    case 1:rc_macro_replacement=(WCHAR *)((DWORD)rc_string_buffer+1);break;
    default:rc_macro_replacement=rc_string_buffer+2047-parameter_boundary_mode;break;
    }
    rc_parse_macro_parameters();
}
static int parameters(void)
{
    WCHAR input[]=L"quote)\n";
    const WCHAR expected[]={14,L'q',L'u',L'o',L't',L'e',0};
    unsigned i;
    memset(rc_string_buffer,0xa7,sizeof(rc_string_buffer));
    rc_macro_parameter_count=0;rc_macro_replacement=rc_string_buffer;
    rc_source_cursor=input;
    rc_parse_macro_parameters();
    if(sizeof(WCHAR)!=2 || rc_macro_parameter_count!=1 ||
       rc_macro_replacement!=rc_string_buffer+7 || rc_source_cursor!=input+6 ||
       memcmp(rc_string_buffer,expected,sizeof(expected)))return 0;
    for(i=sizeof(expected);i<sizeof(rc_string_buffer);++i)
        if(((unsigned char *)rc_string_buffer)[i]!=0xa7)return 0;
    for(parameter_boundary_mode=0;parameter_boundary_mode<5;++parameter_boundary_mode)
        if(!trapped(parameter_boundary))return 0;
    return 1;
}
static void no_space(void *unused)
{
    char *args[]={"S:\\vc5.10\\rcdll\\parameter-parser-original-02\\RC.EXE",
        "/r","/dCASE_FROM_COMMAND=7","/iinclude","/foresources.res","resources.rc",NULL};
    rcdll_native_invoke(6,args);
}
/* The legacy source-ownership guard was a debugger checkpoint, not a trap
 * in the reconstructed copy body. Preserve that boundary without executing
 * its unsupported read or accepting a later, unrelated frontier trap. */
static void __stdcall no_space_boundary(const WCHAR *value,unsigned length,int parameters)
{
    const WCHAR *expected=L"CASE_FROM_COMMAND";
    unsigned i;
    int passed=value && value[0]==L'7' && value[1]==0 && length==3 && parameters==-2;
    for(i=0; expected[i]; ++i)if(rc_macro_name[i]!=expected[i])passed=0;
    if(rc_macro_name[i])passed=0;
    recon_native_entry_finish(passed);
}
static int no_space_guard(void)
{
    return recon_native_at_entry((const void *)rc_define_macro,
        (const void *)no_space_boundary,no_space,NULL)==1;
}
static int copy_end_contract(void)
{
    const WCHAR source[]={L'x',0x1234,0};
    WCHAR destination[]={0xa7a7,0xa7a7,0xa7a7,0xa7a7};
    WCHAR empty[]={0xa7a7,0xc9c9};
    return rc_copy_wide_end(destination,source)==destination+2 &&
        destination[0]==L'x' && destination[1]==0x1234 && !destination[2] &&
        destination[3]==0xa7a7 && rc_copy_wide_end(empty,L"")==empty &&
        !empty[0] && empty[1]==0xc9c9;
}
/* Preserve-comments is independent of whether preprocessing emits output. */
static int comment_mode_identity(void)
{
    WCHAR input[]=L"hidden*/tail";
    unsigned line=rc_preparing_line;
    rc_cpp_C=0;
    rc_cpp_active=1;
    rc_cpp_output_file=NULL;
    rc_source_cursor=input;
    rc_block_comment();
    return rc_source_cursor==input+8 && rc_preparing_line==line;
}
const ReconTest rich_tests[]={
    {"production-native-guards",1,guards},
    {"string-storage-aliases",1,string_layout},
    {"wide-copy-end-contract",1,copy_end_contract},
    {"comment-preservation-option-identity",1,comment_mode_identity},
    {"packed-macro-parameter",1,parameters},
    {"old-no-space-before-unsupported-copy",1,no_space_guard}
};
