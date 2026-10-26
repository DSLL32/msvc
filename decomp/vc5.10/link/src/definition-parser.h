#ifndef LINK_DEFINITION_PARSER_H
#define LINK_DEFINITION_PARSER_H
#include <windows.h>
#include <stdio.h>
#include "link-driver.h"
#include <recon.h>
#ifdef __cplusplus
extern "C" {
#endif
extern const char *const definition_keywords[23];
extern FILE *definition_source;
extern char definition_line[4096];
extern char *definition_cursor,*definition_text;
extern DWORD definition_version;
extern ByteBuffer definition_directives;
WORD __cdecl next_definition_token(void);
char *__cdecl read_definition_line(void);
WORD __stdcall find_definition_keyword(const char *);
WORD __stdcall read_definition_name(LinkContext *,BYTE);
WORD __stdcall read_definition_description(char *);
WORD __stdcall read_definition_size(const char *,DWORD *,DWORD *);
WORD __stdcall read_definition_sections(const char *,char *);
WORD __stdcall read_definition_exports(LinkContext *,char *);
WORD __stdcall read_vxd_definition_export(char *);
WORD __stdcall read_definition_export_entry(LinkContext *,char *,const char *);
WORD __stdcall read_definition_stub(char *);
void __stdcall read_librarian_definition(LinkContext *,const char *);
#ifdef __cplusplus
}
#endif
#endif
