#ifndef RC_VERSION_H
#define RC_VERSION_H
#include "tokens.h"
#include "resource-data.h"
#include <recon.h>
struct VersionHeader { WORD length, value_length, type; };
struct VersionHeader * __cdecl parse_fixed_version(void);
void __stdcall parse_version_number(DWORD *);
void __stdcall parse_version_scalar(DWORD *);
WORD __cdecl parse_version_children(void);
struct VersionHeader * __stdcall put_version_block(char *, void *, WORD);
struct VersionHeader * __stdcall parse_version_value(char *);
WORD __stdcall parse_version_data(WORD *);
#endif
