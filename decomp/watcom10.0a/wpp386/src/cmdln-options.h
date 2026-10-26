#ifndef WPP_CMDLINE_OPTIONS_H
#define WPP_CMDLINE_OPTIONS_H
#include "macro.h"
#include <recon.h>
extern unsigned ProEpiDataSize, ZOptionNumber;
void CmdLnWarnDebug(void);
MEPTR DefineCmdLineMacro(int extended);
void undefineMacro(void);
unsigned VerifyPackAmount(unsigned pack);
void setTarget(const char *name, unsigned length);
void scanTarget(void);
void scanNumber(int *value);
void getNumber(unsigned *value, unsigned fallback);
void scanIncludeOption(void);
void scanFileName(char **name, int quoted);
void scanSegmentOption(void);
void scanMemoryModel(void);
void setMemoryModelOption(int model);
void scanOptimization(void);
void scanDataThreshold(void);
void scanFOption(void);
void scanPreprocess(void);
void scanFpuSetup(void);
void scanRegConvention(void);
int scan87Suffix(void);
int scanTwoChars(int first, int second);
void loadUnicodeTable(void);
int CmdScanSwEnd(void);
int CmdDelimitChar(void);
void CgBackSetOeSize(unsigned value);
void PCHSetFileName(char *name);
int CmdPeekChar(void);
int CmdRecogChar(int expected);
int CmdScanRecogEquals(void);
void PTypeSignedChar(void);
void SetDBChar(int language);
#endif
