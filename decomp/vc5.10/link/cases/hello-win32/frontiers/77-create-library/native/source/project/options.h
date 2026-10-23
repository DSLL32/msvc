#ifndef LINK_OPTIONS_H
#define LINK_OPTIONS_H
#include <windows.h>
#include "link-driver.h"
#include "arguments.h"
#include <recon.h>
#ifdef __cplusplus
extern "C" {
#endif

/* The directive owner is the only module field this dispatcher consumes. */
typedef struct DirectiveModule { DWORD reserved[6]; void *owner; } DirectiveModule;
typedef struct IncludeSymbol { StringNode *symbol; struct IncludeSymbol *next; } IncludeSymbol;
typedef struct ByteBuffer { BYTE *data; unsigned used, capacity; } ByteBuffer;

void __stdcall configure_link_options(LinkContext *,DirectiveModule *,const char *);
BYTE __stdcall switch_number(SwitchInfo *,WORD,DWORD *);
BYTE __stdcall valid_section_name(const unsigned char *);
void *__stdcall find_library(const unsigned char *,void *,BYTE);
void __stdcall exclude_default_library(const char *,LinkPointerList *);
void __stdcall add_default_library(const char *,LinkPointerList *);
void __stdcall disallow_library(const char *,LinkPointerList *,void *);
void __stdcall add_delay_library(const char *);
void __stdcall remove_delay_library(const char *);
BYTE __stdcall has_delay_library(const char *);
void __cdecl enable_warning_errors(void);
void __stdcall disable_warning(unsigned);
void __stdcall add_librarian_export(const char *,void *,BYTE,const char *);
void __stdcall record_incremental_include(void *,StringNode *);
void __stdcall copy_repro_file(const char *,BYTE);
void __stdcall add_mac_data(const unsigned char *,int,DWORD);
unsigned __stdcall append_bytes(ByteBuffer *,const void *,unsigned);
void __stdcall set_mac_initializer(LinkContext *,const char *);
void __stdcall set_mac_terminator(LinkContext *,const char *);
BYTE __stdcall add_mac_import(const unsigned char *,const unsigned char *,DirectiveModule *,LinkContext *);
void __stdcall set_mac_import_versions(const char *,const char *,const char *,const char *);
void __cdecl enable_order_file(void);
BYTE __stdcall find_argument(const char *,InputArguments *);
WORD __stdcall parse_machine(LinkContext *,const char *);
void __stdcall record_symbol_reference(StringNode *,void *);
InputArgument *__stdcall append_argument_record(InputArguments *,const char *,const char *);
char *__stdcall canonicalize_argument(const char *,const char *);
void __cdecl linker_usage(void);
void __cdecl link_warning(const char *,unsigned,...);
void __cdecl link_fatal(const char *,unsigned,...);
void __cdecl link_error(const char *,unsigned,...);
#ifdef __cplusplus
}
#endif
#endif
