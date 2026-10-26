/* Source-authored OMF payload expectations, not original executable data. */
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <unistd.h>
#include <recon-tool.h>
#include "wasm.h"
#include <recon.h>
#define CHECK(x) do { if (!(x)) return 0; } while (0)
int womp_smoke(void)
{
    unsigned char buffer[6] = {0xa5,0,0,0,0,0x5a};
    CHECK(put32((char *)buffer+1,0x12345678UL) == (char *)buffer+5);
    CHECK(memcmp(buffer,"\xa5\x78\x56\x34\x12\x5a",6) == 0);
    return 1;
}
static int record_bytes(unsigned char command,const unsigned char *body,unsigned length)
{
    unsigned char bytes[256];
    unsigned i,sum=0;
    int fd,count;
    fd=open("records.obj",O_BINARY);CHECK(fd >= 0);
    count=read(fd,bytes,sizeof(bytes)); CHECK(close(fd) == 0);
    CHECK(count == length+4 && bytes[0] == command);
    CHECK(bytes[1] == (unsigned char)(length+1) && bytes[2] == (length+1)/256);
    CHECK(memcmp(bytes+3,body,length) == 0);
    for(i=0;i<(unsigned)count;++i) sum+=bytes[i];
    CHECK((sum&255) == 0 && unlink("records.obj") == 0);
    return 1;
}
static int physical_references(void)
{
    physref owner={0x1234,0x89abcdefUL},before;
    unsigned char buffer[10];
    int mode;
    before=owner;
    for(mode=0;mode<3;++mode) {
        memset(buffer,0xa5,sizeof(buffer));
        CHECK(FixGenPRef(&owner,(char *)buffer+1,mode) == (mode==FIX_GEN_PHARLAP ? 6 : 4));
        CHECK(buffer[0] == 0xa5 && buffer[mode==FIX_GEN_PHARLAP ? 7 : 5] == 0xa5);
        CHECK(memcmp(buffer+1,mode==FIX_GEN_PHARLAP ? "\x34\x12\xef\xcd\xab\x89" : "\x34\x12\xef\xcd",mode==FIX_GEN_PHARLAP ? 6 : 4) == 0);
        CHECK(memcmp(&owner,&before,sizeof(owner)) == 0);
    }
    return 1;
}
static int miscellaneous_records(void)
{
    unsigned char data[]={0x12,0x34,0x56},copy[3];
    obj_rec *record;
    pobj_state state;
    int mode;
    memcpy(copy,data,3);ObjRecInit();
    for(mode=0;mode<3;++mode) {
        state.pass=2;state.file_out=ObjWriteOpen("records.obj");CHECK(state.file_out != NULL);
        record=ObjNewRec(CMD_BAKPAT);ObjAttachData(record,data,sizeof(data));record->curoff=2;
        record->is_32=(mode==1);record->is_phar=(mode==2);
        CHECK(writeMisc32(record,&state) == 0 && record->curoff == 2);
        CHECK(record->data == data && !record->free_data && memcmp(data,copy,3) == 0);
        ObjKillRec(record);ObjWriteClose(state.file_out);
        CHECK(record_bytes(mode ? 0xb3 : CMD_BAKPAT,data,3));
    }
    ObjRecFini();return 1;
}
static int iterated_records(void)
{
    /* Two leaf blocks nested in one repeated block; original16-bit repeat fields. */
    unsigned char data[]={2,0,2,0,3,0,0,0,1,'A',1,0,0,0,2,'B','C'},copy[17];
    static const unsigned char converted[]={2,0,0,0,2,0,3,0,0,0,0,0,1,'A',1,0,0,0,0,0,2,'B','C'};
    unsigned char expected[40],data32[sizeof(converted)],*input;
    unsigned input_length;
    obj_rec *record;
    pobj_state state;
    unsigned prefix;
    int mode;
    memcpy(copy,data,sizeof(data));memcpy(data32,converted,sizeof(data32));ObjRecInit();
    for(mode=0;mode<3;++mode) {
        state.pass=2;state.file_out=ObjWriteOpen("records.obj");CHECK(state.file_out != NULL);
        input=mode==1 ? data32 : data;
        input_length=mode==1 ? sizeof(data32) : sizeof(data);
        record=ObjNewRec(CMD_LIDATA);ObjAttachData(record,input,input_length);
        record->is_32=(mode==1);record->is_phar=(mode==2);
        record->d.lidata.idx=0x123;record->d.lidata.offset=0x5678;
        CHECK(writeLidata(record,&state) == 0 && record->curoff == 0);
        CHECK(record->data == input && record->length == input_length && !record->free_data);
        CHECK(memcmp(data,copy,sizeof(data)) == 0 && memcmp(data32,converted,sizeof(data32)) == 0);
        ObjKillRec(record);ObjWriteClose(state.file_out);
        expected[0]=0x81;expected[1]=0x23;expected[2]=0x78;expected[3]=0x56;
        prefix=4;if(mode) {expected[4]=0;expected[5]=0;prefix=6;}
        memcpy(expected+prefix,mode ? converted : data,mode ? sizeof(converted) : sizeof(data));
        CHECK(record_bytes(mode ? 0xa3 : CMD_LIDATA,expected,prefix+(mode ? sizeof(converted) : sizeof(data))));
    }
    GenMSOmfFini();ObjRecFini();return 1;
}
static int comdat_records(void)
{
    unsigned char data[]={3,0,0,0,2,'X','Y'},before[7],expected[40];
    obj_rec *record;
    pobj_state state;
    unsigned length;
    int mode;
    memcpy(before,data,7);ObjRecInit();
    for(mode=0;mode<3;++mode) {
        state.pass=2;state.file_out=ObjWriteOpen("records.obj");CHECK(state.file_out != NULL);
        record=ObjNewRec(CMD_COMDAT);ObjAttachData(record,data,7);
        record->is_32=(mode==1);record->is_phar=(mode==2);
        record->d.comdat.base.grp_idx=0;record->d.comdat.base.seg_idx=0;
        record->d.comdat.base.frame=0x1234;
        record->d.comdat.flags=(mode==2 ? 2 : 0);record->d.comdat.attributes=0;
        record->d.comdat.align=1;record->d.comdat.offset=0x5678;
        record->d.comdat.type_idx=0x123;record->d.comdat.public_name_idx=7;
        CHECK(writeComdat(record,&state) == 0 && record->curoff == 0);
        CHECK(record->data == data && !record->free_data && memcmp(data,before,7) == 0);
        ObjKillRec(record);ObjWriteClose(state.file_out);
        length=0;expected[length++]=(mode==2 ? 2 : 0);expected[length++]=0;expected[length++]=1;
        expected[length++]=0x78;expected[length++]=0x56;
        if(mode) {expected[length++]=0;expected[length++]=0;}
        expected[length++]=0x81;expected[length++]=0x23; /* type index */
        expected[length++]=0;expected[length++]=0;expected[length++]=0x34;expected[length++]=0x12;
        expected[length++]=7;
        if(mode==2) {
            static const unsigned char widened[]={3,0,0,0,0,0,2,'X','Y'};
            memcpy(expected+length,widened,sizeof(widened));length+=sizeof(widened);
        } else {memcpy(expected+length,data,7);length+=7;}
        CHECK(record_bytes(mode ? 0xc3 : CMD_COMDAT,expected,length));
    }
    GenMSOmfFini();ObjRecFini();return 1;
}
static int line_symbols(void)
{
    obj_rec *record;
    pobj_state state;
    struct linnum_data *lines,before[2];
    static const unsigned char expected16[]={1,0x81,0x23,7,0,0x34,0x12,8,0,0x78,0x56};
    static const unsigned char expected32[]={1,0x81,0x23,7,0,0x34,0x12,0,0,8,0,0x78,0x56,0,0};
    int mode;
    ObjRecInit();
    for(mode=0;mode<2;++mode) {
        state.pass=2;state.file_out=ObjWriteOpen("records.obj");CHECK(state.file_out != NULL);
        record=ObjNewRec(CMD_LINSYM);record->is_32=mode;
        lines=MemAlloc(2*sizeof(*lines));CHECK(lines != NULL);
        lines[0].number=7;lines[0].offset=0x1234;lines[1].number=8;lines[1].offset=0x5678;
        memcpy(before,lines,sizeof(before));
        record->d.linsym.d.linsym.flags=1;record->d.linsym.d.linsym.public_name_idx=0x123;
        record->d.linsym.num_lines=2;record->d.linsym.lines=lines;
        CHECK(writeLinsym(record,&state) == 0);
        CHECK(record->d.linsym.lines == lines && memcmp(lines,before,sizeof(before)) == 0);
        ObjKillRec(record);ObjWriteClose(state.file_out);
        CHECK(record_bytes(mode ? 0xc5 : CMD_LINSYM,mode ? expected32 : expected16,mode ? sizeof(expected32) : sizeof(expected16)));
    }
    ObjRecFini();return 1;
}
static int relocation_ranges(void)
{
    lifix_list list;
    LifixInit(&list);
    LifixAdd(&list,2,2);LifixAdd(&list,6,4);LifixAdd(&list,12,6);
    CHECK(list.head->lower_bound == 12 && list.head->delta == 6);
    CHECK(list.head->next->lower_bound == 6 && list.head->next->next->lower_bound == 2);
    CHECK(list.head->next->next->next == NULL);
    CHECK(LifixDelta(&list,0) == 0 && LifixDelta(&list,2) == 2);
    CHECK(LifixDelta(&list,5) == 2 && LifixDelta(&list,6) == 4);
    CHECK(LifixDelta(&list,11) == 4 && LifixDelta(&list,12) == 6 && LifixDelta(&list,65535) == 6);
    LifixDestroy(&list); /* Original destroy does not clear head; do not dereference it. */
    return 1;
}
const ReconTest womp_tests[] = {
    {"physical-reference-widths-and-owner",1,physical_references},
    {"miscellaneous-record-command-width-and-cursor",1,miscellaneous_records},
    {"nested-iterated-data-and-PharLap-widening",1,iterated_records},
    {"explicit-COMDAT-base-and-iterated-payload",1,comdat_records},
    {"line-symbol-widths-and-owned-line-array",1,line_symbols},
    {"ordered-relocation-delta-ownership",1,relocation_ranges}
};
