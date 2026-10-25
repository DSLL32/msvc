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
#include "resource-input.h"
#include "resources.h"
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
static int substitute_invalid_index(void)
{
    WCHAR text[]={2,L')',0};
    RcMacro macro={NULL,L"ARG",text,1,1,0};
    int expanded=0x12345678;
    rc_expanding_macro=1;
    rc_macro_frames[1].macro=&macro;
    rc_macro_frames[1].arguments=1;
    rc_macro_frames[1].output=rc_string_buffer;
    rc_source_cursor=text;
    return !rc_substitute_macro_argument(&expanded) &&
        expanded==0x12345678 && rc_source_cursor==text+1 &&
        text[0]==2 && text[1]==L')' && !text[2];
}
static int identifier_lookahead_contract(void)
{
    WCHAR text[]={1,L'x',L')',0};
    RcMacro macro={NULL,L"LOOK",text,1,1,0};
    unsigned i;
    rc_set_character_token(1,36);
    rc_expanding_macro=1;
    rc_macro_frames[1].macro=&macro;
    for(i=0;i<4;++i){
        rc_source_cursor=text+i;
        if(rc_macro_identifier_follows()!=(i<2) || rc_source_cursor!=text+i)return 0;
    }
    return text[0]==1 && text[1]==L'x' && text[2]==L')' && !text[3];
}
static void interior_number_call(void){rc_read_number(L'2');}
static int interior_number_contract(void)
{
    WCHAR text[]={L'(',L'(',1,1,L')',L' ',L'*',L' ',L'2',L')',0,L'D',0};
    RcMacro macro={NULL,L"TWICE",text,1,1,0};
    rc_expanding_macro=1;rc_number_buffer_active=1;rc_cpp_active=1;
    rc_macro_frames[1].macro=&macro;
    rc_number_buffer=rc_string_buffer+1034;
    rc_source_cursor=text+9;
    rc_string_buffer[1036]=0xa7a7;
    if(rc_read_number(L'2')!=1 || rc_number_buffer!=rc_string_buffer+1035 ||
       rc_string_buffer[1034]!=L'2' || rc_string_buffer[1035] ||
       rc_string_buffer[1036]!=0xa7a7 || rc_source_cursor!=text+9)return 0;
    rc_source_cursor=text+8;
    if(!trapped(interior_number_call))return 0;
    rc_source_cursor=text;
    if(!trapped(interior_number_call))return 0;
    rc_source_cursor=text+11;
    return trapped(interior_number_call);
}
static const WCHAR *quoted_input,*quoted_expected;
static unsigned quoted_words;
static WCHAR *quoted_resume;
static void quoted_invoke(void *unused)
{
    rc_source_cursor=(WCHAR *)quoted_input;
    rc_cpp_output_file=NULL;
    rc_copy_quoted(L'"');
}
/* Stops before actual IO; verifies the first bounded output chunk, not the
 * writer or any continuation after this direct-test checkpoint. */
static void __stdcall quoted_write_boundary(const void *text,unsigned bytes,
                                            unsigned count,FILE *stream)
{
    int passed=text==rc_macro_name && bytes==quoted_words*2 && count==1 &&
        !stream && !rc_macro_name[quoted_words] &&
        rc_source_cursor==quoted_resume &&
        !memcmp(text,quoted_expected,bytes);
    recon_native_entry_finish(passed);
}
static int quoted_output_contract(void)
{
    WCHAR normal[]=L"abc\"tail",newline[]=L"abc\n",escape[]=L"a\\\"b\"";
    WCHAR long_text[261],long_expected[256];
    unsigned i;
    quoted_input=normal;quoted_expected=L"\"abc\"";quoted_words=5;
    quoted_resume=normal+4;
    if(recon_native_at_entry(rc_write_preprocessed,quoted_write_boundary,quoted_invoke,NULL)!=1)return 0;
    quoted_input=newline;quoted_expected=L"\"abc";quoted_words=4;
    quoted_resume=newline+3;
    if(recon_native_at_entry(rc_write_preprocessed,quoted_write_boundary,quoted_invoke,NULL)!=1)return 0;
    quoted_input=escape;quoted_expected=L"\"a\\\"b\"";quoted_words=6;
    quoted_resume=escape+5;
    if(recon_native_at_entry(rc_write_preprocessed,quoted_write_boundary,quoted_invoke,NULL)!=1)return 0;
    for(i=0;i<260;++i)long_text[i]=L'a';
    long_text[260]=0;
    long_expected[0]=L'"';
    for(i=1;i<255;++i)long_expected[i]=L'a';
    long_expected[255]=0;
    quoted_input=long_text;quoted_expected=long_expected;quoted_words=255;
    quoted_resume=long_text+254;
    return recon_native_at_entry(rc_write_preprocessed,quoted_write_boundary,quoted_invoke,NULL)==1;
}
static int raw_macro_argument_contract(void)
{
    WCHAR text[]={1,L')',0};
    WCHAR *spelling=rc_string_buffer+1024;
    RcMacro macro={NULL,L"RAW",text,2,1,0};
    memset(rc_string_buffer,0xa7,sizeof(rc_string_buffer));
    spelling[0]=L'a';spelling[1]=0;
    rc_macro_output_cursor=rc_string_buffer;
    rc_pack_macro_argument(spelling,spelling+1);
    spelling[0]=L'b';
    rc_pack_macro_argument(spelling,spelling+1);
    rc_expanding_macro=1;rc_macro_frames[1].arguments=2;
    rc_macro_frames[1].macro=&macro;rc_macro_frames[1].output=rc_string_buffer;
    rc_source_cursor=text;
    if(rc_macro_argument_text()!=rc_string_buffer+1||rc_source_cursor!=text+1)return 0;
    text[0]=2;rc_source_cursor=text;
    if(rc_macro_argument_text()!=rc_string_buffer+5||rc_source_cursor!=text+1)return 0;
    text[0]=3;rc_source_cursor=text;
    return !rc_macro_argument_text()&&rc_source_cursor==text+1;
}
static int expanded_quoted_contract(void)
{
    const WCHAR spelling[]={L'"',L'a',L'\\',L'b',L'"',0};
    const WCHAR expected[]={L'"',L'\\',L'"',L'a',L'\\',L'\\',L'b',L'\\',L'"',L'"'};
    WCHAR *source=rc_string_buffer+1024;
    memset(rc_string_buffer,0xa7,sizeof(rc_string_buffer));
    memcpy(source,spelling,sizeof(spelling));
    rc_macro_output_cursor=rc_string_buffer;
    rc_pack_macro_argument(source,source+5);
    rc_number_buffer=rc_string_buffer+1024;
    rc_copy_expanded_quoted(L'"',NULL);
    if(rc_number_buffer!=rc_string_buffer+1024)return 0;
    rc_string_buffer[1034]=0xc9c9;
    rc_copy_expanded_quoted(L'"',rc_string_buffer+1);
    return rc_number_buffer==rc_string_buffer+1034 &&
        !memcmp(rc_string_buffer+1024,expected,sizeof(expected)) &&
        rc_string_buffer[1034]==0xc9c9 &&
        !memcmp(rc_string_buffer+1,spelling,sizeof(spelling));
}
static void apstudio_unobserved(void)
{
    rc_apstudio=1;
    rc_initialize_resource();
}
static int apstudio_hold(void){return trapped(apstudio_unobserved);}
static void resource_input_invoke(void *unused)
{
    rc_heap=(HANDLE)0x55555550;
    rc_result=17;rc_resource_status=5;rc_resource_file_index=7;rc_resource_line=9;
    rc_font_resource_count=0x4455;rc_hwb_resource_offset=0x55667788;
    rc_resource_types=(RcResourceType *)0x12345670;
    rc_resource_input_file=(FILE *)0x23456780;
    rc_initialize_input((FILE *)0x12345678);
}
static unsigned __cdecl resource_character_boundary(void)
{
    recon_native_entry_finish(rc_heap==(HANDLE)0x55555550&&rc_result==17&&!rc_resource_status&&!rc_resource_file_index&&
        rc_font_resource_count==0x4455&&rc_hwb_resource_offset==0x55667788&&
        rc_resource_types==(RcResourceType *)0x12345670&&
        rc_resource_line==1&&rc_resource_input_file==(FILE *)0x12345678);
}
static int resource_input_contract(void)
{
    return recon_native_at_entry(rc_next_resource_character,
        resource_character_boundary,resource_input_invoke,NULL)==1;
}
static void negative_resource_index(void)
{
    rc_resource_buffer_index=-1;
    rc_read_resource_code_unit();
}
static int resource_buffer_contract(void)
{
    rc_resource_buffer_index=0;
    rc_resource_read_buffer[0]=0x1234;
    rc_resource_read_buffer[1]=0;
    rc_resource_character=0xa7a7;
    if(rc_read_resource_code_unit()!=0x1234||rc_resource_buffer_index!=1||
       rc_resource_character!=0xa7a7)return 0;
    if(rc_read_resource_code_unit()||rc_resource_buffer_index!=1||
       rc_read_resource_code_unit()||rc_resource_buffer_index!=1)return 0;
    rc_resource_buffer_index=4096;rc_resource_read_buffer[4096]=0xc9c9;
    if(rc_read_resource_code_unit()!=0xc9c9||rc_resource_buffer_index!=4097)return 0;
    return trapped(negative_resource_index);
}
static int byte_read_contract(void)
{
    unsigned char bytes[9];
    unsigned received;
    ShimFILE *file=recon_fopen("resources.rc","rb");
    if(!file)return 0;
    memset(bytes,0xa7,sizeof(bytes));
    received=rc_read_bytes((FILE *)file,bytes,8);
    if(received!=8 || memcmp(bytes,"#include",8) || bytes[8]!=0xa7 ||
       recon_ferror(file))return 0;
    return recon_fclose(file)==0;
}

extern WORD rc_language;
extern int rc_verbose,rc_mac;
extern char rc_input_name[260],rc_output_path[260];
extern char *rc_output_temporary;
static void command_reset_invoke(void *unused)
{
    rc_heap=(HANDLE)0x66666660;
    rc_result=17;rc_resource_status=51;rc_codepage=88;rc_language=0x1234;
    rc_font_resource_count=0x2345;
    rc_input_temporary=(char *)0x11111110;
    rc_output_temporary=(char *)0x22222220;
    rc_hwb_resource_offset=0x33445566;
    rc_resource_types=(RcResourceType *)0x12345670;
    rc_string_tables=(void *)0x44444440;
    rc_verbose=1;rc_mac=1;rc_input_name[0]='q';rc_output_path[0]='q';
    rc_initialize();
}
static HANDLE WINAPI command_reset_boundary(DWORD flags,DWORD initial,DWORD maximum)
{
    recon_native_entry_finish(flags==1&&!initial&&!maximum&&
        rc_heap==(HANDLE)0x66666660&&!rc_result&&rc_resource_status==51&&!rc_codepage&&rc_language==128&&
        !rc_font_resource_count&&!rc_input_temporary&&!rc_output_temporary&&
        !rc_hwb_resource_offset&&!rc_resource_types&&
        rc_string_tables==(void *)0x44444440&&
        !rc_verbose&&!rc_mac&&!rc_input_name[0]&&!rc_output_path[0]);
}
static int command_reset_contract(void)
{
    return recon_native_at_entry(recon_HeapCreate,command_reset_boundary,
                                  command_reset_invoke,NULL)==1;
}

static int resource_type_lookup_contract(void)
{
    WCHAR cursor[]=L"CURSOR",icon[]=L"ICON";
    RcResourceType first={0},second={0},zero_id={0};
    RcResourceType *saved=rc_resource_types;
    int passed;
    first.next=&second;first.name=cursor;first.id=12;
    second.next=&zero_id;second.name=icon;second.id=14;
    rc_resource_types=&first;
    passed=rc_find_resource_type(icon,12)==&second &&
        rc_find_resource_type(NULL,0x1000e)==&second &&
        rc_find_resource_type(NULL,0x10000)==&zero_id;
    rc_resource_types=saved;
    return passed;
}

extern int rc_resource_line_start;
static void prepare_resource_name(unsigned length)
{
    unsigned i;
    memset(rc_resource_read_buffer,0,sizeof(rc_resource_read_buffer));
    for(i=0;i+1<length;++i)rc_resource_read_buffer[i]=L'a';
    rc_resource_read_buffer[length-1]=L' ';
    rc_resource_buffer_index=0;
    rc_resource_character=L'a';rc_resource_line_start=0;
    rc_resource_token.kind=3;rc_resource_token.text[0]=L'q';
}
static void overlong_resource_name(void)
{
    prepare_resource_name(257);
    rc_collect_resource_name();
}
static int collected_resource_name_contract(void)
{
    unsigned i;
    prepare_resource_name(256);
    if(rc_collect_resource_name()!=rc_resource_name ||
       rc_resource_name[256] || rc_resource_token.text[0] ||
       rc_resource_character!=127 || rc_resource_token.kind!=127)return 0;
    for(i=0;i<256;++i)if(rc_resource_name[i]!=L'a')return 0;
    return trapped(overlong_resource_name);
}

static void resource_number_invoke(void *context)
{
    (void)context;
    rc_read_resource_number();
}
static DWORD __cdecl resource_number_reset_boundary(void)
{
    recon_native_entry_finish(!rc_resource_parentheses&&!rc_resource_long_number&&
        rc_expression_parentheses==11&&rc_resource_token.unknown12==13);
}
static int resource_number_reset_contract(void)
{
    rc_resource_parentheses=5;rc_resource_long_number=7;
    rc_expression_parentheses=11;rc_resource_token.unknown12=13;
    return recon_native_at_entry(rc_parse_resource_expression,
        resource_number_reset_boundary,resource_number_invoke,NULL)==1;
}

static void overlong_resource_word(void)
{
    prepare_resource_name(257);
    rc_collect_resource_word(rc_resource_name);
}
static void foreign_resource_word(void)
{
    WCHAR text[4];
    rc_collect_resource_word(text);
}
static int resource_word_contract(void)
{
    unsigned i;
    prepare_resource_name(256);
    if(rc_collect_resource_word(rc_resource_name)!=rc_resource_name+256||
       rc_resource_name[256]||rc_resource_character!=L' '||
       rc_resource_number_character!=L'a')return 0;
    for(i=0;i<256;++i)if(rc_resource_name[i]!=L'a')return 0;
    rc_resource_buffer_index=0;rc_resource_character=L'9';
    rc_resource_read_buffer[0]=L',';rc_resource_read_buffer[1]=0;
    if(rc_collect_resource_word(rc_resource_name)!=rc_resource_name+1||
       rc_resource_name[0]!=L'9'||rc_resource_name[1]||
       rc_resource_character!=L',')return 0;
    return trapped(overlong_resource_word)&&trapped(foreign_resource_word);
}

static void resource_digit_call(void){rc_next_resource_digit();}
static int resource_digit_contract(void)
{
    WCHAR foreign[2]={L'x',0};
    unsigned i;
    rc_resource_name[0]=L'0';rc_resource_name[1]=L'x';
    rc_resource_name[2]=L'0';rc_resource_name[3]=L'9';rc_resource_name[4]=0;
    rc_resource_number_cursor=rc_resource_name+1;rc_resource_character=0x55;
    if(rc_next_resource_digit()!=L'x'||rc_resource_number_cursor!=rc_resource_name+2||
       rc_resource_number_character!=L'x')return 0;
    if(rc_next_resource_digit()!=L'0'||rc_next_resource_digit()!=L'9'||
       rc_next_resource_digit()!=0||rc_resource_number_cursor!=rc_resource_name+5||
       rc_resource_character!=0x55)return 0;
    if(!trapped(resource_digit_call))return 0;
    rc_resource_number_cursor=rc_resource_name;
    if(!trapped(resource_digit_call))return 0;
    rc_resource_number_cursor=(WCHAR *)((char *)rc_resource_name+3);
    if(!trapped(resource_digit_call))return 0;
    rc_resource_number_cursor=foreign;
    if(!trapped(resource_digit_call))return 0;
    for(i=0;i<257;++i)rc_resource_name[i]=L'a';
    rc_resource_number_cursor=rc_resource_name+1;
    return trapped(resource_digit_call);
}

static int resource_hexadecimal_contract(void)
{
    static const WCHAR mixed[]=L"0xfF",overflow[]=L"0x100000000",stop[]=L"0xG";
    memcpy(rc_resource_name,mixed,sizeof(mixed));
    rc_resource_number_cursor=rc_resource_name+2;rc_resource_number_character=L'x';
    rc_resource_token.unknown0=0x12345678;
    if(rc_parse_resource_hexadecimal()!=255||rc_resource_number_character||
       rc_resource_number_cursor!=rc_resource_name+5||
       rc_resource_name[2]!=L'f'||rc_resource_token.unknown0!=0x12345678)return 0;
    memcpy(rc_resource_name,overflow,sizeof(overflow));
    rc_resource_number_cursor=rc_resource_name+2;rc_resource_number_character=L'x';
    if(rc_parse_resource_hexadecimal()!=0||rc_resource_number_character||
       rc_resource_number_cursor!=rc_resource_name+12)return 0;
    memcpy(rc_resource_name,stop,sizeof(stop));
    rc_resource_number_cursor=rc_resource_name+2;rc_resource_number_character=L'x';
    return !rc_parse_resource_hexadecimal()&&rc_resource_number_character==L'G'&&
        rc_resource_number_cursor==rc_resource_name+3;
}

static int resource_operator_contract(void)
{
    struct {WCHAR value,tail;} output;
    output.value=0x1234;output.tail=0xabcd;
    rc_resource_character=L',';rc_resource_buffer_index=0;
    if(rc_read_resource_operator(&output.value)||output.value!=L','||
       output.tail!=0xabcd||rc_resource_buffer_index)return 0;
    rc_resource_character=L'+';rc_resource_line_start=0;
    rc_resource_read_buffer[0]=L' ';rc_resource_read_buffer[1]=L'a';
    rc_resource_read_buffer[2]=0;
    return rc_read_resource_operator(&output.value)==1&&output.value==L'+'&&
        output.tail==0xabcd&&rc_resource_character==L'a'&&
        rc_resource_buffer_index==2;
}

static void prepare_resource_punctuation(WCHAR character)
{
    rc_resource_character=character;rc_resource_line_start=0;
    rc_resource_buffer_index=0;rc_resource_read_buffer[0]=L'a';
    rc_resource_read_buffer[1]=0;rc_resource_token.value=0x1234;
}
static int resource_symbol_contract(void)
{
    RcResourcePunctuation saved=rc_resource_punctuation[0];
    int result;
    prepare_resource_punctuation(L',');
    if(rc_read_resource_symbol(0)!=1||rc_resource_token.value||
       rc_resource_token.kind!=15||rc_resource_character!=L'a')return 0;
    prepare_resource_punctuation(L',');
    rc_resource_punctuation[0].reserved2=0xabcd;
    rc_resource_punctuation[0].reserved5[0]=0xee;
    rc_resource_punctuation[0].kind=1;
    result=!rc_read_resource_symbol(0)&&!rc_resource_token.value&&
        rc_resource_token.kind==1&&rc_resource_character==L'a';
    rc_resource_punctuation[0]=saved;
    if(!result)return 0;
    prepare_resource_punctuation(L'(');
    return !rc_read_resource_symbol(0)&&!rc_resource_token.value&&
        rc_resource_token.kind==1&&rc_resource_character==L'a';
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
    {"substitution-invalid-index",1,substitute_invalid_index},
    {"identifier-lookahead-contract",1,identifier_lookahead_contract},
    {"interior-numeric-spelling-contract",1,interior_number_contract},
    {"quoted-output-chunk-contract",1,quoted_output_contract},
    {"raw-macro-argument-contract",1,raw_macro_argument_contract},
    {"expanded-quoted-escape-contract",1,expanded_quoted_contract},
    {"apstudio-service-hold",1,apstudio_hold},
    {"resource-input-initialization",1,resource_input_contract},
    {"command-resource-reset-owners",1,command_reset_contract},
    {"resource-type-lookup-contract",1,resource_type_lookup_contract},
    {"collected-resource-name-contract",1,collected_resource_name_contract},
    {"resource-number-reset-contract",1,resource_number_reset_contract},
    {"resource-word-contract",1,resource_word_contract},
    {"resource-digit-contract",1,resource_digit_contract},
    {"resource-hexadecimal-contract",1,resource_hexadecimal_contract},
    {"resource-operator-contract",1,resource_operator_contract},
    {"resource-symbol-contract",1,resource_symbol_contract},
    {"resource-read-buffer-contract",1,resource_buffer_contract},
    {"resource-byte-read-contract",1,byte_read_contract},
    {"packed-macro-parameter",1,parameters},
    {"old-no-space-before-unsupported-copy",1,no_space_guard}
};
