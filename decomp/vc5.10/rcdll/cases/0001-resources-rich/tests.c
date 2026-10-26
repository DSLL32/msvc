/* Retained production guard/layout tests; no reconstructed functions are replaced. */
#define RECON_SHIM_IMPL
#include <stddef.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
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
extern FILE *rc_preprocessed_file;
extern DWORD rc_resource_recording;
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
    rc_last_string_resource=(RcResource *)0x44444440;
    rc_verbose=1;rc_mac=1;rc_input_name[0]='q';rc_output_path[0]='q';
    rc_initialize();
}
static HANDLE WINAPI command_reset_boundary(DWORD flags,DWORD initial,DWORD maximum)
{
    recon_native_entry_finish(flags==1&&!initial&&!maximum&&
        rc_heap==(HANDLE)0x66666660&&!rc_result&&rc_resource_status==51&&!rc_codepage&&rc_language==128&&
        !rc_font_resource_count&&!rc_input_temporary&&!rc_output_temporary&&
        !rc_hwb_resource_offset&&!rc_resource_types&&
        rc_last_string_resource==(RcResource *)0x44444440&&
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

static int resource_decimal_id_contract(void)
{
    const WCHAR high_digit[]={0x0131,0},low_zero[]={0x0100,0},signed_byte[]={0x00ff,0};
    return rc_decimal_resource_id(L"101")==101&&!rc_decimal_resource_id(L"")&&
        !rc_decimal_resource_id(L"4294967296")&&
        rc_decimal_resource_id(high_digit)==1&&
        rc_decimal_resource_id(low_zero)==0xffffffd0u&&
        rc_decimal_resource_id(signed_byte)==0xffffffcfu&&
        high_digit[0]==0x0131&&low_zero[0]==0x0100&&signed_byte[0]==0x00ff;
}

static int resource_keyword_contract(void)
{
    const WCHAR mixed[]={L'm',L'o',L'V',L'e',L'a',L'b',L'l',L'e',0};
    /* BEDIT is deliberately shadowed by the original BEGIN/BEDIT order. */
    return rc_lookup_resource_keyword(mixed)==0xfff1&&mixed[2]==L'V'&&
        rc_lookup_resource_keyword(L"ALT")==110&&
        rc_lookup_resource_keyword(L"VIRTKEY")==112&&
        rc_lookup_resource_keyword(L"BEGIN")==13&&
        rc_lookup_resource_keyword(L"BEDIT")==0xffff&&
        rc_lookup_resource_keyword(L"MESAGETABLE")==96&&
        rc_lookup_resource_keyword(L"MESSAGETABLE")==0xffff&&
        rc_lookup_resource_keyword(L"")==0xffff&&
        rc_lookup_resource_keyword(L"ZZZ")==0xffff;
}

static int resource_attribute_contract(void)
{
    static const WORD tokens[]={0xfff0,0xfff1,0xfff2,0xfff3,0xfff4,0xfff5,0xfff6};
    static const WORD before[]={0xffff,0xa585,0xa585,0xffff,0xa585,0xa585,0xffff};
    static const WORD after[]={0xefef,0xa595,0xb5b5,0xffbf,0xa5c5,0xa5a5,0xefdf};
    RcResource resource,expected;
    unsigned i;
    for(i=0;i<7;++i){
        memset(&resource,0xa5,sizeof(resource));resource.flags=before[i];
        expected=resource;expected.flags=after[i];
        prepare_resource_punctuation(L',');
        rc_resource_token.value=tokens[i];rc_resource_token.kind=0;
        if(rc_read_resource_attributes(&resource)!=1||
           memcmp(&resource,&expected,sizeof(resource))||
           rc_resource_token.kind!=15||rc_resource_character!=L'a')return 0;
    }
    expected=resource;rc_resource_token.value=0xfff1;rc_resource_token.kind=20;
    if(rc_read_resource_attributes(&resource)||memcmp(&resource,&expected,sizeof(resource)))return 0;
    rc_resource_token.value=0xffff;rc_resource_token.kind=0;
    return !rc_read_resource_attributes(&resource)&&
        !memcmp(&resource,&expected,sizeof(resource));
}

extern DWORD rc_apstudio_size3,rc_apstudio_size4;
static void metadata_unobserved(void)
{
    rc_apstudio=1;
    rc_write_resource_metadata(NULL,NULL,0);
}
static int resource_metadata_contract(void)
{
    RcResource resource,saved;
    RcResourceType type,saved_type;
    memset(&resource,0xa5,sizeof(resource));saved=resource;
    memset(&type,0x5a,sizeof(type));saved_type=type;
    rc_apstudio=0;rc_apstudio_size3=17;rc_apstudio_size4=19;
    rc_write_resource_metadata(&resource,&type,0);
    rc_write_resource_metadata(&resource,&type,1);
    rc_write_resource_metadata(NULL,NULL,0);
    return rc_apstudio_size3==17&&rc_apstudio_size4==19&&
        !memcmp(&resource,&saved,sizeof(resource))&&
        !memcmp(&type,&saved_type,sizeof(type))&&trapped(metadata_unobserved);
}

static int resource_buffer_reset_contract(void)
{
    rc_resource_buffer_used=37;rc_resource_data_file=(FILE *)0x12345670;
    rc_resource_buffer_size=81;rc_resource_buffer=(BYTE *)0x34567890;
    rc_preprocessed_file=(FILE *)0x23456780;
    rc_resource_input_file=(FILE *)0x45678900;
    rc_reset_resource_buffer();
    return !rc_resource_buffer_used&&!rc_resource_data_file&&
        rc_resource_buffer_size==81&&rc_resource_buffer==(BYTE *)0x34567890&&
        rc_preprocessed_file==(FILE *)0x23456780&&
        rc_resource_input_file==(FILE *)0x45678900;
}

static void reserve_negative_cursor(void)
{
    rc_resource_buffer_used=-1;rc_reserve_resource_bytes(2);
}
static void reserve_past_capacity(void)
{
    rc_resource_buffer_used=rc_resource_buffer_size+1;rc_reserve_resource_bytes(2);
}
static void reserve_missing_owner(void)
{
    rc_resource_buffer=NULL;rc_reserve_resource_bytes(2);
}
static int resource_buffer_reserve_contract(void)
{
    BYTE *buffer;
    rc_heap=recon_HeapCreate(1,0,0);
    rc_initialize_resource_buffer();rc_reset_resource_buffer();
    buffer=rc_resource_buffer;
    buffer[0]=0x7a;buffer[127999]=0x5a;
    rc_resource_data_file=(FILE *)0x12345670;
    if(rc_reserve_resource_bytes(2)!=buffer||rc_resource_buffer_used!=2||
       rc_reserve_resource_bytes(7)!=buffer+2||rc_resource_buffer_used!=9)return 0;
    rc_resource_buffer_used=127998;
    if(rc_reserve_resource_bytes(2)!=buffer+127998||rc_resource_buffer_used!=128000||
       rc_reserve_resource_bytes(0)!=buffer+128000||rc_resource_buffer_used!=128000)return 0;
    return rc_resource_buffer==buffer&&rc_resource_buffer_size==128000&&
        buffer[0]==0x7a&&buffer[127999]==0x5a&&
        rc_resource_data_file==(FILE *)0x12345670&&
        trapped(reserve_negative_cursor)&&trapped(reserve_past_capacity)&&
        trapped(reserve_missing_owner);
}

static int resource_preamble_contract(void)
{
    const WCHAR input[]=L"0x20011 CHARACTERISTICS 0x30013 LANGUAGE 0x9,0x1 BEGIN END";
    RcResource resource,expected;
    memset(&resource,0xa5,sizeof(resource));expected=resource;
    expected.version=0x20011;expected.characteristics=0x30013;expected.language=0x409;
    memcpy(rc_resource_read_buffer,input+1,sizeof(input)-sizeof(input[0]));
    rc_resource_buffer_index=0;rc_resource_line_start=0;rc_resource_recording=0;
    rc_resource_character=input[0];rc_resource_token.kind=132;
    rc_read_resource_preamble(&resource,2121);
    return !memcmp(&resource,&expected,sizeof(resource))&&rc_resource_token.kind==14;
}

static int check_resource_string(const WCHAR *input,const WCHAR *expected,
                                 unsigned raw_count,unsigned result_count)
{
    RcResourceToken saved;
    unsigned i,units=0;
    while(input[units])++units;
    memcpy(rc_resource_read_buffer,input,(units+1)*sizeof(WCHAR));
    rc_resource_buffer_index=0;rc_resource_line_start=0;rc_resource_recording=0;
    rc_resource_character=L'"';
    memset(rc_resource_name,0xa5,sizeof(rc_resource_name));
    memset(&rc_resource_token,0xa5,sizeof(rc_resource_token));saved=rc_resource_token;
    saved.value=(WORD)result_count;saved.kind=23;saved.unknown19=21;
    rc_read_resource_string();
    if(memcmp(&saved,&rc_resource_token,sizeof(saved))||rc_resource_character!=L' '||
       memcmp(rc_resource_name,expected,(result_count+1)*sizeof(WCHAR)))return 0;
    for(i=raw_count+1;i<257;++i)if(rc_resource_name[i]!=0xa5a5)return 0;
    return 1;
}
static void resource_string_overflow(void)
{
    unsigned i;
    for(i=0;i<257;++i)rc_resource_read_buffer[i]=L'A';
    rc_resource_read_buffer[257]=L'"';rc_resource_read_buffer[258]=L' ';
    rc_resource_read_buffer[259]=0;
    rc_resource_buffer_index=0;rc_resource_line_start=0;rc_resource_recording=0;
    rc_resource_character=L'"';rc_read_resource_string();
}
static int resource_string_contract(void)
{
    extern int rc_mac;
    rc_mac=0;
    return check_resource_string(L"Menu\" ",L"Menu",4,4)&&
        check_resource_string(L"Cap\"\"tion\" ",L"Cap\"tion",8,8)&&
        check_resource_string(L"A\\\\B\\t\\a\\n\\r\\q\" ",L"A\\B\t\b\n\r\\q",14,9)&&
        trapped(resource_string_overflow);
}

static int menu_caption_contract(void)
{
    WCHAR destination[259],saved[259];
    unsigned i,j,lengths[]={0,5,256};
    RcResourceToken token;
    for(i=0;i<257;++i)rc_resource_name[i]=(WCHAR)(0x100+i);
    for(j=0;j<3;++j){
        memset(destination,0xa5,sizeof(destination));
        memset(&rc_resource_token,0x5a,sizeof(rc_resource_token));
        rc_resource_token.kind=j&1?21:23;
        rc_resource_token.value=(WORD)lengths[j];token=rc_resource_token;
        if(rc_copy_menu_caption(destination)!=1||
           memcmp(destination,rc_resource_name,(lengths[j]+1)*2)||
           memcmp(&token,&rc_resource_token,sizeof(token)))return 0;
        for(i=lengths[j]+1;i<259;++i)if(destination[i]!=0xa5a5)return 0;
    }
    memset(destination,0xa5,sizeof(destination));memcpy(saved,destination,sizeof(saved));
    rc_resource_token.kind=72;token=rc_resource_token;
    return !rc_copy_menu_caption(destination)&&!memcmp(destination,saved,sizeof(saved))&&
        !memcmp(&token,&rc_resource_token,sizeof(token));
}

static int menu_commas_contract(void)
{
    const WCHAR *inputs[]={L" BEGIN ",L",, BEGIN "};
    unsigned j,units;
    for(j=0;j<2;++j){
        units=0;while(inputs[j][units])++units;
        memcpy(rc_resource_read_buffer,inputs[j]+1,units*sizeof(WCHAR));
        rc_resource_character=inputs[j][0];rc_resource_buffer_index=0;
        rc_resource_line_start=0;rc_resource_recording=0;
        rc_menu_had_comma=37;
        if(rc_scan_menu_commas()!=13||rc_resource_token.kind!=13||
           rc_menu_had_comma!=(int)j)return 0;
    }
    return 1;
}

static int menu_attribute_contract(void)
{
    unsigned kinds[]={50,67,73,74,75,76,77,79};
    WORD bits[]={0x100,4,8,1,2,0x20,0x40,0x4000};
    RcMenuItem item,expected;
    unsigned i;
    for(i=0;i<8;++i){
        memset(&item,0xa5,sizeof(item));
        item.flags=(WORD)~bits[i];expected=item;expected.flags|=bits[i];
        if(rc_parse_menu_attribute(kinds[i],&item)!=1||
           memcmp(&item,&expected,sizeof(item)))return 0;
    }
    memset(&item,0xa5,sizeof(item));expected=item;
    return !rc_parse_menu_attribute(0x10032,&item)&&
        !memcmp(&item,&expected,sizeof(item))&&!rc_parse_menu_attribute(78,NULL);
}

static RcMenuItem writer_item,writer_saved;
static BYTE writer_buffer[20],writer_expected[20];
static int writer_used;
static void __stdcall menu_writer_boundary(const WCHAR *text,int flags)
{
    recon_native_entry_finish(text==writer_item.caption&&flags==1&&
        rc_resource_buffer_used==writer_used&&
        !memcmp(writer_buffer,writer_expected,sizeof(writer_buffer))&&
        !memcmp(&writer_item,&writer_saved,sizeof(writer_item)));
}
static void menu_writer_invoke(void *context)
{
    (void)context;rc_write_menu_item(&writer_item);
}
static int menu_writer_prefix_contract(void)
{
    extern int rc_mac;
    unsigned j;
    rc_mac=0;
    for(j=0;j<2;++j){
        memset(&writer_item,0xa5,sizeof(writer_item));
        writer_item.id=0x1234;writer_item.flags=j?4:0x10;writer_saved=writer_item;
        memset(writer_buffer,0x5a,sizeof(writer_buffer));
        memcpy(writer_expected,writer_buffer,sizeof(writer_buffer));
        writer_expected[4]=(BYTE)writer_item.flags;writer_expected[5]=0;
        writer_used=j?8:6;
        if(j){writer_expected[6]=0x34;writer_expected[7]=0x12;}
        rc_resource_buffer=writer_buffer;rc_resource_buffer_size=20;
        rc_resource_buffer_used=4;
        if(recon_native_at_entry(rc_write_resource_string,menu_writer_boundary,
                                menu_writer_invoke,NULL)!=1)return 0;
    }
    return 1;
}

static int resource_string_write_contract(void)
{
    const WCHAR text[]={L'A',0x1234,0},empty[]={0};
    BYTE output[40],expected[40];
    RcMenuItem item;
    extern int rc_mac;
    rc_mac=0;
    memset(output,0x5a,sizeof(output));memcpy(expected,output,sizeof(output));
    memcpy(expected+4,text,sizeof(text));
    rc_resource_buffer=output;rc_resource_buffer_size=40;rc_resource_buffer_used=4;
    rc_write_resource_string(text,0);
    if(rc_resource_buffer_used!=10||memcmp(output,expected,sizeof(output)))return 0;
    rc_write_resource_string(empty,1);expected[10]=0;expected[11]=0;
    if(rc_resource_buffer_used!=12||memcmp(output,expected,sizeof(output)))return 0;
    memset(&item,0xa5,sizeof(item));item.id=0x1234;item.flags=4;
    memcpy(item.caption,text,sizeof(text));
    expected[12]=4;expected[13]=0;expected[14]=0x34;expected[15]=0x12;
    memcpy(expected+16,text,sizeof(text));
    return rc_write_menu_item(&item)==12&&rc_resource_buffer_used==22&&
        !memcmp(output,expected,sizeof(output));
}

static int resource_decimal_contract(void)
{
    const WCHAR *inputs[]={L"0!",L"101x",L"4294967295!",L"4294967296!",L"Q"};
    DWORD values[]={0,101,0xffffffff,0,0};
    unsigned lengths[]={1,3,10,10,0};
    WCHAR saved[257];
    unsigned i,n;
    for(i=0;i<5;++i){
        n=0;while(inputs[i][n])++n;
        memset(rc_resource_name,0xa5,sizeof(rc_resource_name));
        memcpy(rc_resource_name,inputs[i],(n+1)*2);
        memcpy(saved,rc_resource_name,sizeof(saved));
        rc_resource_number_character=rc_resource_name[0];
        rc_resource_number_cursor=rc_resource_name+1;
        if(rc_parse_resource_decimal()!=values[i]||
           rc_resource_number_character!=inputs[i][lengths[i]]||
           rc_resource_number_cursor!=rc_resource_name+lengths[i]+1||
           memcmp(saved,rc_resource_name,sizeof(saved)))return 0;
    }
    return 1;
}

static void apstudio_name_unobserved(void)
{
    rc_apstudio=1;rc_write_apstudio_name(NULL);
}
static int apstudio_name_contract(void)
{
    RcResourceToken saved;
    memset(&rc_resource_token,0xa5,sizeof(rc_resource_token));saved=rc_resource_token;
    rc_apstudio=0;rc_apstudio_size3=37;
    rc_write_apstudio_name(NULL);
    rc_write_apstudio_name(rc_resource_token.text);
    return rc_apstudio_size3==37&&
        !memcmp(&saved,&rc_resource_token,sizeof(saved))&&
        trapped(apstudio_name_unobserved);
}

static DWORD style_storage,style_initial;
static unsigned style_flags;
static void __stdcall style_expression_boundary(DWORD *value,int parenthesized)
{
    recon_native_entry_finish(!parenthesized&&*value==style_initial&&
                              style_storage==0xa5a51234);
}
static void style_invoke(void *context)
{
    (void)context;rc_parse_resource_style(&style_storage,style_flags);
}
static int resource_style_initial_contract(void)
{
    unsigned i,flags[]={0,1,2,3,0x10002};
    DWORD values[]={0xa5a51234,0,0x1234,0,0x1234};
    for(i=0;i<5;++i){
        style_storage=0xa5a51234;style_flags=flags[i];style_initial=values[i];
        if(recon_native_at_entry(rc_parse_resource_style_expression,
                                style_expression_boundary,style_invoke,NULL)!=1)return 0;
    }
    return 1;
}

/* Seed an already-lexed numeric token for isolated style operations. */
static void style_integer(DWORD number)
{
    rc_resource_read_buffer[0]=L' ';rc_resource_read_buffer[1]=0;
    rc_resource_character=L',';rc_resource_buffer_index=0;
    rc_resource_line_start=0;rc_resource_recording=0;
    rc_resource_token.kind=20;rc_resource_token.unknown0=number;
    rc_resource_name[0]=L'1';rc_resource_name[1]=0;
}
static int resource_style_contract(void)
{
    DWORD value;
    unsigned flags,i;
    WORD operations[]={12,17,101,102,0x1234};
    DWORD results[]={11,2,13,7,10};
    for(flags=0;flags<4;++flags){
        value=0xa5a51234;style_integer(1);
        if(rc_parse_resource_style(&value,flags)!=1||rc_resource_token.kind!=15||
           value!=(flags==0?0xa5a51235:flags==1?1:flags==2?0xa5a51235:0xa5a50001))return 0;
    }
    for(i=0;i<5;++i){
        value=10;style_integer(3);
        if(rc_apply_resource_style_term(&value,operations[i])!=1||
           value!=results[i]||rc_resource_token.kind!=15)return 0;
    }
    value=0x5a5a1234;rc_resource_token.kind=15;
    return !rc_apply_resource_style_term(&value,12)&&value==0x5a5a1234;
}

static void menu_marker_outside_prefix(void)
{
    rc_mark_last_menu_item(16);
}
static void menu_marker_high_byte_outside_prefix(void)
{
    extern int rc_mac;
    rc_mac=1;rc_mark_last_menu_item(15);
}
static int menu_end_marker_contract(void)
{
    BYTE output[16],expected[16];
    extern int rc_mac;
    unsigned j;
    for(j=0;j<2;++j){
        memset(output,0x21,sizeof(output));memcpy(expected,output,sizeof(output));
        expected[4+j]|=0x80;rc_mac=j;
        rc_resource_buffer=output;rc_resource_buffer_size=16;rc_resource_buffer_used=16;
        rc_mark_last_menu_item((WORD)0x10004);
        if(memcmp(output,expected,sizeof(output))||rc_resource_buffer_used!=16)return 0;
    }
    return trapped(menu_marker_outside_prefix)&&
        trapped(menu_marker_high_byte_outside_prefix);
}

static void __cdecl resource_count_reset_boundary(void)
{
    recon_native_entry_finish(!rc_resource_count&&!rc_font_resource_count&&
                              rc_resource_version==0x12345678);
}
static void resource_count_reset_invoke(void *context)
{
    (void)context;rc_parse_resources();
}
static int resource_count_reset_contract(void)
{
    rc_resource_count=0x1357;rc_font_resource_count=0x2468;
    rc_resource_version=0x12345678;
    return recon_native_at_entry(rc_initialize_resource_types,
             resource_count_reset_boundary,resource_count_reset_invoke,NULL)==1;
}

static int output_alignment_contract(void)
{
    ShimFILE *file=recon_fopen("alignment-unit.bin","w+b");
    FILE *check;
    BYTE bytes[5];
    int result;
    if(!file)return 0;
    if(rc_align_output((FILE *)file)||rc_write_bytes((FILE *)file,"abc",3)!=3||
       rc_seek((FILE *)file,0,SEEK_CUR)!=3||rc_align_output((FILE *)file)!=1||
       rc_seek((FILE *)file,0,SEEK_CUR)!=4||rc_align_output((FILE *)file)||
       rc_seek((FILE *)file,1,SEEK_SET)!=1||rc_align_output((FILE *)file)!=3||
       rc_seek((FILE *)file,0,SEEK_CUR)!=4||recon_fclose(file))return 0;
    check=fopen("alignment-unit.bin","rb");
    if(!check)return 0;
    result=fread(bytes,1,sizeof(bytes),check)==4&&bytes[0]=='a'&&
        !bytes[1]&&!bytes[2]&&!bytes[3];
    fclose(check);unlink("alignment-unit.bin");
    return result;
}

static RcResourceType append_type,append_type_expected;
static RcResource append_nodes[3],append_expected[3];
static BYTE append_bytes[5];
static void __stdcall append_record_boundary(RcResourceType *type,RcResource *resource,
                                             const void *data,int bytes,int file_bytes)
{
    recon_native_entry_finish(type==&append_type&&resource==&append_nodes[2]&&
        data==append_bytes&&bytes==5&&file_bytes==-1&&
        !memcmp(&append_type,&append_type_expected,sizeof(append_type))&&
        !memcmp(append_nodes,append_expected,sizeof(append_nodes))&&
        rc_resource_count==37&&rc_font_resource_count==99);
}
static void append_invoke(void *context)
{
    (void)context;rc_append_resource(&append_type,&append_nodes[2],append_bytes,5,-1);
}
static int resource_append_prefix_contract(void)
{
    unsigned j;
    for(j=0;j<2;++j){
        memset(&append_type,0xa5,sizeof(append_type));
        memset(append_nodes,0x5a,sizeof(append_nodes));
        append_nodes[0].next=&append_nodes[1];append_nodes[1].next=NULL;
        append_nodes[2].next=NULL;
        append_type.resources=j?&append_nodes[0]:NULL;append_type.count=17;
        append_type_expected=append_type;
        memcpy(append_expected,append_nodes,sizeof(append_nodes));
        if(j)append_expected[1].next=&append_nodes[2];
        else append_type_expected.resources=&append_nodes[2];
        rc_resource_count=37;rc_font_resource_count=99;
        if(recon_native_at_entry(rc_write_resource_record,append_record_boundary,
                                append_invoke,NULL)!=1)return 0;
    }
    return 1;
}

static void verbose_resource_record(void)
{
    RcResourceType type;
    RcResource resource;
    memset(&type,0xa5,sizeof(type));memset(&resource,0x5a,sizeof(resource));
    type.id=4;resource.id=1;resource.size=1;
    rc_verbose=1;rc_write_resource_record(&type,&resource,NULL,0,-1);
}
static int resource_record_verbose_hold(void)
{
    return trapped(verbose_resource_record);
}

static int resource_data_size_contract(void)
{
    struct stat info;
    ShimFILE *file;
    DWORD size;
    rc_resource_data_file=NULL;rc_resource_buffer_used=37;
    if(rc_resource_data_size()!=37||rc_resource_buffer_used!=37)return 0;
    rc_resource_buffer_used=-1;
    if(rc_resource_data_size()!=0xffffffff)return 0;
    if(stat("resources.rc",&info)||info.st_size<2||info.st_size>0x7ffffff0)return 0;
    file=recon_fopen("resources.rc","rb");if(!file)return 0;
    if(rc_seek((FILE *)file,2,SEEK_SET)!=2)return 0;
    rc_resource_data_file=(FILE *)file;rc_resource_buffer_used=5;
    size=rc_resource_data_size();
    if(size!=(DWORD)info.st_size+3||rc_resource_data_file!=(FILE *)file||
       rc_resource_buffer_used!=5||recon_ftell(file)!=2)return 0;
    return !recon_fclose(file);
}

static int resource_identifier_write_contract(void)
{
    const BYTE expected[]={0xff,0xff,0x34,0x12,0x41,0,0x42,0,0,0,0xff,0xff,0xff,0xff};
    BYTE bytes[15];
    ShimFILE *file=recon_fopen("identifier-unit.bin","w+b");
    FILE *check;
    int result;
    extern FILE *rc_output_file;
    if(!file)return 0;rc_output_file=(FILE *)file;
    if(rc_write_resource_identifier(0x1234,NULL)!=2||
       rc_write_resource_identifier(0,L"AB")!=6||
       rc_write_resource_identifier(0xffff,NULL)!=2||recon_fclose(file))return 0;
    check=fopen("identifier-unit.bin","rb");if(!check)return 0;
    result=fread(bytes,1,sizeof(bytes),check)==sizeof(expected)&&
        !memcmp(bytes,expected,sizeof(expected));
    fclose(check);unlink("identifier-unit.bin");return result;
}

static int resource_payload_contract(void)
{
    BYTE bytes[4];
    ShimFILE *file=recon_fopen("payload-unit.bin","w+b");
    FILE *check;
    int result;
    if(!file)return 0;
    rc_resource_data_file=NULL;
    if(rc_write_resource_payload((FILE *)file,"abc",3,-1)!=3||
       rc_write_resource_payload((FILE *)file,NULL,0,17)||
       rc_write_resource_payload((FILE *)file,NULL,-2,-1)!=-2||
       rc_resource_data_file||recon_fclose(file))return 0;
    check=fopen("payload-unit.bin","rb");if(!check)return 0;
    result=fread(bytes,1,sizeof(bytes),check)==3&&!memcmp(bytes,"abc",3);
    fclose(check);unlink("payload-unit.bin");return result;
}

static RcDialog dialog_initial,dialog_expected;
static RcResource dialog_resource,dialog_resource_saved;
static void __stdcall dialog_rectangle_boundary(WORD *x,WORD *y,WORD *width,WORD *height)
{
    struct DialogAlignment {BYTE before;RcDialog value;};
    recon_native_entry_finish(x==&dialog_initial.x&&y==&dialog_initial.y&&
        width==&dialog_initial.width&&height==&dialog_initial.height&&
        !memcmp(&dialog_initial,&dialog_expected,sizeof(dialog_initial))&&
        !memcmp(&dialog_resource,&dialog_resource_saved,sizeof(dialog_resource))&&
        offsetof(struct DialogAlignment,value)==2);
}
static void dialog_initial_invoke(void *context)
{
    (void)context;rc_parse_dialog(&dialog_resource,&dialog_initial,0);
}
static int dialog_initial_contract(void)
{
    memset(&dialog_initial,0xa5,sizeof(dialog_initial));dialog_expected=dialog_initial;
    memset(&dialog_resource,0x5a,sizeof(dialog_resource));
    dialog_resource.unknown8=0x12345678;dialog_resource_saved=dialog_resource;
    dialog_expected.style=0x80880000;dialog_expected.exstyle=0x12345678;
    dialog_expected.menu[0]=0;dialog_expected.caption[0]=0;
    dialog_expected.class_name[0]=0;dialog_expected.menu_is_ordinal=0;
    dialog_expected.class_is_ordinal=0;dialog_expected.point_size=0;
    return recon_native_at_entry(rc_read_dialog_rectangle,dialog_rectangle_boundary,
                                 dialog_initial_invoke,NULL)==1;
}

static int dialog_coordinate_contract(void)
{
    DWORD inputs[]={0,0x12345678,0xffffffff};
    unsigned i;
    for(i=0;i<3;++i){
        style_integer(inputs[i]);
        if(rc_read_dialog_coordinate()!=(WORD)inputs[i]||
           rc_resource_token.kind!=15)return 0;
    }
    return 1;
}

static int dialog_rectangle_contract(void)
{
    struct {WORD before,x,y,width,height,after;} rectangle;
    static const WCHAR rest[]=L"2,300,4 BEGIN ";
    memset(&rectangle,0xa5,sizeof(rectangle));
    style_integer(0x10001);
    memcpy(rc_resource_read_buffer,rest,sizeof(rest));
    rc_read_dialog_rectangle(&rectangle.x,&rectangle.y,
                             &rectangle.width,&rectangle.height);
    return rectangle.before==0xa5a5&&rectangle.after==0xa5a5&&
        rectangle.x==1&&rectangle.y==2&&rectangle.width==300&&
        rectangle.height==4&&rc_resource_token.kind==13;
}

static RcDialog attribute_dialog,attribute_expected;
static RcResource attribute_resource,attribute_resource_saved;
static WORD __stdcall dialog_style_boundary(WORD flags)
{
    recon_native_entry_finish(flags==0x8000&&
        !memcmp(&attribute_dialog,&attribute_expected,sizeof(attribute_dialog))&&
        !memcmp(&attribute_resource,&attribute_resource_saved,sizeof(attribute_resource)));
    return 0;
}
static void dialog_attribute_invoke(void *context)
{
    (void)context;rc_parse_dialog_attributes(&attribute_resource,&attribute_dialog,0);
}
static int dialog_attribute_style_contract(void)
{
    DWORD initial[]={0xffffffff,0x80880000,0x00400000};
    unsigned i;
    for(i=0;i<3;++i){
        memset(&attribute_dialog,0xa5,sizeof(attribute_dialog));
        memset(&attribute_resource,0x5a,sizeof(attribute_resource));
        attribute_resource_saved=attribute_resource;
        attribute_dialog.style=initial[i];attribute_expected=attribute_dialog;
        attribute_expected.style=i==0?0xc00000:0;
        rc_resource_token.kind=133;
        if(recon_native_at_entry(rc_scan_resource_value,dialog_style_boundary,
                                 dialog_attribute_invoke,NULL)!=1)return 0;
    }
    return 1;
}

static int dialog_caption_contract(void)
{
    RcDialog dialog,expected;
    static const WCHAR rest[]=L"X\" BEGIN ";
    memset(&dialog,0xa5,sizeof(dialog));expected=dialog;
    expected.caption[0]=L'X';expected.caption[1]=0;expected.style|=0xc00000;
    memcpy(rc_resource_read_buffer,rest,sizeof(rest));
    rc_resource_character=L'"';rc_resource_buffer_index=0;
    rc_resource_line_start=0;rc_resource_recording=0;
    rc_resource_token.kind=135;
    rc_parse_dialog_caption(&dialog);
    return rc_resource_token.kind==13&&!memcmp(&dialog,&expected,sizeof(dialog));
}

static int dialog_font_contract(void)
{
    RcDialog dialog,expected;
    static const WCHAR rest[]=L", \"Arial\" BEGIN ";
    memset(&dialog,0xa5,sizeof(dialog));expected=dialog;
    expected.point_size=8;expected.charset=1;
    memcpy(expected.font,L"Arial",12);
    memcpy(rc_resource_read_buffer,rest,sizeof(rest));
    rc_resource_character=L'8';rc_resource_buffer_index=0;
    rc_resource_line_start=0;rc_resource_recording=0;
    rc_special_file[0]=0;rc_special_type_count=0;
    rc_parse_dialog_font(&dialog,0);
    return rc_resource_token.kind==13&&!memcmp(&dialog,&expected,sizeof(dialog));
}

static int dialog_menu_contract(void)
{
    RcDialog dialog,expected;
    static const WCHAR rest[]=L"01 BEGIN ";
    memset(&dialog,0xa5,sizeof(dialog));expected=dialog;
    memcpy(expected.menu,L"101",8);expected.menu_is_ordinal=1;
    memcpy(rc_resource_read_buffer,rest,sizeof(rest));
    rc_resource_character=L'1';rc_resource_buffer_index=0;
    rc_resource_line_start=0;rc_resource_recording=0;
    rc_resource_token.value=3;
    rc_parse_dialog_menu(&dialog);
    return rc_resource_token.kind==13&&!memcmp(&dialog,&expected,sizeof(dialog));
}

static int dialog_header_contract(void)
{
    RcDialog dialog,saved;
    BYTE output[64],expected[64];
    DWORD style=0x12345678,exstyle=0x87654321;
    WORD suffix[]={0,1,2,3,4,0,0,0};
    extern int rc_mac;
    memset(&dialog,0xa5,sizeof(dialog));dialog.style=style;dialog.exstyle=exstyle;
    dialog.x=1;dialog.y=2;dialog.width=3;dialog.height=4;
    dialog.menu_is_ordinal=0;dialog.class_is_ordinal=0;
    dialog.menu[0]=0;dialog.class_name[0]=0;dialog.caption[0]=0;dialog.point_size=0;
    saved=dialog;memset(output,0x5a,sizeof(output));memcpy(expected,output,sizeof(output));
    memcpy(expected,&style,4);memcpy(expected+4,&exstyle,4);
    memcpy(expected+8,suffix,sizeof(suffix));
    rc_resource_buffer=output;rc_resource_buffer_size=64;rc_resource_buffer_used=0;
    rc_dialog_count_offset=0xdeadbeef;rc_mac=0;
    rc_write_dialog_header(&dialog,0);
    if(rc_resource_buffer_used!=24||rc_dialog_count_offset!=8||
       memcmp(output,expected,sizeof(output))||memcmp(&dialog,&saved,sizeof(dialog)))return 0;
    dialog.menu_is_ordinal=1;memcpy(dialog.menu,L"101",8);
    dialog.point_size=8;memcpy(dialog.font,L"Arial",12);saved=dialog;
    memset(output,0x5a,sizeof(output));memcpy(expected,output,sizeof(output));
    memcpy(expected,&style,4);memcpy(expected+4,&exstyle,4);
    memcpy(expected+8,suffix,10);
    expected[18]=0xff;expected[19]=0xff;expected[20]=101;expected[21]=0;
    memset(expected+22,0,4);expected[26]=8;expected[27]=0;
    memcpy(expected+28,L"Arial",12);
    rc_resource_buffer_used=0;
    rc_write_dialog_header(&dialog,0);
    return rc_resource_buffer_used==40&&rc_dialog_count_offset==8&&
        !memcmp(output,expected,sizeof(output))&&!memcmp(&dialog,&saved,sizeof(dialog));
}

static void __stdcall control_initial_boundary(RcControl *control)
{
    recon_native_entry_finish(!control->text_is_ordinal&&!control->text[0]&&
        !control->id&&control->class_name[0]==0xffff&&control->class_name[1]==0x82&&
        control->style==0x50020000&&!control->exstyle&&!control->help_id&&
        !rc_dialog_id_count&&rc_dialog_id_capacity==100&&rc_dialog_ids!=NULL);
}
static void control_initial_invoke(void *context)
{
    (void)context;rc_parse_dialog_controls(0);
}
static int dialog_control_initial_contract(void)
{
    static const WCHAR rest[]=L"TEXT \"Title\",100,1,2,3,4 END ";
    extern int rc_mac;
    int result;
    memcpy(rc_resource_read_buffer,rest,sizeof(rest));
    rc_resource_character=L'L';rc_resource_buffer_index=0;
    rc_resource_line_start=0;rc_resource_recording=0;rc_mac=0;
    rc_resource_token.kind=13;
    rc_dialog_id_count=17;rc_dialog_id_capacity=3;rc_dialog_ids=NULL;
    rc_heap=recon_HeapCreate(1,0,0);
    result=recon_native_at_entry(rc_read_control_text,control_initial_boundary,
                                 control_initial_invoke,NULL);
    rc_release(rc_dialog_ids);
    return result==1;
}

static int control_text_contract(void)
{
    RcControl control,expected;
    static const WCHAR rest[]=L"Title\",100,1,2,3,4 END ";
    memset(&control,0xa5,sizeof(control));expected=control;
    memcpy(expected.text,L"Title",12);expected.text_is_ordinal=0;
    memcpy(rc_resource_read_buffer,rest,sizeof(rest));
    rc_resource_character=L'"';rc_resource_buffer_index=0;
    rc_resource_line_start=0;rc_resource_recording=0;
    rc_resource_token.text[0]=0x1234;rc_resource_token.unknown1038=0x5678;
    rc_read_control_text(&control);
    return !memcmp(&control,&expected,sizeof(control))&&
        !rc_resource_token.text[0]&&!rc_resource_token.unknown1038&&
        rc_resource_token.kind==23;
}

static int control_id_contract(void)
{
    RcControl control,expected;
    static const WCHAR rest[]=L"00, BEGIN ";
    DWORD *old_ids;
    unsigned i;
    extern int rc_apstudio;
    int result;
    memset(&control,0xa5,sizeof(control));control.id=0xa5a50000;expected=control;
    expected.id=0xa5a50064;
    memcpy(rc_resource_read_buffer,rest,sizeof(rest));
    rc_resource_character=L'1';rc_resource_buffer_index=0;
    rc_resource_line_start=0;rc_resource_recording=0;rc_apstudio=0;
    rc_heap=recon_HeapCreate(1,0,0);
    old_ids=rc_allocate(400);for(i=0;i<100;++i)old_ids[i]=i;
    rc_dialog_ids=old_ids;rc_dialog_id_count=100;rc_dialog_id_capacity=100;
    rc_read_control_id(&control,0);
    result=!memcmp(&control,&expected,sizeof(control))&&rc_resource_token.kind==13&&
        rc_dialog_id_count==101&&rc_dialog_id_capacity==200&&rc_dialog_ids!=old_ids&&
        rc_dialog_ids[100]==0xa5a50064;
    for(i=0;i<100;++i)if(rc_dialog_ids[i]!=i)result=0;
    rc_release(rc_dialog_ids);
    return result;
}

static int resource_buffer_padding_contract(void)
{
    BYTE output[16],expected[16];
    unsigned used,i;
    for(used=0;used<4;++used){
        memset(output,0xa5,sizeof(output));memcpy(expected,output,sizeof(output));
        for(i=used;i<used*2;++i)expected[i]=0;
        rc_resource_buffer=output;rc_resource_buffer_size=16;rc_resource_buffer_used=used;
        rc_pad_resource_buffer();
        if(rc_resource_buffer_used!=(int)(used*2)||memcmp(output,expected,sizeof(output)))return 0;
    }
    return 1;
}

static void dialog_count_outside_prefix(void)
{
    rc_dialog_count_offset=64;rc_increment_dialog_count();
}
static int control_record_contract(void)
{
    RcControl control,saved;
    BYTE output[64],expected[64];
    static const BYTE normal[]={0x44,0x33,0x22,0x11,0x88,0x77,0x66,0x55,
        1,0,2,0,3,0,4,0,0x42,0xef,0xff,0xff,0x82,0,L'O',0,L'K',0,0,0,0,0};
    static const BYTE extended[]={0xef,0xbe,0xad,0xde,0x88,0x77,0x66,0x55,
        0x44,0x33,0x22,0x11,1,0,2,0,3,0,4,0,0x42,0xef,0xcd,0xab,
        0xff,0xff,0x82,0,L'O',0,L'K',0,0,0,0,0};
    unsigned mode;
    extern int rc_mac;
    memset(&control,0xa5,sizeof(control));
    control.style=0x11223344;control.exstyle=0x55667788;control.help_id=0xdeadbeef;
    control.x=1;control.y=2;control.width=3;control.height=4;control.id=0xabcdef42;
    control.class_name[0]=0xffff;control.class_name[1]=0x82;
    control.text_is_ordinal=0;memcpy(control.text,L"OK",6);saved=control;
    rc_mac=0;
    for(mode=0;mode<2;++mode){
        memset(output,0x5a,sizeof(output));memcpy(expected,output,sizeof(output));
        output[0]=output[1]=0xff;memset(expected,0,4);
        memcpy(expected+4,mode?extended:normal,mode?sizeof(extended):sizeof(normal));
        rc_resource_buffer=output;rc_resource_buffer_size=64;rc_resource_buffer_used=2;
        rc_dialog_count_offset=0;rc_control_data_offset=0xdeadbeef;
        rc_write_control(&control,mode);
        if(rc_resource_buffer_used!=(mode?40:34)||
           rc_control_data_offset!=(mode?38:0xdeadbeef)||
           memcmp(output,expected,sizeof(output))||memcmp(&control,&saved,sizeof(control)))return 0;
    }
    return trapped(dialog_count_outside_prefix);
}

static int generic_control_contract(void)
{
    RcControl control,expected;
    static const WCHAR upper[]=L"Generic\",100,\"STATIC\",0x00800001,1,2,3,4 END ";
    static const WCHAR lower[]=L"Generic\",100,\"static\",0x00800001,1,2,3,4 END ";
    static const WCHAR other[]=L"Generic\",100,\"Custom\",0x00800001,1,2,3,4 END ";
    const WCHAR *inputs[]={upper,lower,other};
    unsigned i,n;
    extern int rc_apstudio;
    int result=1;
    rc_heap=recon_HeapCreate(1,0,0);rc_dialog_ids=rc_allocate(400);
    rc_dialog_id_capacity=100;rc_apstudio=0;
    for(i=0;i<3;++i){
        memset(&control,0xa5,sizeof(control));control.style=0x50000000;
        control.exstyle=0;control.help_id=0;control.id=0;expected=control;
        memcpy(expected.text,L"Generic",16);expected.text_is_ordinal=0;expected.id=100;
        expected.style=0x50800001;expected.x=1;expected.y=2;expected.width=3;expected.height=4;
        if(i<2){expected.class_name[0]=0xffff;expected.class_name[1]=0x82;}
        else memcpy(expected.class_name,L"Custom",14);
        n=0;while(inputs[i][n])++n;
        memcpy(rc_resource_read_buffer,inputs[i],(n+1)*2);
        rc_resource_character=L'"';rc_resource_buffer_index=0;
        rc_resource_line_start=0;rc_resource_recording=0;rc_dialog_id_count=0;
        rc_parse_generic_control(&control,0);
        if(rc_resource_token.kind!=14||memcmp(&control,&expected,sizeof(control))||
           rc_dialog_id_count!=1||rc_dialog_ids[0]!=100)result=0;
    }
    rc_release(rc_dialog_ids);return result;
}

static void wide_resource_radix_unsupported(void)
{
    WCHAR text[16];rc_format_wide_resource_number(16,text,16);
}
static int wide_resource_number_contract(void)
{
    DWORD values[]={0,1,65535,0xffffffff};
    const WCHAR *answers[]={L"",L"1",L"65535",L"4294967295"};
    WCHAR output[16],expected[16];
    unsigned i,n;
    for(i=0;i<4;++i){
        memset(output,0xa5,sizeof(output));memcpy(expected,output,sizeof(output));
        n=0;while(answers[i][n])++n;
        memcpy(expected,answers[i],(n+1)*2);
        if(rc_format_wide_resource_number(values[i],output,10)!=output||
           memcmp(output,expected,sizeof(output)))return 0;
    }
    return trapped(wide_resource_radix_unsupported);
}

static int string_table_contract(void)
{
    RcResource resource,saved;
    RcStringBlock *first,*second,*block,*next;
    static const WCHAR input[]=L"6 \"First\" 17 \"Second\" 32 \"Third\" END ";
    static const WCHAR translated[]=L"6 \"Bonjour\" END ";
    unsigned i;
    int result;
    memset(&resource,0xa5,sizeof(resource));resource.language=0x409;
    resource.flags=0x1030;resource.version=0x12345678;
    resource.characteristics=0x87654321;saved=resource;
    rc_heap=recon_HeapCreate(1,0,0);
    rc_last_string_resource=NULL;rc_string_blocks=(RcStringBlock *)0x33333330;
    memcpy(rc_resource_read_buffer,input,sizeof(input));
    rc_resource_character=L'1';rc_resource_buffer_index=0;
    rc_resource_line_start=0;rc_resource_recording=0;rc_resource_token.kind=13;
    if(rc_parse_string_table(&resource)!=&resource)return 0;
    first=rc_string_blocks;second=first?first->next:NULL;
    result=first&&second&&!second->next&&first->block==1&&second->block==2&&
        first->language==0x409&&first->flags==0x1030&&first->version==0x12345678&&
        first->characteristics==0x87654321&&rc_last_string_resource==&resource&&
        rc_resource_token.kind==14&&!memcmp(&resource,&saved,sizeof(resource))&&
        !memcmp(first->strings[0],L"First\0",14)&&
        !memcmp(first->strings[1],L"Second\0",16)&&
        !memcmp(second->strings[0],L"Third\0",14);
    if(!result)return 0;
    for(i=2;i<16;++i)if(first->strings[i]||first->prefixes[i])return 0;
    for(i=1;i<16;++i)if(second->strings[i]||second->prefixes[i])return 0;
    if(!first->prefixes[0]||!first->prefixes[1]||!second->prefixes[0])return 0;
    resource.language=0x40c;
    memcpy(rc_resource_read_buffer,translated,sizeof(translated));
    rc_resource_character=L'1';rc_resource_buffer_index=0;rc_resource_token.kind=13;
    if(rc_parse_string_table(&resource)!=&resource||rc_string_blocks!=first||
       !second->next||second->next->next||second->next->block!=1||
       second->next->language!=0x40c||
       memcmp(second->next->strings[0],L"Bonjour\0",18))return 0;
    block=first;
    while(block){
        next=block->next;
        for(i=0;i<16;++i){
            if(block->strings[i])rc_release(block->strings[i]);
            if(block->prefixes[i])rc_release(block->prefixes[i]);
        }
        rc_release(block);block=next;
    }
    return 1;
}

static DWORD buffer_word_offset;
static void buffer_word_store_invalid(void)
{
    rc_set_resource_buffer_word(buffer_word_offset,0x1234);
}
static int buffer_word_store_contract(void)
{
    BYTE output[16],expected[16];
    memset(output,0xa5,sizeof(output));memcpy(expected,output,sizeof(output));
    expected[4]=0x34;expected[5]=0x12;
    rc_resource_buffer=output;rc_resource_buffer_size=sizeof(output);rc_resource_buffer_used=8;
    rc_set_resource_buffer_word(4,0x1234);
    if(rc_resource_buffer_used!=8||memcmp(output,expected,sizeof(output)))return 0;
    buffer_word_offset=8;if(!trapped(buffer_word_store_invalid))return 0;
    buffer_word_offset=1;return trapped(buffer_word_store_invalid);
}
static void buffer_word_load_invalid(void)
{
    rc_resource_buffer_word(buffer_word_offset);
}
static int buffer_word_load_contract(void)
{
    BYTE output[16],saved[16];
    memset(output,0xa5,sizeof(output));output[4]=0x34;output[5]=0x12;
    memcpy(saved,output,sizeof(output));
    rc_resource_buffer=output;rc_resource_buffer_size=sizeof(output);rc_resource_buffer_used=8;
    if(rc_resource_buffer_word(4)!=0x1234||rc_resource_buffer_used!=8||
       memcmp(output,saved,sizeof(output)))return 0;
    buffer_word_offset=8;if(!trapped(buffer_word_load_invalid))return 0;
    buffer_word_offset=1;return trapped(buffer_word_load_invalid);
}
static int version_block_contract(void)
{
    BYTE output[32],expected[32];
    static const BYTE block[]={16,0,4,0,0,0,'K',0,0,0,0,0,0x78,0x56,0x34,0x12};
    static const BYTE empty[]={10,0,0,0,0,0,'K',0,0,0};
    DWORD value=0x12345678;
    extern int rc_mac;
    rc_mac=0;
    memset(output,0xa5,sizeof(output));memcpy(expected,output,sizeof(output));
    memset(expected+2,0,2);memcpy(expected+4,block,sizeof(block));
    rc_resource_buffer=output;rc_resource_buffer_size=sizeof(output);rc_resource_buffer_used=2;
    if(rc_write_version_block(L"K",&value,4)!=4||rc_resource_buffer_used!=20||
       memcmp(output,expected,sizeof(output)))return 0;
    memset(output,0xa5,sizeof(output));memcpy(expected,output,sizeof(output));
    memset(expected+2,0,2);memcpy(expected+4,empty,sizeof(empty));
    rc_resource_buffer_used=2;
    return rc_write_version_block(L"K",NULL,0x10000)==4&&rc_resource_buffer_used==14&&
        !memcmp(output,expected,sizeof(output))&&value==0x12345678;
}

static void resource_flush_unobserved(void)
{
    rc_apstudio=1;rc_flush_resources(NULL);
}
static int resource_flush_contract(void)
{
    extern DWORD rc_apstudio_size1,rc_apstudio_size2,rc_apstudio_size3,rc_apstudio_size4;
    extern FILE *rc_apstudio_stream1,*rc_apstudio_stream2,*rc_apstudio_stream3,*rc_apstudio_stream4;
    rc_apstudio=0;
    rc_apstudio_size1=11;rc_apstudio_size2=13;rc_apstudio_size3=17;rc_apstudio_size4=19;
    rc_apstudio_stream1=(FILE *)0x1234;rc_apstudio_stream2=(FILE *)0x2345;
    rc_apstudio_stream3=(FILE *)0x3456;rc_apstudio_stream4=(FILE *)0x4567;
    return rc_flush_resources((FILE *)0x87654321)==1&&rc_flush_resources(NULL)==1&&
        rc_apstudio_size1==11&&rc_apstudio_size2==13&&rc_apstudio_size3==17&&rc_apstudio_size4==19&&
        rc_apstudio_stream1==(FILE *)0x1234&&rc_apstudio_stream2==(FILE *)0x2345&&
        rc_apstudio_stream3==(FILE *)0x3456&&rc_apstudio_stream4==(FILE *)0x4567&&
        trapped(resource_flush_unobserved);
}

static void __stdcall resource_buffer_release_boundary(void *memory)
{
    recon_native_entry_finish(memory==(void *)0x12345678&&
        rc_resource_buffer==(BYTE *)memory&&rc_resource_buffer_size==0&&
        rc_resource_buffer_used==17);
}
static void resource_buffer_release_invoke(void *context)
{
    (void)context;rc_release_resource_buffer();
}
static int resource_buffer_release_contract(void)
{
    rc_resource_buffer=(BYTE *)0x12345678;
    rc_resource_buffer_size=37;rc_resource_buffer_used=17;
    return recon_native_at_entry(rc_release,resource_buffer_release_boundary,
                                  resource_buffer_release_invoke,NULL)==1;
}

static RcStringBlock written_string_block,written_string_saved;
static RcResourceType written_string_type;
static BYTE written_string_output[64],written_string_expected[64];
static unsigned written_string_size;
static void __stdcall string_table_write_boundary(RcResourceType *type,RcResource *resource)
{
    recon_native_entry_finish(type==&written_string_type&&resource&&
        resource->language==0x409&&resource->flags==0x1030&&resource->id==2&&
        resource->version==0x11111111&&resource->characteristics==0x22222222&&
        resource->size==written_string_size&&!resource->name&&
        rc_resource_buffer_used==(int)written_string_size&&!rc_resource_data_file&&
        !memcmp(written_string_output,written_string_expected,sizeof(written_string_output))&&
        !memcmp(&written_string_block,&written_string_saved,sizeof(written_string_block)));
}
static void string_table_write_invoke(void *context)
{
    (void)context;rc_write_string_table(NULL);
}
static int string_table_write_contract(void)
{
    static WCHAR text[]=L"Hi\0",empty[]=L"\0";
    static const BYTE plain[36]={0,0,2,0,'H',0,'i',0};
    static const BYTE terminated[40]={0,0,3,0,'H',0,'i',0,0,0,0,0,1,0,0,0};
    unsigned mode;
    extern int rc_mac;
    memset(&written_string_block,0,sizeof(written_string_block));
    written_string_block.version=0x11111111;written_string_block.characteristics=0x22222222;
    written_string_block.block=1;written_string_block.language=0x409;
    written_string_block.flags=0x1030;
    written_string_block.strings[1]=text;written_string_block.strings[3]=empty;
    written_string_saved=written_string_block;
    memset(&written_string_type,0,sizeof(written_string_type));written_string_type.id=6;
    rc_heap=recon_HeapCreate(1,0,0);rc_resource_types=&written_string_type;
    rc_string_blocks=&written_string_block;rc_mac=0;
    for(mode=0;mode<2;++mode){
        rc_null_terminate_strings=mode;written_string_size=mode?40:36;
        memset(written_string_output,0xa5,sizeof(written_string_output));
        memcpy(written_string_expected,written_string_output,sizeof(written_string_expected));
        memcpy(written_string_expected,mode?terminated:plain,written_string_size);
        rc_resource_buffer=written_string_output;rc_resource_buffer_size=sizeof(written_string_output);
        rc_resource_buffer_used=17;
        if(recon_native_at_entry(rc_write_resource,string_table_write_boundary,
                                 string_table_write_invoke,NULL)!=1)return 0;
    }
    return 1;
}

static int version_value_contract(void)
{
    const WCHAR *inputs[]={L",\"AB\\0\" VALUE ",L",0x12345678L,7 VALUE "};
    static const BYTE strings[]={18,0,3,0,1,0,'K',0,0,0,0,0,'A',0,'B',0,0,0};
    static const BYTE numbers[]={18,0,6,0,0,0,'K',0,0,0,0,0,0x78,0x56,0x34,0x12,7,0};
    BYTE output[32],expected[32];
    unsigned i,n;
    extern int rc_mac;
    rc_mac=0;rc_codepage=1252;
    for(i=0;i<2;++i){
        for(n=0;inputs[i][n];++n);
        memcpy(rc_resource_read_buffer,inputs[i]+1,n*sizeof(WCHAR));
        rc_resource_character=inputs[i][0];rc_resource_buffer_index=0;
        rc_resource_line_start=0;rc_resource_recording=0;
        memset(output,0xa5,sizeof(output));memcpy(expected,output,sizeof(output));
        memset(expected+2,0,2);memcpy(expected+4,i?numbers:strings,sizeof(strings));
        rc_resource_buffer=output;rc_resource_buffer_size=sizeof(output);rc_resource_buffer_used=2;
        if(rc_write_version_value(L"K")!=4||rc_resource_buffer_used!=22||
           rc_resource_token.kind!=113||memcmp(output,expected,sizeof(output)))return 0;
    }
    return 1;
}

static int version_number_contract(void)
{
    const WCHAR input[]=L"0x89abcdefL BEGIN ";
    DWORD number[3]={0x12345678,0x55555555,0x87654321};
    memcpy(rc_resource_read_buffer,input+1,sizeof(input)-sizeof(input[0]));
    rc_resource_character=input[0];rc_resource_buffer_index=0;
    rc_resource_line_start=0;rc_resource_recording=0;
    rc_read_version_number(number+1);
    return number[0]==0x12345678&&number[1]==0x89abcdef&&number[2]==0x87654321&&
        rc_resource_token.kind==13;
}

static int version_quad_contract(void)
{
    const WCHAR *inputs[]={L"1 BEGIN ",L"1,2 BEGIN ",L"1,2,3 BEGIN ",L"1,2,3,4 BEGIN "};
    const RcVersionNumber expected[]={{0x10000,0},{0x10002,0},
                                     {0x10002,0x30000},{0x10002,0x30004}};
    RcVersionNumber number;
    unsigned i,n;
    for(i=0;i<4;++i){
        for(n=0;inputs[i][n];++n);
        memcpy(rc_resource_read_buffer,inputs[i]+1,n*sizeof(WCHAR));
        rc_resource_character=inputs[i][0];rc_resource_buffer_index=0;
        rc_resource_line_start=0;rc_resource_recording=0;
        number.ms=0x12345678;number.ls=0x87654321;
        rc_read_version_quad(&number);
        if(memcmp(&number,&expected[i],sizeof(number))||rc_resource_token.kind!=13)return 0;
    }
    return 1;
}

static int dialog_include_contract(void)
{
    static const WCHAR name[]=L"include\\rich.h";
    static const char bytes[]="include\\rich.h";
    BYTE output[32],expected[32];
    RcResourceToken saved;
    memset(output,0xa5,sizeof(output));memcpy(expected,output,sizeof(output));
    memcpy(expected+2,bytes,sizeof(bytes));
    memcpy(rc_resource_name,name,sizeof(name));
    memset(&rc_resource_token,0xa5,sizeof(rc_resource_token));
    rc_resource_token.kind=23;saved=rc_resource_token;
    rc_heap=recon_HeapCreate(1,0,0);
    rc_resource_buffer=output;rc_resource_buffer_size=sizeof(output);rc_resource_buffer_used=2;
    rc_parse_dialog_include((RcResource *)0x12345678);
    return rc_resource_buffer_used==2+sizeof(bytes)&&
        !memcmp(output,expected,sizeof(output))&&!memcmp(rc_resource_name,name,sizeof(name))&&
        !memcmp(&rc_resource_token,&saved,sizeof(saved));
}

static void wide_resource_string_overflow(void)
{
    unsigned i;
    for(i=0;i<257;++i)rc_resource_read_buffer[i]=L'A';
    rc_resource_read_buffer[257]=L'"';rc_resource_read_buffer[258]=L' ';
    rc_resource_read_buffer[259]=0;
    rc_resource_buffer_index=0;rc_resource_line_start=0;rc_resource_recording=0;
    rc_resource_character=L'"';rc_expand_resource_string();
}
static int check_wide_resource_string(const WCHAR *input,const WCHAR *expected)
{
    RcResourceToken saved;
    unsigned raw=0,count=0,i;
    while(input[raw]!=L'"')++raw;
    while(expected[count])++count;
    memcpy(rc_resource_read_buffer,input,(raw+3)*sizeof(WCHAR));
    rc_resource_buffer_index=0;rc_resource_line_start=0;rc_resource_recording=0;
    rc_resource_character=L'"';
    memset(rc_resource_name,0xa5,sizeof(rc_resource_name));
    memset(&rc_resource_token,0xa5,sizeof(rc_resource_token));saved=rc_resource_token;
    saved.value=(WORD)count;saved.kind=23;saved.unknown19=23;
    rc_expand_resource_string();
    if(memcmp(&saved,&rc_resource_token,sizeof(saved))||rc_resource_character!=L' '||
       memcmp(rc_resource_name,expected,(count+1)*sizeof(WCHAR)))return 0;
    for(i=raw+1;i<257;++i)if(rc_resource_name[i]!=0xa5a5)return 0;
    return 1;
}
static int wide_resource_string_contract(void)
{
    extern int rc_mac;
    rc_mac=0;
    return check_wide_resource_string(L"Wide\" ",L"Wide")&&
        check_wide_resource_string(L"A\\qB\\x0043\\0000101\\t\\n\\r\\\\\" ",L"ABCA\t\n\r\\")&&
        trapped(wide_resource_string_overflow);
}

static int raw_resource_contract(void)
{
    static const WCHAR input[]=L"1,0x12345678L,\"AB\" END ";
    static const BYTE bytes[]={1,0,0x78,0x56,0x34,0x12,'A','B'};
    BYTE output[24],expected[24];
    extern int rc_mac;
    memset(output,0xa5,sizeof(output));memcpy(expected,output,sizeof(output));
    memcpy(expected+1,bytes,sizeof(bytes));
    memcpy(rc_resource_read_buffer,input+1,sizeof(input)-sizeof(input[0]));
    rc_resource_character=input[0];rc_resource_buffer_index=0;
    rc_resource_line_start=0;rc_resource_recording=0;rc_resource_token.kind=13;
    rc_resource_buffer=output;rc_resource_buffer_size=sizeof(output);rc_resource_buffer_used=1;
    rc_heap=recon_HeapCreate(1,0,0);rc_codepage=1252;rc_mac=0;
    return rc_parse_raw_resource(NULL)==sizeof(bytes)&&rc_resource_buffer_used==9&&
        rc_resource_token.kind==14&&!memcmp(output,expected,sizeof(output));
}

static int file_copy_stream_contract(void)
{
    BYTE input[16397],output[16398];
    unsigned i;
    FILE *file;
    ShimFILE *from,*to;
    for(i=0;i<sizeof(input);++i)input[i]=(BYTE)(i*43+3);
    file=fopen("stream-copy-input.bin","wb");if(!file)return 0;
    if(fwrite(input,1,sizeof(input),file)!=sizeof(input)||fclose(file))return 0;
    rc_heap=recon_HeapCreate(1,0,0);
    from=recon_fopen("stream-copy-input.bin","rb");
    to=recon_fopen("stream-copy-output.bin","wb");
    if(!from||!to||rc_copy_file((FILE *)from,(FILE *)to)!=1||
       recon_ftell(from)!=sizeof(input)||recon_ftell(to)!=sizeof(input))return 0;
    /* The second copy starts at EOF and must emit no bytes. */
    if(rc_copy_file((FILE *)from,(FILE *)to)!=1||recon_ftell(to)!=sizeof(input)||
       recon_fclose(from)||recon_fclose(to))return 0;
    file=fopen("stream-copy-output.bin","rb");if(!file)return 0;
    return fread(output,1,sizeof(output),file)==sizeof(input)&&
        !memcmp(output,input,sizeof(input))&&!fclose(file);
}

static void bitmap_extended_header(void)
{
    rc_resource_data_file=(FILE *)recon_fopen("bitmap-extended-unit.bmp","rb");
    rc_convert_bitmap_file();
}
static int bitmap_file_contract(void)
{
    RcBitmapFileHeader file_header;
    RcBitmapInfo info;
    BYTE output[64],expected[64];
    FILE *file=fopen("assets/pattern.bmp","rb");
    ShimFILE *input;
    long size;
    extern int rc_mac;
    if(!file)return 0;
    if(fread(&file_header,1,sizeof(file_header),file)!=sizeof(file_header)||
       fread(&info,1,sizeof(info),file)!=sizeof(info)||fclose(file))return 0;
    if(info.size!=40||info.planes!=1||info.bit_count!=24||info.compression||info.colors_used)
        return 0;
    memset(output,0xa5,sizeof(output));memcpy(expected,output,sizeof(output));
    memcpy(expected,&info,sizeof(info));
    input=recon_fopen("assets/pattern.bmp","rb");if(!input)return 0;
    rc_resource_data_file=(FILE *)input;
    rc_resource_buffer=output;rc_resource_buffer_size=sizeof(output);rc_resource_buffer_used=0;
    rc_mac=0;
    size=rc_convert_bitmap_file();
    if(size!=(long)(file_header.size+40-file_header.pixel_offset)||
       rc_resource_buffer_used!=40||memcmp(output,expected,sizeof(output))||
       recon_ftell(input)!=(long)file_header.pixel_offset||
       rc_resource_data_file!=(FILE *)input||recon_fclose(input))return 0;
    file=fopen("bitmap-extended-unit.bmp","wb");if(!file)return 0;
    info.size=41;
    if(fwrite(&file_header,1,sizeof(file_header),file)!=sizeof(file_header)||
       fwrite(&info,1,sizeof(info),file)!=sizeof(info)||fclose(file))return 0;
    return trapped(bitmap_extended_header);
}

static void file_copy_short_read(void)
{
    ShimFILE *input=recon_fopen("file-copy-input.bin","rb");
    ShimFILE *output=recon_fopen("file-copy-output.bin","wb");
    rc_copy_file_bytes((FILE *)input,(FILE *)output,20000);
}
static int file_copy_bytes_contract(void)
{
    BYTE input[16397],output[16398];
    const unsigned sizes[]={0,1,16397};
    unsigned i,j;
    FILE *file;
    ShimFILE *from,*to;
    for(i=0;i<sizeof(input);++i)input[i]=(BYTE)(i*37+11);
    file=fopen("file-copy-input.bin","wb");if(!file)return 0;
    if(fwrite(input,1,sizeof(input),file)!=sizeof(input)||fclose(file))return 0;
    rc_heap=recon_HeapCreate(1,0,0);
    for(i=0;i<3;++i){
        from=recon_fopen("file-copy-input.bin","rb");
        to=recon_fopen("file-copy-output.bin","wb");
        if(!from||!to)return 0;
        j=rc_copy_file_bytes((FILE *)from,(FILE *)to,sizes[i]);
        if(j!=(sizes[i]>16384?sizes[i]-16384:sizes[i])||
           recon_ftell(from)!=(long)sizes[i]||recon_ftell(to)!=(long)sizes[i])return 0;
        if(recon_fclose(from)||recon_fclose(to))return 0;
        file=fopen("file-copy-output.bin","rb");if(!file)return 0;
        if(fread(output,1,sizeof(output),file)!=sizes[i]||
           memcmp(input,output,sizes[i])||fclose(file))return 0;
    }
    return trapped(file_copy_short_read);
}

static int resource_byte_append_contract(void)
{
    BYTE output[24],expected[24],source[]={0x01,0x23,0x45,0x67,0x89,0xab};
    memset(output,0xa5,sizeof(output));memcpy(expected,output,sizeof(output));
    rc_resource_buffer=output;rc_resource_buffer_size=sizeof(output);rc_resource_buffer_used=3;
    rc_append_resource_bytes(NULL,0);
    if(rc_resource_buffer_used!=3||memcmp(output,expected,sizeof(output)))return 0;
    rc_append_resource_bytes(source,sizeof(source));memcpy(expected+3,source,sizeof(source));
    if(rc_resource_buffer_used!=9||memcmp(output,expected,sizeof(output)))return 0;
    /* Forward overlapping copies propagate the first byte, as in the body. */
    rc_resource_buffer_used=4;
    rc_append_resource_bytes(output+3,5);memset(expected+4,source[0],5);
    return rc_resource_buffer_used==9&&!memcmp(output,expected,sizeof(output));
}

static int riff_header_contract(void)
{
    static const BYTE bytes[][8]={{'R','I','F','F',0,0,0,0},
                                 {'R','I','F','X',0,0,0,0},
                                 {'R','I','F','F',0,0,0,0}};
    const unsigned sizes[]={8,8,4};
    unsigned i;
    for(i=0;i<3;++i){
        FILE *file=fopen("riff-header-unit.bin","wb");
        ShimFILE *input;
        if(!file)return 0;
        if(fwrite(bytes[i],1,sizes[i],file)!=sizes[i]||fclose(file))return 0;
        input=recon_fopen("riff-header-unit.bin","rb");if(!input)return 0;
        rc_resource_data_file=(FILE *)input;
        if(rc_is_riff_resource(0x12345678)!=(i==0)||recon_ftell(input)!=0)return 0;
        if(recon_fclose(input))return 0;
    }
    return !remove("riff-header-unit.bin");
}

static int resource_file_contract(void)
{
    FILE *first=(FILE *)0x12345670,*second=(FILE *)0x76543210;
    rc_resource_data_file=first;rc_resource_buffer_used=37;
    if(rc_resource_file(NULL)!=first||rc_resource_data_file!=first)return 0;
    if(rc_resource_file(second)!=second||rc_resource_data_file!=second)return 0;
    if(rc_resource_file(NULL)!=second||rc_resource_buffer_used!=37)return 0;
    rc_reset_resource_buffer();
    return !rc_resource_file(NULL)&&!rc_resource_data_file&&!rc_resource_buffer_used;
}

static void resource_search_empty_component(void)
{
    char output[MAX_PATH];
    rc_include_paths="";
    rc_search_resource_path("x","INCLUDE",output);
}
static int resource_search_contract(void)
{
    char output[MAX_PATH],expected[MAX_PATH];
    const char *absolute="R:\\absolute\\file.bin";
    rc_include_paths=".";
    memset(output,0xa5,sizeof(output));memcpy(expected,output,sizeof(output));
    strcpy(expected,".\\resources.rc");
    rc_search_resource_path("resources.rc","INCLUDE",output);
    if(memcmp(output,expected,sizeof(output)))return 0;
    memset(output,0xa5,sizeof(output));memcpy(expected,output,sizeof(output));
    strcpy(expected,absolute);
    rc_search_resource_path(absolute,"INCLUDE",output);
    if(memcmp(output,expected,sizeof(output)))return 0;
    /* Use a real ASCII current-directory component after a failed candidate. */
    rc_include_paths="__rc_missing_search_dir__;.";
    rc_search_resource_path("resources.rc","INCLUDE",output);
    return !strcmp(output,".\\resources.rc")&&trapped(resource_search_empty_component);
}

static void resource_source_unobserved(void)
{
    rc_apstudio=1;rc_write_resource_source(NULL,NULL,NULL);
}
static int resource_source_contract(void)
{
    RcResource resource,saved;
    RcResourceType type,saved_type;
    WCHAR name[4]={L'f',L'i',L'l',0},saved_name[4];
    extern DWORD rc_apstudio_size2;
    memset(&resource,0xa5,sizeof(resource));saved=resource;
    memset(&type,0x5a,sizeof(type));saved_type=type;
    memcpy(saved_name,name,sizeof(name));
    rc_apstudio=0;rc_apstudio_size2=0x87654321;
    rc_write_resource_source(&resource,&type,name);
    rc_write_resource_source(NULL,NULL,NULL);
    return rc_apstudio_size2==0x87654321&&
        !memcmp(&resource,&saved,sizeof(resource))&&
        !memcmp(&type,&saved_type,sizeof(type))&&
        !memcmp(name,saved_name,sizeof(name))&&trapped(resource_source_unobserved);
}

static void accelerator_outside_prefix(void)
{
    rc_accelerator_offset=rc_resource_buffer_used;
    rc_mark_last_accelerator();
}
static void accelerator_mac_outside_prefix(void)
{
    extern int rc_mac;
    rc_mac=1;rc_accelerator_offset=rc_resource_buffer_used-1;
    rc_mark_last_accelerator();
}
static int accelerator_marker_contract(void)
{
    BYTE output[24],expected[24];
    unsigned mode;
    extern int rc_mac;
    for(mode=0;mode<2;++mode){
        memset(output,0x25,sizeof(output));memcpy(expected,output,sizeof(output));
        rc_resource_buffer=output;rc_resource_buffer_size=24;rc_resource_buffer_used=8;
        rc_dialog_count_offset=0x12345678;rc_control_data_offset=0x87654321;
        rc_accelerator_offset=0xffffffff;rc_mac=mode;
        rc_begin_accelerator();
        if(rc_accelerator_offset!=8||rc_resource_buffer_used!=8||
           rc_dialog_count_offset!=0x12345678||rc_control_data_offset!=0x87654321||
           memcmp(output,expected,sizeof(output)))return 0;
        rc_resource_buffer_used=16;expected[8+mode]|=0x80;
        rc_mark_last_accelerator();
        if(rc_accelerator_offset!=8||rc_resource_buffer_used!=16||
           memcmp(output,expected,sizeof(output)))return 0;
    }
    return trapped(accelerator_outside_prefix)&&trapped(accelerator_mac_outside_prefix);
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
    {"resource-decimal-id-contract",1,resource_decimal_id_contract},
    {"resource-keyword-contract",1,resource_keyword_contract},
    {"resource-attribute-contract",1,resource_attribute_contract},
    {"resource-metadata-contract",1,resource_metadata_contract},
    {"resource-buffer-reset-contract",1,resource_buffer_reset_contract},
    {"resource-buffer-reserve-contract",1,resource_buffer_reserve_contract},
    {"resource-preamble-contract",1,resource_preamble_contract},
    {"resource-string-contract",1,resource_string_contract},
    {"menu-caption-contract",1,menu_caption_contract},
    {"menu-commas-contract",1,menu_commas_contract},
    {"menu-attribute-contract",1,menu_attribute_contract},
    {"menu-writer-prefix-contract",1,menu_writer_prefix_contract},
    {"resource-string-write-contract",1,resource_string_write_contract},
    {"resource-decimal-contract",1,resource_decimal_contract},
    {"apstudio-name-contract",1,apstudio_name_contract},
    {"resource-style-initial-contract",1,resource_style_initial_contract},
    {"resource-style-contract",1,resource_style_contract},
    {"menu-end-marker-contract",1,menu_end_marker_contract},
    {"resource-count-reset-contract",1,resource_count_reset_contract},
    {"output-alignment-contract",1,output_alignment_contract},
    {"resource-append-prefix-contract",1,resource_append_prefix_contract},
    {"resource-record-verbose-hold",1,resource_record_verbose_hold},
    {"resource-data-size-contract",1,resource_data_size_contract},
    {"resource-identifier-write-contract",1,resource_identifier_write_contract},
    {"resource-payload-contract",1,resource_payload_contract},
    {"dialog-initial-contract",1,dialog_initial_contract},
    {"dialog-coordinate-contract",1,dialog_coordinate_contract},
    {"dialog-rectangle-contract",1,dialog_rectangle_contract},
    {"dialog-attribute-style-contract",1,dialog_attribute_style_contract},
    {"dialog-caption-contract",1,dialog_caption_contract},
    {"dialog-font-contract",1,dialog_font_contract},
    {"dialog-menu-contract",1,dialog_menu_contract},
    {"dialog-header-contract",1,dialog_header_contract},
    {"dialog-control-initial-contract",1,dialog_control_initial_contract},
    {"control-text-contract",1,control_text_contract},
    {"control-id-contract",1,control_id_contract},
    {"resource-buffer-padding-contract",1,resource_buffer_padding_contract},
    {"control-record-contract",1,control_record_contract},
    {"generic-control-contract",1,generic_control_contract},
    {"wide-resource-number-contract",1,wide_resource_number_contract},
    {"string-table-contract",1,string_table_contract},
    {"accelerator-marker-contract",1,accelerator_marker_contract},
    {"resource-source-contract",1,resource_source_contract},
    {"resource-search-contract",1,resource_search_contract},
    {"resource-file-contract",1,resource_file_contract},
    {"riff-header-contract",1,riff_header_contract},
    {"resource-byte-append-contract",1,resource_byte_append_contract},
    {"file-copy-bytes-contract",1,file_copy_bytes_contract},
    {"bitmap-file-contract",1,bitmap_file_contract},
    {"file-copy-stream-contract",1,file_copy_stream_contract},
    {"raw-resource-contract",1,raw_resource_contract},
    {"wide-resource-string-contract",1,wide_resource_string_contract},
    {"dialog-include-contract",1,dialog_include_contract},
    {"version-quad-contract",1,version_quad_contract},
    {"version-number-contract",1,version_number_contract},
    {"buffer-word-store-contract",1,buffer_word_store_contract},
    {"buffer-word-load-contract",1,buffer_word_load_contract},
    {"version-block-contract",1,version_block_contract},
    {"version-value-contract",1,version_value_contract},
    {"string-table-write-contract",1,string_table_write_contract},
    {"resource-buffer-release-contract",1,resource_buffer_release_contract},
    {"resource-flush-contract",1,resource_flush_contract},
    {"resource-read-buffer-contract",1,resource_buffer_contract},
    {"resource-byte-read-contract",1,byte_read_contract},
    {"packed-macro-parameter",1,parameters},
    {"old-no-space-before-unsupported-copy",1,no_space_guard}
};
