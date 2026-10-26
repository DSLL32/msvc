#ifndef WCC_SYMBOLS_H
#define WCC_SYMBOLS_H
#include "types.h"
#include <recon.h>
#define SYM_HASH_SIZE 241
#define MAX_SYM_SEGS 164
#define SYM_BUF_SIZE 1024
typedef struct sym_hash_entry *SYM_HASHPTR;
typedef unsigned short SYM_HANDLE;
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
    struct textsegment *seginfo;
    unsigned short sym_leaf,attrib,flags;
    unsigned char level,stg_class;
} SYM_ENTRY,*SYMPTR;
void NewSym(void);
void SymReplace(SYMPTR sym,SYM_HANDLE handle);
void SymGet(SYMPTR sym,SYM_HANDLE handle);
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
extern SYM_HANDLE NextSymHandle,CurFuncHandle;
extern FIELDPTR ErrSym;
extern unsigned SymBufNum,LastSymBuf,SymBufDirty;
extern unsigned SymSegNum;
extern SYM_ENTRY CurFuncSym;
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
void EnumInit(void);
#endif
