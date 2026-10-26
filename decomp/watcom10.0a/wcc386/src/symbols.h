#ifndef WCC_SYMBOLS_H
#define WCC_SYMBOLS_H
#include <stddef.h>
#include "types.h"
#include <recon.h>
#define SYM_HASH_SIZE 241
#define MAX_SYM_SEGS 164
#define SYM_BUF_SIZE 1024
typedef struct sym_hash_entry *SYM_HASHPTR;
typedef unsigned short SYM_HANDLE;
#pragma pack(1)
struct sym_hash_entry {
    struct sym_hash_entry *next_sym;
    TYPEPTR sym_type;
    SYM_HANDLE handle;
    unsigned char level;
    char name[1];
};
#pragma pack()
typedef unsigned long SEGADDR_T;
#pragma pack(1)
typedef struct field_entry {
    struct field_entry *next_field;
    TYPEPTR field_type;
    unsigned long offset;
    char name[1];
} FIELD_ENTRY,*FIELDPTR;
#pragma pack()
/* The old storage-class byte is a token value, before the later bitfields. */
enum stg_classes { SC_NULL=0,SC_AUTO=0x39,SC_STATIC=0x5d,SC_EXTERN=0x61 };
#define SYM_FUNCTION 1
#define SYM_REFERENCED 2
#define SYM_ASSIGNED 0x40
#pragma pack(1)
typedef struct symtab_entry {
    char *name;
    TYPEPTR sym_type;
    SYM_HANDLE handle;
    unsigned short defn_file_index;
    union { int defn_line,index; } d;
    union { void *backinfo; int hash_value; void *return_var; } info;
    union {
        struct { int offset,segment; } var;
        struct { SYM_HANDLE parms,locals; unsigned short number_of_quads,start_of_func; } func;
    } u;
    unsigned short sym_leaf;
    struct textsegment *seginfo;
    unsigned short attrib,flags;
    unsigned char level,stg_class;
} SYM_ENTRY,*SYMPTR;
#pragma pack()
/* Recovered unaligned 1994 symbol ABI; keep both compilers honest. */
typedef char sym_size_is_40[(sizeof(SYM_ENTRY)==40)?1:-1];
typedef char sym_leaf_at_28[(offsetof(SYM_ENTRY,sym_leaf)==28)?1:-1];
typedef char sym_segment_at_30[(offsetof(SYM_ENTRY,seginfo)==30)?1:-1];
typedef char sym_attrib_at_34[(offsetof(SYM_ENTRY,attrib)==34)?1:-1];
typedef char sym_flags_at_36[(offsetof(SYM_ENTRY,flags)==36)?1:-1];
typedef char sym_level_at_38[(offsetof(SYM_ENTRY,level)==38)?1:-1];
typedef char sym_storage_at_39[(offsetof(SYM_ENTRY,stg_class)==39)?1:-1];
void NewSym(void);
SYM_HANDLE GetNewSym(SYMPTR sym,char id,TYPEPTR typ,int stg_class);
SYM_HANDLE MakeNewSym(SYMPTR sym,char id,TYPEPTR typ,int stg_class);
SYM_HASHPTR SymHash(SYMPTR sym,SYM_HANDLE handle);
void SymReplace(SYMPTR sym,SYM_HANDLE handle);
void SymGet(SYMPTR sym,SYM_HANDLE handle);
SYMPTR SymGetPtr(SYM_HANDLE handle);
char *SymName(SYMPTR sym,SYM_HANDLE handle);
SYM_HANDLE SegSymbol(char *name);
SYM_HANDLE MakeFunction(char *name,TYPEPTR typ);
struct seg_info {
    SEGADDR_T index;
    unsigned allocated:1;
    unsigned in_EMS:1;
    unsigned in_farmem:1;
    unsigned in_page_file:1;
};
extern SYM_HASHPTR *HashTab,HashFreeList;
extern struct tag_entry *TagHead,*DeadTags;
extern struct label_entry *LabelHead;
extern int SymLevel;
extern SYM_HANDLE NextSymHandle,CurFuncHandle,GlobalSym,MainSymHandle,SymMEMCMP,SpecialSyms;
extern SYM_HANDLE TrySymHandle;
extern SYM_HANDLE SymMIN,SymMAX,SymTryInit,SymExcept,SymTryUnwind;
extern SYM_HANDLE SymSTOSB,SymSTOSD,SymFinally,SymTryFini,SymSTOD,SymSTOW,SymSTOWB;
extern FIELDPTR ErrSym;
extern unsigned SymBufNum,LastSymBuf,SymBufDirty;
extern unsigned SymSegNum;
extern SYM_ENTRY CurFuncSym;
extern SYMPTR CurFunc;
int VarParm(SYMPTR sym);
extern int PCH_MaxSymHandle;
extern SYMPTR *PCH_SymArray;
void SymAccess(unsigned handle);
extern struct seg_info SymSegs[MAX_SYM_SEGS];
extern char *SymBuffer,*SymBufPtr;
extern SEGADDR_T SymSegment;
SYM_HASHPTR SymHashAlloc(unsigned amount);
TYPEPTR TypeDefault(void);
SEGADDR_T AllocSegment(struct seg_info *segment);
SEGADDR_T AccessSegment(struct seg_info *segment);
void PageRead(char *buffer,unsigned count);
void PageWrite(char *buffer,unsigned count);
void PageSeek(unsigned long offset);
void FreeTags(void);
SYM_HASHPTR FreeSym(void);
SYM_HASHPTR GetSymList(void);
int SymBucket(SYMPTR sym);
void AssignSeg(SYMPTR sym);
void ChkFunction(SYMPTR sym,char *name);
void ChkIncomplete(SYMPTR sym,char *name);
void SetSymLoc(SYMPTR sym);
void ChkDefined(SYMPTR sym,char *name);
void ChkReference(SYMPTR sym,char *name);
void AsgnSegs(SYM_HANDLE handle);
void EndBlock(void);
void EnumInit(void);
#endif
