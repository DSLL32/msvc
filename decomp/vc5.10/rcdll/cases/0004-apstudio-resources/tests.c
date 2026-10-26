#define RECON_SHIM_IMPL
#include "command.h"
#include "directives.h"
#include "resource-output.h"
#include "resources.h"
#include <unistd.h>
#include <string.h>
#include <recon-tool.h>
#include <recon.h>
static int escape_character(void)
{
    WCHAR octal[]=L"01!",hexadecimal[]=L"42!",high[]=L"ff!",limit[]=L"1234";
    extern int rc_mac;
    rc_cpp_active=1;rc_mac=0;rc_source_cursor=octal;
    if(rc_escape_character(L'1')!=65||rc_source_cursor!=octal+2)return 0;
    rc_source_cursor=hexadecimal;
    if(rc_escape_character(L'x')!=66||rc_source_cursor!=hexadecimal+2)return 0;
    rc_source_cursor=high;
    if(rc_escape_character(L'x')!=0xffffffffU||rc_source_cursor!=high+2)return 0;
    rc_source_cursor=limit;
    if(rc_escape_character(L'x')!=0x23||rc_source_cursor!=limit+3)return 0;
    if(rc_escape_character(L'n')!=10||rc_escape_character(L'r')!=13)return 0;
    rc_mac=1;
    return rc_escape_character(L'n')==13&&rc_escape_character(L'r')==10;
}
static int macro_identifier(void)
{
    WCHAR text[]={L'b',1,3,L'c',L'!',0};
    rc_set_character_token(1,39);rc_cpp_active=1;rc_source_cursor=text;
    if(rc_read_macro_identifier(L'a')||rc_source_cursor!=text+4||
       rc_macro_name_length!=4||rc_macro_hash!=0xc6||
       rc_macro_name[0]!=L'a'||rc_macro_name[1]!=L'b'||
       rc_macro_name[2]!=L'c'||rc_macro_name[3])return 0;
    text[2]=2;rc_source_cursor=text;
    return rc_read_macro_identifier(L'a')==1&&rc_source_cursor==text+4;
}
static int apstudio_header(void)
{
    extern DWORD rc_apstudio_header_size;
    ShimFILE *file=recon_fopen("apstudio-header-unit.bin","w+b");
    FILE *check;
    BYTE bytes[37];
    int valid;
    if(!file)return 0;
    rc_apstudio_header_size=36;rc_default_language=0x1234;
    rc_resource_version=0x87654321;rc_resource_characteristics=0x78563412;
    rc_write_apstudio_header((FILE *)file,0x44332211,201);
    if(recon_fclose(file))return 0;
    check=fopen("apstudio-header-unit.bin","rb");if(!check)return 0;
    valid=fread(bytes,1,sizeof(bytes),check)==36&&
        bytes[0]==0x11&&bytes[1]==0x22&&bytes[2]==0x33&&bytes[3]==0x44&&
        bytes[4]==36&&bytes[8]=='H'&&!bytes[9]&&bytes[10]=='W'&&
        !bytes[11]&&bytes[12]=='B'&&!bytes[13]&&!bytes[14]&&!bytes[15]&&
        bytes[16]==255&&bytes[17]==255&&bytes[18]==201&&!bytes[19]&&
        !bytes[20]&&!bytes[21]&&!bytes[22]&&!bytes[23]&&
        bytes[24]==0x30&&!bytes[25]&&bytes[26]==0x34&&bytes[27]==0x12&&
        bytes[28]==2&&!bytes[29]&&!bytes[30]&&!bytes[31]&&
        bytes[32]==0x12&&bytes[33]==0x34&&bytes[34]==0x56&&bytes[35]==0x78;
    fclose(check);unlink("apstudio-header-unit.bin");
    return valid;
}
static int resource_marker(void)
{
    static const WCHAR input[]=L"\"IDM_OPEN" L"\xe002" L"resource.h" L"\xe002" L"7\"  42";
    extern DWORD rc_resource_recording;
    extern int rc_resource_line_start;
    rc_apstudio=1;rc_resource_recording=0;rc_resource_line_start=0;
    rc_resource_token.unknown1036=0x9876;
    memcpy(rc_resource_read_buffer,input,sizeof(input));
    rc_resource_buffer_index=0;
    rc_read_resource_marker(1,0xe001);
    if(rc_resource_character!=L'4'||rc_resource_token.unknown1036!=7||
       memcmp(rc_resource_token.text,L"IDM_OPEN",18)||
       memcmp(rc_resource_token.filename,L"resource.h",22))return 0;
    if(rc_is_resource_marker_space(8)||rc_is_resource_marker_space(11)||
       rc_is_resource_marker_space(0xffff)||rc_is_resource_marker_space(0xe001)||
       rc_is_resource_marker_space(9)!=-1||rc_is_resource_marker_space(10)!=-1||
       rc_is_resource_marker_space(13)!=-1||rc_is_resource_marker_space(32)!=-1)return 0;
    rc_apstudio=0;rc_read_resource_marker(0,0xe001);
    return !rc_resource_token.text[0]&&!rc_resource_token.filename[0]&&
           !rc_resource_token.unknown1036;
}
const ReconTest apstudio_tests[]={
    {"escape-character-width-and-cursor",1,escape_character},
    {"macro-identifier-marker-length",1,macro_identifier},
    {"apstudio-header-distinct-fields",1,apstudio_header},
    {"resource-marker-storage-and-cursor",1,resource_marker}
};
