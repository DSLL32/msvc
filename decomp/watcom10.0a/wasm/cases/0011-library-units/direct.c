/* Source/1994-instruction unit expectations; no original execution claim. */
#include <string.h>
#include <recon-tool.h>
#include "direct.h"
#include <recon.h>
#define CHECK(x) do { if (!(x)) return 0; } while (0)

int output_unit(void)
{
    unsigned char ascii[] = { 'A','S','C','I','I',0 };
    unsigned char high[] = { 0x80,0xff,0 };
    /* The original read-ahead requires an owned second byte after initial NUL. */
    unsigned char empty[] = { 0,0 };
    const unsigned char ascii_before[] = { 'A','S','C','I','I',0 };
    const unsigned char high_before[] = { 0x80,0xff,0 };
    output(ascii);
    output(high);
    output(empty);
    CHECK(memcmp(ascii,ascii_before,sizeof(ascii)) == 0);
    CHECK(memcmp(high,high_before,sizeof(high)) == 0);
    CHECK(empty[0] == 0 && empty[1] == 0);
    return 1;
}

int group_index_unit(void)
{
    asm_sym symbol, symbol_before;
    dir_node segment, segment_before;
    seg_info info, info_before;
    obj_rec record, record_before;
    symbol_queue saved, registered;
    int ok = 1;
    memset(&symbol,0,sizeof(symbol));
    memset(&segment,0,sizeof(segment));
    memset(&info,0,sizeof(info));
    memset(&record,0,sizeof(record));
    saved = Tables[TAB_SEG];
    Tables[TAB_SEG].head = Tables[TAB_SEG].tail = NULL;
    symbol.grpidx = 17;
    symbol.segidx = 23;
    symbol_before = symbol;
    ok &= GetGrpIdx(&symbol) == 17;
    ok &= memcmp(&symbol,&symbol_before,sizeof(symbol)) == 0;
    symbol.grpidx = 0;
    ok &= GetGrpIdx(&symbol) == 0;
    segment.sym.state = SYM_SEG;
    segment.e.seginfo = &info;
    info.segrec = &record;
    info.idx = 9;
    record.d.segdef.idx = 23;
    Tables[TAB_SEG].head = Tables[TAB_SEG].tail = &segment;
    registered = Tables[TAB_SEG];
    symbol_before = symbol;
    segment_before = segment;
    info_before = info;
    record_before = record;
    ok &= GetGrpIdx(&symbol) == 9;
    ok &= memcmp(&symbol,&symbol_before,sizeof(symbol)) == 0;
    symbol.segidx = 24;
    symbol_before = symbol;
    ok &= GetGrpIdx(&symbol) == 0;
    ok &= memcmp(&symbol,&symbol_before,sizeof(symbol)) == 0;
    ok &= memcmp(&segment,&segment_before,sizeof(segment)) == 0;
    ok &= memcmp(&info,&info_before,sizeof(info)) == 0;
    ok &= memcmp(&record,&record_before,sizeof(record)) == 0;
    ok &= memcmp(&Tables[TAB_SEG],&registered,sizeof(registered)) == 0;
    Tables[TAB_SEG] = saved;
    return ok;
}
int library_units(void)
{
    CHECK(output_unit());
    CHECK(group_index_unit());
    return 1;
}
