#ifndef WPP_SRCFILE_H
#define WPP_SRCFILE_H
#include <stdio.h>
#include <recon.h>
typedef struct src_file *SRCFILE;
void SrcFileOpen(FILE *stream, char *name);
SRCFILE SrcFileCurrent(void);
int SrcFilesOpen(void);
char *SrcFileName(SRCFILE file);
SRCFILE SrcFileIncluded(SRCFILE file, unsigned *line);
void SrcFileCommand(void);
void SrcFileLibrary(void);
void SrcFilePrimary(void);
void SrcFileSetCreatePCHeader(void);
int SrcFileProcessOnce(char *name);
#endif
