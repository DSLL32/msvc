#define RECON_SHIM_IMPL
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include "command.h"
#include "directives.h"
#include "macro-expansion.h"
#include "resource-input.h"
#include "resources.h"
#include <recon-tool.h>
#include <recon.h>
static int skip_white(void)
{
    WCHAR text[]={' ',9,'A','B',' ','C',0,0xa5a5};
    WCHAR saved[8];
    memcpy(saved,text,sizeof(text));
    if(rc_skip_white(text,0)!=text+2||memcmp(text,saved,sizeof(text)))return 0;
    if(rc_skip_white(text,1)!=text+2)return 0;
    saved[4]=0;
    return !memcmp(text,saved,sizeof(text));
}
static int character_constant(void)
{
    extern int rc_cpp_J,rc_cpp_char;
    WCHAR first[]={'A',39,0},second[]={'B',39,0};
    RcIntegerToken *value;
    rc_heap=recon_HeapCreate(1,0,0);
    rc_cpp_J=0;rc_cpp_char=1;rc_source_cursor=first;
    if(rc_read_character_constant()!=7||rc_source_cursor!=first+2)return 0;
    value=(RcIntegerToken *)rc_token_value;
    if(value->token!=7||value->value!=65||value->type->flags!=0x1003)return 0;
    rc_cpp_J=1;rc_cpp_char=0;rc_source_cursor=second;
    if(rc_read_character_constant()!=9||rc_source_cursor!=second+2)return 0;
    value=(RcIntegerToken *)rc_token_value;
    return value->token==9&&value->value==66&&value->type->flags==0x1013&&
           recon_HeapDestroy(rc_heap);
}
static unsigned packed_mode;
static int packed_number(void)
{
    RcMacro macro={0};
    static WCHAR text[]=L"MACRO";
    const WCHAR record[]={10,L'1',L'2',0,L'A'};
    macro.text=text;
    memcpy(rc_string_buffer,record,sizeof(record));
    rc_number_buffer_active=1;rc_expanding_macro=1;
    rc_macro_frames[1].macro=&macro;
    rc_macro_frames[1].output=rc_string_buffer;
    rc_macro_frames[1].arguments=1;rc_macro_frames[1].expanded=1;
    rc_macro_output_cursor=rc_string_buffer+5;
    rc_number_buffer=rc_string_buffer+1024;
    rc_source_cursor=rc_string_buffer+2;
    switch(packed_mode){
    case 1:rc_source_cursor=rc_string_buffer+8;break;
    case 2:rc_macro_frames[1].expanded=2;break;
    case 3:rc_string_buffer[4]=L'R';break;
    case 4:rc_string_buffer[2]=0;break;
    case 5:rc_number_buffer=rc_string_buffer+2044;break;
    }
    return rc_read_number(L'1')==1&&rc_source_cursor==rc_string_buffer+3&&
        rc_number_buffer==rc_string_buffer+1026&&
        rc_string_buffer[1024]==L'1'&&rc_string_buffer[1025]==L'2'&&
        !rc_string_buffer[1026]&&!memcmp(rc_string_buffer,record,sizeof(record));
}
static int packed_number_guard(void)
{
    pid_t child;
    int status;
    if(!packed_number())return 0;
    for(packed_mode=1;packed_mode<=5;++packed_mode){
        child=fork();
        if(child<0)return 0;
        if(!child){packed_number();_exit(17);}
        if(waitpid(child,&status,0)!=child||!WIFSIGNALED(status)||
           WTERMSIG(status)!=SIGTRAP)return 0;
    }
    return 1;
}
static int fraction_spelling(void)
{
    WCHAR decimal[]=L".25",exponent[]=L"e-2F!",suffix[]=L".25L!";
    rc_number_buffer_active=1;rc_cpp_active=0;
    rc_string_buffer[1024]=L'1';rc_source_cursor=decimal;
    if(rc_read_fraction(rc_string_buffer+1025)!=1||
       rc_number_buffer!=rc_string_buffer+1028||rc_source_cursor!=decimal+3||
       memcmp(rc_string_buffer+1024,L"1.25",10))return 0;
    rc_string_buffer[1024]=L'1';rc_source_cursor=exponent;
    if(rc_read_fraction(rc_string_buffer+1025)!=1||
       rc_number_buffer!=rc_string_buffer+1028||rc_source_cursor!=exponent+4||
       memcmp(rc_string_buffer+1024,L"1e-2",10))return 0;
    rc_cpp_active=1;rc_source_cursor=suffix;
    return rc_read_fraction(rc_string_buffer+1025)==1&&
       rc_number_buffer==rc_string_buffer+1029&&rc_source_cursor==suffix+4&&
       !memcmp(rc_string_buffer+1024,L"1.25L",12);
}
static int token_copy(void)
{
    RcResourceToken from,to,expected;
    memset(&from,0xc9,sizeof(from));memset(&to,0xa7,sizeof(to));
    from.unknown0=0x12345678;from.line=17;from.file_index=23;
    from.unknown12=0xabcdef01;from.value=0x2468;from.kind=42;from.unknown19=9;
    memcpy(from.text,L"token",12);memcpy(from.filename,L"source.rc",20);
    from.unknown1036=0x1357;from.unknown1038=0x9753;
    expected=to;
    memcpy(&expected,&from,20);
    memcpy(expected.text,from.text,12);memcpy(expected.filename,from.filename,20);
    expected.unknown1036=from.unknown1036;expected.unknown1038=from.unknown1038;
    rc_copy_resource_token(&to,&from);
    return !memcmp(&to,&expected,sizeof(to));
}
static int menu_end_tokens(void)
{
    const WORD any[]={0},ends[]={3,78,71,14};
    rc_resource_token.kind=13;
    if(!rc_menu_token_in_set(any)||rc_menu_token_in_set(ends))return 0;
    rc_resource_token.kind=78;
    if(!rc_menu_token_in_set(ends))return 0;
    rc_resource_token.kind=14;
    return rc_menu_token_in_set(ends);
}
static int control_data_patch(void)
{
    BYTE data[16],expected[16];
    memset(data,0xa7,sizeof(data));memcpy(expected,data,sizeof(data));
    rc_resource_buffer=data;rc_resource_buffer_size=sizeof(data);
    rc_resource_buffer_used=sizeof(data);rc_control_data_offset=6;
    rc_patch_control_data_size(0x1234,1);
    expected[6]=0x34;expected[7]=0x12;
    if(memcmp(data,expected,sizeof(data)))return 0;
    rc_resource_buffer=NULL;
    rc_patch_control_data_size(0x5678,0);
    return !memcmp(data,expected,sizeof(data));
}
const ReconTest editor_tests[]={
    {"skip-white-token-boundary",1,skip_white},
    {"character-constant-J-identity",1,character_constant},
    {"packed-number-owner",1,packed_number_guard},
    {"fraction-spelling",1,fraction_spelling},
    {"resource-token-copy-tails",1,token_copy},
    {"menu-end-token-sets",1,menu_end_tokens},
    {"control-data-patch-width",1,control_data_patch}
};
