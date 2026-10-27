#ifndef WPP_DBINFO_H
#define WPP_DBINFO_H
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
struct symbol;
void *FEAuxInfo(struct symbol *symbol,unsigned char request);
unsigned DBSrcFile(char *name);
void AddCueBlk(cue_ctl *ctl);
cue_idx CueAdd(int file,int line,int column);
void DFInitDbgInfo(void);
void CVInitDbgInfo(void);
void WVInitDbgInfo(void);
#endif
