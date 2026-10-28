#ifndef WCC_DBINFO_H
#define WCC_DBINFO_H
#include <stddef.h>
#include <recon.h>
typedef unsigned short cue_idx;
typedef struct cue_state {
    cue_idx cue;
    short fno;
    unsigned short line,col;
} cue_state;
struct cue_blk;
typedef struct cue_ctl {
    struct cue_blk *head,*curr,**lnk;
    cue_state *next,*end;
    cue_state state,start[1];
    long count;
} cue_ctl;
typedef char old_cue_state_size[(sizeof(cue_state)==8)?1:-1];
typedef char old_cue_ctl_size[(sizeof(cue_ctl)==40)?1:-1];
#pragma pack(2)
struct fname_lst {
    struct fname_lst *next;
    unsigned short len;
    char fname[1];
};
struct fname_ctl { struct fname_lst *lst;short count; };
#pragma pack()
extern struct fname_ctl DBFiles;
extern cue_ctl LineInfo;
extern cue_idx SrcLine;
void SrcFileNoInit(void);
char DBNested(char nested);
void SourceCueInit(cue_ctl *ctl);
void DBSrcCue(unsigned file,unsigned line,unsigned column);
void DBLineNum(unsigned line);
void OutSourceLine(unsigned short line);
void AddCueBlk(cue_ctl *ctl);
void *FEAuxInfo(void *handle,unsigned char request);
unsigned DBSrcFile(char *name);
cue_idx CueAdd(int file,int line,int column);
void DFInitDbgInfo(void);
void CVInitDbgInfo(void);
void WVInitDbgInfo(void);
#pragma pack(4)
typedef struct dbg_patch_handle { unsigned segment,offset; } dbg_patch_handle;
typedef struct temp_buff {
    unsigned index,fix_idx;
    struct { void *p;unsigned char pos,type; } fix[2];
    unsigned char buff[512];
} temp_buff;
#pragma pack()
typedef char old_temp_size[(sizeof(temp_buff)==536)?1:-1];
typedef char old_temp_data[(offsetof(temp_buff,buff)==24)?1:-1];
void BuffStart(temp_buff *temp,unsigned definition);
void BuffForward(dbg_patch_handle *handle);
void BuffRelocatable(void *pointer,unsigned char type,unsigned long offset);
void BuffWord(unsigned value);
void BuffByte(unsigned char value);
void Zoiks(int message);
void BuffOffset(unsigned long value);
void EndType(unsigned char check_size);
void BuffEnd(unsigned short segment);
void ChkDbgSegSize(unsigned size,unsigned char need);
void WVSrcCueLoc(void);
void DFObjFiniDbgInfo(unsigned long code_size);
void CVObjFiniDbgInfo(void);
extern unsigned short TypeIdx;
extern short DbgLocals,DbgTypes;
#endif
