#include <string.h>
#include <recon-tool.h>
#include "wasm.h"
#include <recon.h>
#define CHECK(x) do { if (!(x)) return 0; } while (0)
int object_smoke(void)
{
    obj_rec record;
    unsigned char data[6]={0xa5,0,0,0,0,0x5a};
    memset(&record,0,sizeof(record));record.data=data+1;record.length=4;
    ObjPut32(&record,0x12345678UL);
    CHECK(record.curoff == 4 && record.length == 4 && record.data == data+1);
    CHECK(memcmp(data,"\xa5\x78\x56\x34\x12\x5a",6) == 0);
    return 1;
}
static int width_roundtrip(void)
{
    struct { unsigned char before,data[24],after; } owner;
    static const unsigned char expected[]={0x81,0x34,0x12,0xef,0xcd,0xab,0x89};
    obj_rec *record;
    int mode;
    ObjRecInit();
    for(mode=0;mode<3;++mode) {
        memset(&owner,0xa5,sizeof(owner));
        record=ObjNewRec(CMD_THEADR);ObjAttachData(record,owner.data,7);
        record->is_32=(mode==1);record->is_phar=(mode==2);
        ObjPut8(record,0x81);ObjPut16(record,0x1234);ObjPut32(record,0x89abcdefUL);
        CHECK(record->curoff == 7 && memcmp(owner.data,expected,7) == 0);
        record->curoff=0;
        CHECK(ObjGet8(record) == 0x81 && ObjGet16(record) == 0x1234);
        CHECK(ObjGet32(record) == 0x89abcdefUL && record->curoff == 7);
        record->curoff=3;
        CHECK(ObjGetEither(record) == (mode ? 0x89abcdefUL : 0xcdefUL));
        CHECK(record->curoff == (mode ? 7 : 5));
        CHECK(record->data == owner.data && record->length == 7 && !record->free_data);
        CHECK(record->is_32 == (mode==1) && record->is_phar == (mode==2));
        CHECK(memcmp(owner.data,expected,7) == 0 && owner.before == 0xa5 && owner.after == 0xa5);
        CHECK(owner.data[7] == 0xa5 && owner.data[23] == 0xa5);
        ObjKillRec(record);
        CHECK(memcmp(owner.data,expected,7) == 0 && owner.before == 0xa5 && owner.after == 0xa5);
    }
    ObjRecFini();return 1;
}
static int index_roundtrip(void)
{
    static const unsigned short values[]={0,0x7f,0x80,0x123,0x7fff};
    static const unsigned char expected[]={0,0x7f,0x80,0x80,0x81,0x23,0xff,0xff};
    unsigned char data[10];
    obj_rec *record;
    unsigned i;
    memset(data,0xa5,sizeof(data));ObjRecInit();
    record=ObjNewRec(CMD_THEADR);ObjAttachData(record,data+1,8);
    for(i=0;i<sizeof(values)/sizeof(values[0]);++i) ObjPutIndex(record,values[i]);
    CHECK(record->curoff == 8 && memcmp(data+1,expected,8) == 0);
    record->curoff=0;
    for(i=0;i<sizeof(values)/sizeof(values[0]);++i) CHECK(ObjGetIndex(record) == values[i]);
    CHECK(record->curoff == 8 && record->length == 8 && record->data == data+1 && !record->free_data);
    CHECK(memcmp(data+1,expected,8) == 0 && data[0] == 0xa5 && data[9] == 0xa5);
    ObjKillRec(record);ObjRecFini();
    CHECK(memcmp(data+1,expected,8) == 0 && data[0] == 0xa5 && data[9] == 0xa5);
    return 1;
}
const ReconTest object_tests[]={
    {"16-32-PharLap-widths-and-borrowed-storage",1,width_roundtrip},
    {"OMF-index-boundaries-and-cursor",1,index_roundtrip}
};
