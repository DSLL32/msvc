#ifndef WCC_ENCODE_H
#define WCC_ENCODE_H
#include "codegen.h"
#include <recon.h>
#pragma pack(1)
typedef struct oc_header { unsigned char objlen,kind,reclen; } oc_header;
#pragma pack()

#pragma pack(4)
typedef struct oc_handle { oc_header op;void *ref;struct code_lbl *handle; } oc_handle;
typedef union any_oc { oc_header op;oc_handle handle; } any_oc;
#pragma pack()
typedef char old_oc_handle_size[(sizeof(oc_handle)==12)?1:-1];
#pragma pack(4)
typedef struct ins_entry { struct { struct ins_entry *prev,*next; } ins;any_oc oc; } ins_entry;
#pragma pack()
typedef char old_ins_header[(offsetof(ins_entry,oc)==8)?1:-1];
extern ins_entry *FirstIns,*LastIns,*RetList,*PendingDeletes;
extern unsigned QCount;
extern void *InstrFrl;
unsigned char LDone(any_oc *record);
unsigned char PrevClass(ins_entry *entry);
void PullQueue(void);
ins_entry *NewInstr(any_oc *record);
void AddInstr(ins_entry *entry,ins_entry *after);
void OptPush(void);
void UnLinkInstr(ins_entry *entry);
extern unsigned char InsDelete;
void MultiLineNums(ins_entry *entry);
ins_entry * Untangle(ins_entry *entry);
void TraceCommon(ins_entry *entry);
void CallRet(ins_entry *entry);
ins_entry * PrevIns(ins_entry *entry);
unsigned char ComCode(ins_entry *entry);
void JmpRet(ins_entry *entry);
unsigned char RetAftrLbl(ins_entry *entry);
void RetAftrCall(ins_entry *entry);
unsigned char ComTail(ins_entry *list,ins_entry *entry);
void FreePendingDeletes(void);
void InsertQueue(ins_entry *entry,ins_entry *after);
void AddLblDef(ins_entry *entry);
void AddLblRef(ins_entry *entry);
void InputOC(any_oc *record);
#endif
