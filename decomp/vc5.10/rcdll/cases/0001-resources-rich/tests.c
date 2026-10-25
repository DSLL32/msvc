/* Retained production guard/layout tests; no reconstructed functions are replaced. */
#define RECON_SHIM_IMPL
#include <stddef.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "command.h"
#include "directives.h"
#include "macro-expansion.h"
#include <recon-tool.h>
#include <recon-native.h>
#include <recon.h>
extern int rcdll_native_invoke(int, char **);
extern int rc_macro_listing_readonly,rc_macro_listing_hidden;

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
static int replacement_quoted_contract(void)
{
    WCHAR source[]=L"abc\"tail";
    const WCHAR expected[]={L'"',L'a',L'b',L'c',L'"'};
    WCHAR *out=rc_string_buffer+10,*end;
    memset(rc_string_buffer,0xa7,sizeof(rc_string_buffer));
    rc_source_cursor=source;
    end=rc_copy_replacement_quoted(out,L'"');
    if(end!=out+5 || rc_source_cursor!=source+4 ||
       memcmp(out,expected,sizeof(expected)) || out[5]!=0xa7a7)return 0;
    rc_source_cursor=source;
    end=rc_copy_replacement_quoted(rc_string_buffer+2044,L'"');
    return end==rc_string_buffer+2043 && rc_source_cursor==source &&
        rc_string_buffer[2044]==L'"';
}
static void undef_query(const WCHAR *name,RcMacro *head)
{
    unsigned length=0;
    do rc_macro_name[length]=name[length]; while(name[length++]);
    rc_macro_name_length=length;
    rc_macro_hash=rc_hash_macro_name(rc_macro_name);
    rc_macro_buckets[rc_macro_hash]=head;
}
static int undef_unlink_contract(void)
{
    RcMacro last={NULL,L"last",NULL,0,0,0};
    RcMacro target={&last,L"REMOVE",NULL,0,0,0};
    RcMacro first={&target,L"other",NULL,0,0,0};
    rc_macro_listing_hidden=5;
    rc_macro_listing_readonly=7;
    undef_query(L"REMOVE",&first);
    rc_undefine_macro();
    if(rc_macro_buckets[rc_macro_hash]!=&first || first.next!=&last ||
       target.next!=&last || rc_macro_listing_hidden!=5 ||
       rc_macro_listing_readonly!=7)return 0;
    target.name=L"APSTUDIO_HIDDEN_SYMBOLS";
    undef_query(target.name,&target);
    rc_undefine_macro();
    if(rc_macro_buckets[rc_macro_hash]!=&last || rc_macro_listing_hidden ||
       rc_macro_listing_readonly!=7)return 0;
    rc_macro_listing_hidden=5;
    target.name=L"APSTUDIO_READONLY_SYMBOLS";
    undef_query(target.name,&target);
    rc_undefine_macro();
    return rc_macro_buckets[rc_macro_hash]==&last &&
        rc_macro_listing_hidden==5 && !rc_macro_listing_readonly;
}
static int packed_argument_contract(void)
{
    WCHAR *source=rc_string_buffer+1024;
    memset(rc_string_buffer,0xa7,sizeof(rc_string_buffer));
    source[0]=L'a';source[1]=0;source[2]=L'x';
    rc_macro_output_cursor=rc_string_buffer;
    rc_pack_macro_argument(source,source+3);
    if(rc_macro_output_cursor!=rc_string_buffer+6 ||
       rc_string_buffer[0]!=12 || rc_string_buffer[1]!=L'a' ||
       rc_string_buffer[2] || rc_string_buffer[3] || rc_string_buffer[4] ||
       rc_string_buffer[5]!=L'A' || rc_string_buffer[6]!=0xa7a7 ||
       source[0]!=L'a' || source[1] || source[2]!=L'x')return 0;
    rc_pack_macro_argument(source,source);
    if(rc_macro_output_cursor!=rc_string_buffer+9 ||
       rc_string_buffer[6]!=6 || rc_string_buffer[7] ||
       rc_string_buffer[8]!=L'A' || rc_string_buffer[9]!=0xa7a7)return 0;
    rc_expanding_macro=1;
    rc_macro_frames[1].output=rc_string_buffer;
    rc_macro_frames[1].arguments=2;
    rc_macro_frames[1].expanded=0xa7;
    rc_select_macro_argument(1);
    if(rc_source_cursor!=rc_string_buffer+1)return 0;
    rc_select_macro_argument(2);
    return rc_source_cursor==rc_string_buffer+7 &&
        rc_macro_frames[1].arguments==2 && rc_macro_frames[1].expanded==0xa7;
}
static int collected_identifier_contract(void)
{
    WCHAR source[]=L"ASE_FROM_COMMAND ";
    WCHAR blocked[]={L'A',L'S',L'E',4,4,L' ',0};
    WCHAR long_source[34];
    unsigned i;
    memset(rc_string_buffer,0xa7,sizeof(rc_string_buffer));
    rc_number_buffer=rc_string_buffer+1024;
    rc_source_cursor=source;
    rc_cpp_active=1;rc_cpp_C=0;
    if(!rc_collect_macro_identifier(L'C') || rc_macro_name_length!=18 ||
       rc_macro_hash!=13 || rc_number_buffer!=rc_string_buffer+1041 ||
       rc_source_cursor!=source+16 ||
       memcmp(rc_macro_name,L"CASE_FROM_COMMAND",36) ||
       rc_string_buffer[1041] || rc_string_buffer[1042]!=0xa7a7)return 0;
    for(i=0;i<32;++i)long_source[i]=L'A';
    long_source[32]=L' ';long_source[33]=0;
    rc_number_buffer=rc_string_buffer+1024;
    rc_source_cursor=long_source;
    if(!rc_collect_macro_identifier(L'A') || rc_macro_name_length!=34 ||
       rc_macro_hash!=97 || rc_source_cursor!=long_source+32)return 0;
    memset(rc_string_buffer,0xa7,sizeof(rc_string_buffer));
    rc_number_buffer=rc_string_buffer+1024;
    rc_source_cursor=blocked;
    rc_macro_name[0]=0xc9c9;rc_macro_name_length=0x12345678;rc_macro_hash=0xa7;
    rc_set_character_token(4,39);
    return !rc_collect_macro_identifier(L'C') &&
        rc_number_buffer==rc_string_buffer+1028 && rc_source_cursor==blocked+5 &&
        rc_macro_name[0]==0xc9c9 && rc_macro_name_length==0x12345678 &&
        rc_macro_hash==0xa7 && rc_string_buffer[1028]==0xa7a7;
}
static void nested_number_call(void){rc_read_number(L'2');}
static void nested_finish_call(void){rc_finish_macro_frame();}
static int nested_number_contract(void)
{
    WCHAR spelling[]={L'2',L'1',0,L'D',0};
    RcMacro macro={NULL,L"BASE_VALUE",spelling,0,1,0};
    unsigned i;
    memset(rc_string_buffer,0xa7,sizeof(rc_string_buffer));
    rc_expanding_macro=2;rc_number_buffer_active=2;rc_cpp_active=1;
    rc_macro_frames[2].macro=&macro;
    rc_source_cursor=spelling+1;
    rc_number_buffer=rc_string_buffer+1026;
    if(rc_read_number(L'2')!=1 || rc_number_buffer!=rc_string_buffer+1028 ||
       rc_source_cursor!=spelling+2 || rc_string_buffer[1026]!=L'2' ||
       rc_string_buffer[1027]!=L'1' || rc_string_buffer[1028] ||
       memcmp(spelling,L"21\0D",sizeof(spelling)))return 0;
    for(i=0;i<2045;++i)
        if((i<1026||i>1028)&&rc_string_buffer[i]!=0xa7a7)return 0;
    rc_source_cursor=spelling+1;rc_number_buffer=rc_string_buffer+1026;
    rc_number_buffer_active=3;
    if(!trapped(nested_number_call))return 0;
    rc_number_buffer_active=2;rc_expanding_macro=3;
    if(!trapped(nested_number_call))return 0;
    rc_expanding_macro=2;rc_source_cursor=spelling+2;
    if(!trapped(nested_number_call))return 0;
    rc_source_cursor=spelling+1;rc_number_buffer=rc_string_buffer+2044;
    if(!trapped(nested_number_call))return 0;
    rc_number_buffer=rc_string_buffer+1028;
    rc_macro_frames[2].source=rc_string_buffer+11;
    rc_macro_frames[2].output=rc_string_buffer+13;
    rc_macro_output_cursor=rc_string_buffer+20;
    if(rc_finish_macro_frame()!=0 || rc_number_buffer_active!=1 ||
       rc_expanding_macro!=1 || macro.flags ||
       rc_source_cursor!=rc_string_buffer+11 ||
       rc_macro_output_cursor!=rc_string_buffer+13 ||
       rc_number_buffer!=rc_string_buffer+1028 ||
       rc_string_buffer[1029]!=0xa7a7)return 0;
    rc_expanding_macro=2;rc_number_buffer_active=3;
    return trapped(nested_finish_call);
}
const ReconTest rich_tests[]={
    {"production-native-guards",1,guards},
    {"string-storage-aliases",1,string_layout},
    {"wide-copy-end-contract",1,copy_end_contract},
    {"comment-preservation-option-identity",1,comment_mode_identity},
    {"replacement-quoted-copy-contract",1,replacement_quoted_contract},
    {"macro-undefinition-unlink-contract",1,undef_unlink_contract},
    {"packed-argument-record-contract",1,packed_argument_contract},
    {"collected-identifier-output-contract",1,collected_identifier_contract},
    {"nested-numeric-spelling-contract",1,nested_number_contract},
    {"packed-macro-parameter",1,parameters},
    {"old-no-space-before-unsupported-copy",1,no_space_guard}
};
