#ifndef WCC_COMMAND_H
#define WCC_COMMAND_H
#include "compiler.h"
#include <recon.h>
void BannerMsg(const char *line);
void CBanner(void);
void ConsMsg(const char *line);
void Wait_for_return(void);
void MyExit(int status);
#if defined(__WATCOMC__) && !defined(WCC_DEFINE_EXIT)
#pragma aux MyExit aborts;
#endif
extern char *_Copyright;
void CCusage(void);
void GenCOptions(char **cmdline);
void MakePgmName(char *name);
int OpenPgmFile(void);
extern FCB *MainSrcFile;
#endif
