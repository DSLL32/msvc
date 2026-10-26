#define RECON_SHIM_IMPL
#include <string.h>
#include "command.h"
#include "resources.h"
#include <recon-tool.h>
#include <recon.h>

static int color_mapping(void)
{
    RcMacColor colors[2]={{0x1256,0,0,0},{0x78,0xfe00,0,0}};
    RcMacRgbQuad value={0,0,127,0xa5},saved=value;
    rc_map_mac_color(colors,2,&value);
    if(value.index!=0x56||memcmp(&value,&saved,3))return 0;
    value.red=254;value.index=0xa5;
    rc_map_mac_color(colors,2,&value);
    if(value.index!=0x78||value.red!=254||value.green||value.blue)return 0;
    value.red=value.green=value.blue=255;
    rc_map_mac_color(rc_mac_colors_mono,2,&value);
    if(value.index)return 0;
    value.red=value.green=value.blue=0;
    rc_map_mac_color(rc_mac_colors_mono,2,&value);
    return value.index==1&&rc_mac_long(0x12345678)==0x78563412;
}
static int icon_repack(void)
{
    BYTE bits[16]={1,2,0xa6,0xb7,3,4,0xc8,0xd9,
                   0x0f,0xf0,0xa6,0xb7,0x55,0xaa,0xc8,0xd9};
    const BYTE expected[16]={3,4,1,2,0xaa,0x55,0xf0,0x0f,
                            0,0,0,0,0x55,0xaa,0xc8,0xd9};
    rc_heap=recon_HeapCreate(1,0,0);
    rc_repack_mac_icon_bits(bits,4,4,2,2,2);
    return !memcmp(bits,expected,sizeof(bits))&&recon_HeapDestroy(rc_heap);
}
static int expanded_string(void)
{
    BYTE bytes[10],expected[6]={0,'A',0,'\n',0,0};
    int i;
    rc_codepage=1252;
    memset(bytes,0xa7,sizeof(bytes));
    if(rc_expand_mac_bytes("A\n",2,bytes)!=6||memcmp(bytes,expected,6))return 0;
    for(i=6;i<10;++i)if(bytes[i]!=0xa7)return 0;
    memset(bytes,0xb9,sizeof(bytes));
    return rc_expand_mac_bytes(NULL,0,bytes)==2&&!bytes[0]&&!bytes[1]&&bytes[2]==0xb9;
}
static int dialog_count(void)
{
    WORD words[4]={0xa7a7,0x1234,0xb8b8,0xc9c9};
    rc_resource_buffer=(BYTE *)words;
    rc_resource_buffer_used=rc_resource_buffer_size=8;
    rc_dialog_count_offset=2;
    rc_convert_dialog_count();
    return words[0]==0xa7a7&&words[1]==0x3412&&words[2]==0xb8b8&&
           words[3]==0xc9c9&&rc_dialog_count_offset==2&&
           rc_resource_buffer_used==8&&rc_resource_buffer_size==8;
}
static int resource_ids(void)
{
    extern WORD rc_language;
    WCHAR icon_name[5]={'I','C','N','#',0};
    RcResource items[3];
    RcResourceType type;
    memset(items,0,sizeof(items));memset(&type,0,sizeof(type));
    rc_language=128;rc_resource_types=&type;type.resources=items;
    items[0].id=128;items[0].next=&items[1];items[1].id=129;
    if(rc_assign_mac_resource_id(&type,&items[2])!=130||rc_language!=128)return 0;
    if(rc_is_mac_icon_type(&type))return 0;
    type.name=icon_name;
    return rc_is_mac_icon_type(&type)==1&&items[0].id==128&&items[1].id==129;
}
static int resource_types(void)
{
    WCHAR short_name[2]={'A',0},long_name[6]={'I','C','N','#','X',0};
    return rc_mac_resource_type(short_name,0)==0x41202020&&
           rc_mac_resource_type(long_name,0)==0x49434e23&&
           rc_mac_resource_type(NULL,0x1234)==0x4e323334&&
           rc_mac_resource_type(NULL,0xffff)==0x5c464646&&
           rc_mac_resource_type(NULL,10)==0x48455841&&
           rc_mac_resource_type(NULL,240)==0x48455841&&
           rc_mac_resource_type(NULL,12)==0x43555253;
}
static unsigned notice_calls;
static char *notice_message;
static int WINAPI receive_notice(DWORD first,DWORD second,char *message)
{
    if(first||second)return 1;
    ++notice_calls;notice_message=message;return 0;
}
static int notice_dispatch(void)
{
    extern int rc_line,rc_last_line,rc_message_count;
    char message[]="a notice";
    rc_line=rc_last_line=17;rc_message_count=3;rc_window=NULL;
    rc_diagnostic=NULL;notice_calls=0;notice_message=NULL;
    rc_emit_notice(message);
    if(notice_calls||rc_message_count!=3)return 0;
    rc_diagnostic=receive_notice;
    rc_emit_notice(message);
    return notice_calls==1&&notice_message==message&&rc_message_count==3&&
           rc_line==17&&rc_last_line==17;
}
const ReconTest mac_tests[]={
    {"mac-color-map",1,color_mapping},
    {"mac-icon-repack",1,icon_repack},
    {"mac-expanded-string",1,expanded_string},
    {"mac-dialog-count",1,dialog_count},
    {"mac-resource-ids",1,resource_ids},
    {"mac-resource-types",1,resource_types},
    {"mac-notice-dispatch",1,notice_dispatch}
};
