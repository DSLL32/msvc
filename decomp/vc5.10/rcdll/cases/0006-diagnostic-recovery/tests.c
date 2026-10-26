#define RECON_SHIM_IMPL
#include "command.h"
#include "resources.h"
#include "directives.h"
#include <string.h>
#include <recon-tool.h>
#include <recon.h>
static int token_line_owner(void)
{
    extern int rc_last_line,rc_message_count;
    char newline[]="\n";
    rc_window=NULL;rc_diagnostic=NULL;
    rc_resource_token.line=17;rc_preparing_line=29;
    rc_last_line=17;rc_message_count=2;
    rc_emit(newline);
    if(rc_message_count!=3||rc_last_line!=17)return 0;
    rc_resource_token.line=19;rc_emit(newline);
    return !rc_message_count&&rc_last_line==19&&rc_preparing_line==29;
}
static int skip_line_cursor(void)
{
    WCHAR text[]={L'x',L'/',L'y',L'/',L'*',L'z',L'*',L'/',L'\n',L'q',0};
    rc_source_cursor=text;rc_skip_line();
    if(rc_source_cursor!=text+8||*rc_source_cursor!=L'\n')return 0;
    rc_source_cursor=text;rc_cpp_C=0;rc_line_comment();
    return rc_source_cursor==text+8&&*rc_source_cursor==L'\n';
}
static char *reported_pointer;
static int WINAPI remember_message(DWORD first,DWORD second,char *message)
{
    reported_pointer=message;
    return first||second;
}
static int report_storage(void)
{
    extern char rc_diagnostic_text[512],rc_normalized_diagnostic[512];
    char *source="retained\\\\path";
    rc_window=NULL;rc_diagnostic=remember_message;
    rc_preparing_line=0;rc_filename=NULL;
    strcpy(rc_diagnostic_text,source);
    rc_report_input_message(0,0,NULL);
    return reported_pointer==rc_diagnostic_text&&
        !strcmp(rc_diagnostic_text,source)&&
        !strcmp(rc_normalized_diagnostic,"retained\\path");
}
const ReconTest diagnostic_tests[]={
 {"diagnostic-token-line-owner",1,token_line_owner},
 {"skip-line-cursor",1,skip_line_cursor},
 {"report-distinct-storage",1,report_storage}
};
