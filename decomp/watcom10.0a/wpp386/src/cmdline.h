#ifndef WPP_CMDLINE_H
#define WPP_CMDLINE_H
#include <recon.h>
extern unsigned CpuSwitches, TargetSwitches, GenSwitches;
extern unsigned PackAmount, GblPackAmount;
extern unsigned FloatSaveOverride;
extern unsigned DataThreshold, OptSize, PragToggle;
extern unsigned ErrLimit, WngLevel;
extern char *ObjectFileName, *ModuleName;
extern unsigned char DestructorMethod, Stack87;
extern char *CodeClassName, *TextSegName, *DataSegName, *GenCodeGroup;
extern char *target_name;
extern unsigned TargetSystem;
extern unsigned DataPtrSize, CodePtrSize;
extern char CLIB_Name[8], WCPPLIB_Name[10], MATHLIB_Name[10];
extern char *EmuLib_Name;
typedef struct cmd_scan_state {
    const char *position, *switch_addr;
    unsigned cmd_file;
} CMD_SCAN_STATE;
extern int SwitchChar;
void CtxSwitchAddr(const char *position);
void CmdScanInit(const char *position);
int CmdScanChar(void);
int CmdScanLowerChar(void);
int CmdScanWhiteSpace(void);
void CmdScanSwitchBegin(void);
const char *CmdScanUngetChar(void);
const char *CmdScanAddr(void);
unsigned CmdScanId(const char **name);
unsigned CmdScanFilename(const char **name);
char *get_env(const char *name, unsigned length);
int openCmdFile(const char *name, unsigned length);
void saveCmdState(CMD_SCAN_STATE *state);
void restoreCmdState(const CMD_SCAN_STATE *state);
void processCmdFile(void);
void BadCmdLine(unsigned message);
void processSwitch(void);
void scanInputFile(void);
void InitModInfo(void);
void CmdSysInit(void);
void procOptions(char *options);
void setFinalTargetSystem(void);
void ConcatBase10(char *buffer, unsigned value);
void setMemoryModel(void);
int IsBigData(void);
int IsHugeData(void);
int IsBigCode(void);
void PreDefineStringMacro(char *definition);
void setStackConventions(void);
void macroDefs(void);
void MiscMacroDefs(void);
#endif
