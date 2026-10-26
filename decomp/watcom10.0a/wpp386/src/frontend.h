#ifndef WPP_FRONTEND_H
#define WPP_FRONTEND_H
#include <stdio.h>
#include <recon.h>
/* Recovered byte/bit positions of encountered1994 compiler flags. */
struct comp_flags {
    unsigned char reserved0;
    unsigned char reserved1a:1, scanning_c_comment:1, stats_printed:1, check_syntax:1, quiet_mode:1, reserved1b:2, extra_stats_wanted:1;
    unsigned char external_defn_found:1, reserved2:6, no_conmsg:1;
    unsigned char reserved3a:1, banner_printed:1, reserved3b:6;
    unsigned char reserved4a:5, cpp_output:1, reserved4b:2;
    unsigned char reserved5;
    unsigned char reserved6a:3, warnings_cause_bad_exit:1, reserved6b:4;
    unsigned char reserved7, reserved8, reserved9;
    unsigned char reserved10:7, fatal_error:1;
    unsigned char reserved11a:1, use_pcheaders:1, watch_for_pcheader:1, reserved11b:5;
};
extern struct comp_flags CompFlags;
extern unsigned WngCount, ErrCount;
extern unsigned SrcLineCount, IncLineCount;
extern char *ForceInclude, *WholeFName;
extern void *Environment;
void ScanInit(void);
void parseCmdLine(char **argv);
void CBanner(void);
void CCusage(void);
void GenCOptions(char **argv);
void MakePgmName(char *filename);
int makeExitStatus(int fatal);
void PTypeCheckInit(void);
void ErrFileOpen(void);
void PpOpen(void);
void OpenPgmFile(void);
void PrtChar(int ch);
int OpenSrcFile(char *filename, int is_lib);
void PpParse(void);
void CgFrontModInitInit(void);
int NextToken(void);
void ParseDecls(void);
void RtnGenerate(void);
void ModuleInitFini(void);
void ScopeEndFileScope(void);
void ScopeCreatePCHDebugSym(void);
void CgBackEnd(void);
void CgFrontFini(void);
void CloseFiles(void);
int IoSuppCloseFile(FILE **file);
extern FILE *DefFile;
int SrcFileFClose(FILE *fp);
void IoSuppSetLineBuffering(FILE *fp, unsigned size);
void set_buffering(FILE *fp, unsigned size, int mode);
/* Exact original e1d1 is a shared empty host hook; original name unknown. */
void frontend_host_hook(void);
void resetHandlers(void);
int _dos_switch_char(void);
void PpInit(void);
void PpStartFile(void);
extern FILE *CppFile;
extern unsigned char PreProcChar;
int doCCompile(char **argv);
#endif
