#ifndef WCC_PRAGMA_H
#define WCC_PRAGMA_H
#include "options.h"
#include <recon.h>
extern struct aux_info CdeclInfo,PascalInfo,FortranInfo,SyscallInfo,StdcallInfo;
extern struct aux_info STOSBInfo,Far16CdeclInfo,Far16PascalInfo;
struct library_list { struct library_list *next;char name[1]; };
extern struct library_list *HeadLibs;
int CppPrinting(void);
int GetNextToken(void);
void PrtToken(void);
int PragRecog(char *name);
void PragFlag(int value);
void PragAux(void);
void PragLibs(void);
void PragPack(void);
void PragAllocText(void);
void PragCodeSeg(void);
void PragDataSeg(void);
void PragEnableDisableMessage(int enable);
void PragIntrinsic(int enable);
void PragUnroll(void);
void EndOfPragma(void);
int ParmsToBeReversed(int flags,struct aux_info *info);
struct symtab_entry;
struct aux_info *FindInfo(struct symtab_entry *sym,unsigned short handle);
int FunctionAborts(struct symtab_entry *sym,unsigned short handle);
struct aux_entry {
    union { struct aux_info *info; int aux_info_index; } u;
    struct aux_entry *next;
    char name[1];
};
extern struct aux_entry *AuxList;
struct aux_entry *AuxLookup(char *name);
struct aux_info *InfoLookup(struct symtab_entry *sym);
struct inline_funcs {
    char *name;
    void *code;
    hw_reg_set *parms;
    hw_reg_set returns,save;
};
struct inline_funcs *IF_Lookup(char *name);
struct aux_info *ModifyLookup(struct symtab_entry *sym);
void *FEAuxInfo(void *handle,unsigned char request);
void GetCallClass(unsigned short handle);
char *ObjFileName(char *extension);
char *SegClassName(unsigned segment);
struct fname_list;
unsigned long *GetTimeStamp(struct fname_list *file);
void *SegPeggedReg(unsigned segment);
struct aux_info *LangInfo(unsigned flags,struct aux_info *info);
int VarFunc(struct symtab_entry *sym);
extern unsigned CallClass;
void PragmaInit(void);
void PragInit(void);
void AsmInit(int cpu,int fpu,int use32);
#endif
