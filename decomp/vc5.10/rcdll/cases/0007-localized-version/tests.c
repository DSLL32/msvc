#define RECON_SHIM_IMPL
#include <string.h>
#include "resources.h"
#include "command.h"
#include <recon-tool.h>
#include <recon.h>
static int concatenate_prefix(void)
{
    WORD storage[16];unsigned i;
    extern int rc_mac;
    for(i=0;i<16;++i)storage[i]=0x5555;
    rc_mac=0;rc_resource_buffer=(BYTE *)storage;
    rc_resource_buffer_size=sizeof(storage);rc_resource_buffer_used=4;
    storage[0]=L'A';storage[1]=0;
    rc_concatenate_resource_string(L"BC",1);
    if(rc_resource_buffer_used!=8||storage[0]!=L'A'||storage[1]!=L'B'||
       storage[2]!=L'C'||storage[3]||storage[4]!=0x5555)return 0;
    rc_resource_buffer_used=2;storage[0]=L'A';
    rc_concatenate_resource_string(L"D",1);
    return rc_resource_buffer_used==6&&storage[0]==L'A'&&
           storage[1]==L'D'&&!storage[2]&&storage[4]==0x5555;
}
static int octal_end_character(void)
{
    DWORD value;
    WCHAR valid[]=L"0o377z",invalid[]=L"0o8";
    memcpy(rc_resource_name,valid,sizeof(valid));
    rc_resource_number_character=L'o';rc_resource_number_cursor=rc_resource_name+2;
    value=rc_parse_resource_octal();
    if(value!=255||rc_resource_number_character!=L'z'||
       rc_resource_number_cursor!=rc_resource_name+6)return 0;
    memcpy(rc_resource_name,invalid,sizeof(invalid));rc_resource_number_cursor=rc_resource_name+2;
    return !rc_parse_resource_octal()&&rc_resource_number_character==L'8'&&
           rc_resource_number_cursor==rc_resource_name+3;
}
static int report_history(void)
{
    extern DWORD rc_last_resource_error_line,rc_last_resource_error_file;
    extern unsigned rc_last_resource_error_code;
    extern int rc_last_line,rc_message_count;
    rc_window=NULL;rc_diagnostic=NULL;rc_result=0;
    rc_last_resource_error_line=99;rc_last_resource_error_file=98;
    rc_last_resource_error_code=100;
    rc_resource_token.line=33;rc_resource_token.file_index=7;
    rc_last_line=33;rc_message_count=0;strcpy(rc_error_text,"diagnostic");
    rc_report_resource_error(2151);
    return rc_result==1&&rc_last_resource_error_line==33&&
        rc_last_resource_error_file==7&&rc_last_resource_error_code==2151&&
        rc_last_line==33&&rc_message_count==2&&!strcmp(rc_error_text,"diagnostic");
}
const ReconTest localized_tests[]={
 {"version-concatenate-prefix",1,concatenate_prefix},
 {"resource-octal-end",1,octal_end_character},
 {"resource-report-history",1,report_history}
};
