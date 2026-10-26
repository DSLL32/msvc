#ifndef WCC_OPTIONS_H
#define WCC_OPTIONS_H
#include "command.h"
#include <recon.h>
/* The 1994 x86 register set occupies one DWORD; the later released
 * multi-target structure grew. The full aux_info has ten DWORDs, as proved by PragmaInit/PragInit copies. */
typedef unsigned hw_reg_set;
struct aux_info {
    unsigned class;
    void *code;
    hw_reg_set *parms;
    hw_reg_set returns,streturn,save;
    char *objname;
    int use;
    int flags,aux_info_index;
};
#define FLOATING_FS 0x40U
#define FLOATING_GS 0x80U
#define FLOATING_ES 0x100U
#define FLOATING_DS 0x400U
#define HW_EAX 0x01000003U
#define HW_EBX 0x0200000cU
#define HW_ECX 0x04000030U
#define HW_EDX 0x080000c0U
#define HW_ESI 0x10000100U
#define HW_DS 0x1000U
#define HW_ES 0x2000U
#define HW_FS 0x40000000U
#define HW_GS 0x80000000U
#define HW_FLTS 0x00ff0000U
extern unsigned ProcRevision,TargetSwitches,GenSwitches;
extern unsigned char Stack87;
extern char *CodeClassName,*PCH_FileName,*TextSegName,*DataSegName,*GenCodeGroup;
extern int TargSys;
enum target_system { TS_OTHER,TS_DOS,TS_NT,TS_WINDOWS,TS_CHEAP_WINDOWS,TS_PENPOINT,TS_NETWARE,TS_QNX };
extern unsigned Toggles,DataThreshold,OptSize;
extern int ErrLimit,WngLevel,PreProcChar;
struct undef_names { struct undef_names *next; char *name; };
extern struct undef_names *UndefNames;
extern char *ObjectFileName,*ModuleName;
extern hw_reg_set DefaultParms[2];
extern unsigned PackAmount,GblPackAmount;
extern struct aux_info DefaultInfo;
/* Descriptive name: this earlier parser's option sets byte77f78, retaining
 * the floating register save set. No matching later-source symbol survives. */
extern unsigned char Save8087Regs;
extern char *TargetSystemName;
extern int DataPtrSize,CodePtrSize;
extern char CLIB_Name[8],MATHLIB_Name[10];
extern char *EmuLib_Name;
void InitModInfo(void);
void InitCPUModInfo(void);
void ProcOptions(char *options,unsigned depth);
char *ProcessOption(char *option);
void SetTargSystem(void);
void SetGenSwitches(void);
void Define_Memory_Model(void);
void SetStackConventions(void);
void MacroDefs(void);
void MiscMacroDefs(void);
void AddIncList(char *text,unsigned length);

struct peg_options {
    unsigned ds_used:1,ds_on:1;
    unsigned es_used:1,es_on:1;
    unsigned fs_used:1,fs_on:1,gs_used:1,gs_on:1;
    unsigned nd_used:1;
};
extern struct peg_options PegOptions;
extern unsigned char CpuModel,FpuMode,FpuLevel,MemoryModel;
extern unsigned ProEpiDataSize,UnrollCount;
int fndlen(char *text);
char *copy_eq(char *destination,char *source);
char *BadCmdLine(int code,char *text);
int EqualChar(unsigned char character);
int OptionDelimiter(int character);
char *DefineCmdLineMacro(char *text);
char *ProcessFPUStack(char *text);
char *ProcessMemoryModel(char *text);
char *ProcessSegmentName(char *text);
char *ProcessOptimization(char *text);
char *ProcessFileOption(char *text);
unsigned FileOptionLength(char *text);
char *AddIncludePath(char *text);
char *AddUndefName(char *text);
char *SetDataThreshold(char *text);
char *LoadUnicodeTable(char *text);
void SetCppWidth(int width);
void SetTargName(char *name,unsigned length);
void SetSignedChar(void);
void SetDBChar(int kind);
#endif
