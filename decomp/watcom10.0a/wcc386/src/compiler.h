#ifndef WCC_COMPILER_H
#define WCC_COMPILER_H
#include <stdio.h>
#include <recon.h>
extern unsigned char _8087,_real87;
extern int ErrCount;
extern int TagCount,FieldCount,EnumCount,TypeCount;
/* Original CompFlags starts7d528: statistics bits4/28, no_conmsg bit39.
 * Name other flags only when their consumers are reconstructed. */
struct comp_flags {
    unsigned :4;
    unsigned stats_printed:1;
    unsigned quad_init_flag:1; /* Old QuadInit clears bit5. */
    unsigned :2;
    unsigned pre_processing:2;
    unsigned command_line_processed:1;
    unsigned :17;
    unsigned extra_stats_wanted:1;
    unsigned :1;
    unsigned scanning_comment:1;
    unsigned :1;
    unsigned :2;
    unsigned quiet_mode:1;
    unsigned :4;
    unsigned no_conmsg:1;
    unsigned :3;
    unsigned errfile_written:1;
    unsigned :13;
    unsigned banner_printed:1;
    unsigned :4;
    unsigned cpp_output:1;
};
extern struct comp_flags CompFlags;
extern int SwitchChar;
typedef struct fcb_struct FCB;
extern struct fname_list *FNames;
extern FCB *SrcFile;
extern FILE *ErrFile,*DefFile,*CppFile,*PageFile;
extern char *HFileList;
extern int PageHandle,SegmentNum;
void InitStats(void);
extern char *WholeFName,*ForceInclude;
extern void *Environment;
extern int PrintWhiteSpace;
void ParseInit(void);
void ScanInit(void);
extern int CurrChar,SizeOfCount;
extern int (*NextChar)();
int GetNextChar(void);
void CTypeInit(void);
void MacroInit(void);
void SymInit(void);
void SpcSymInit(void);
void StringInit(void);
void QuadInit(void);
void ExprInit(void);
void SegInit(void);
void ParseCmdLine(char **cmdline);
void PrintStats(void);
void EmitAbort(void);
void PragmaFini(void);
void FreeFNames(void);
void CErr1(int code);
void CErr2p(int code,char *text);
int CalcHash(char *name,int length);
void hashpjw(char *name);
void DelErrFile(void);
void OpenErrFile(void);
void MergeInclude(void);
void CPragmaInit(void);
void EmitInit(void);
void PrtChar(int character);
int OpenSrcFile(char *name,int flag);
void Parse(void);
void CMemInit(void);
void FrontEnd(char **cmdline);
void DoCCompile(char **cmdline);
void InitGlobalVars(void);
void *CPermAlloc(unsigned amount);
void *CMemAlloc(unsigned amount);
void CMemFree(void *memory);
void *CFastAlloc(unsigned amount);
void *FEmalloc(unsigned amount);
void CSuicide(void);
#if defined(__WATCOMC__) && defined(WCC_CSUICIDE_ABORTS)
/* Original declaration scope matters: only the reviewed symbol/leaf TUs
 * carry this tail-transfer annotation; ordinary callers retain CALL/RET.
 * Actual failure transfer resumes setjmp or leaves the CF caller entirely. */
#pragma aux CSuicide aborts;
#endif
void PageOutQuads(void);
void PageOutLeafs(void);
void PageOutSyms(void);
void InitMsg(void);
void CloseFiles(void);
void OpenPageFile(void);
void MakeTmpName(char *name);
int _dos_switch_char(void);
void ClearGlobals(void);
void PurgeMemory(void);
#endif
