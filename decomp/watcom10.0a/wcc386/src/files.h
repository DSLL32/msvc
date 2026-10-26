#ifndef WCC_FILES_H
#define WCC_FILES_H
#include "scanner.h"
#include <recon.h>
struct fname_list {
    struct fname_list *next;
    unsigned mtime;
    unsigned short index;
    char name[1];
};
#pragma pack(1)
typedef struct include_file {
    struct include_file *nextfile;
    int len;
    char filename[1];
} INCFILE;
#pragma pack()
extern INCFILE *IncFileList;
struct fname_list *AddFlist(char *name);
int SrcFileTime(char *name,unsigned *mtime);
char *CStrSave(char *name);
void AddIncFileList(char *name);
void OpenCppFile(void);
void EmitPoundLine(int line,char *name);
void __cdecl CppPrtf(char *format,...);
extern int TokenFno;
extern char *ErrFName;
extern int SrcLineCount,IncLineCount;
void CClose(FILE *stream);
void FEfree(void *memory);
void BuildPreCompiledHeader(char *name);
#endif
