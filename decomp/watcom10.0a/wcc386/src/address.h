#ifndef WCC_ADDRESS_H
#define WCC_ADDRESS_H
#include "block.h"
#include <recon.h>
/* 1994 addrname.h: byte-sized classes/flags, before the alignment field. */
#pragma pack(4)
typedef struct address_name {
    unsigned short format;
    struct address_name *link;
    type_def *tipe;
    unsigned char class,flags;
    union { name *name;instruction *ins; } u;
    name *index;
    long offset;
    name *base;
} address_name;
#pragma pack()
typedef char old_address_size[(sizeof(address_name)==32)?1:-1];
typedef char old_address_union[(offsetof(address_name,u)==16)?1:-1];
address_name *NewAddrName(void);
name *Display(void *symbol,int level);
cg_type NamePtrType(name *value);
unsigned char AskNameCode(void *symbol,unsigned char kind);
unsigned char IsFarFunc(void *symbol);
unsigned short AskSegID(void *symbol,unsigned char kind);
unsigned char AskSegPrivate(unsigned short segment);
#pragma pack(4)
typedef struct bool_node {
    unsigned short format;
    struct code_lbl **t,**f,*e;
} bool_node;
#pragma pack()
typedef char old_bool_size[(sizeof(bool_node)==16)?1:-1];
void BoolFree(bool_node *node);
void FlowOff(struct bool_node *node);
void AddrFree(address_name *node);
void FreeIns(instruction *entry);
void FrlFreeSize(void **head,void *entry,int size);
void InsToAddr(address_name *addr);
name *GenIns(address_name *addr);
address_name *AddrName(name *value,type_def *type);
void CopyAddr(address_name *source,address_name *dest);
#endif
