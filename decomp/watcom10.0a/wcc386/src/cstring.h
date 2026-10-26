#ifndef WCC_CSTRING_H
#define WCC_CSTRING_H
#include "quads.h"
#include <recon.h>
#pragma pack(1)
typedef struct string_literal {
    SYM_HANDLE next_string,leaf_index;
    unsigned short length;
    unsigned cg_back_handle;
    unsigned char flags;
    char literal[1];
} STRING_LITERAL;
#pragma pack()
extern SYM_HANDLE CStringList,CS_StringList,StringHash[30];
extern int LitCount,LitPoolSize;
void DumpCS_Strings(void);
STRING_LITERAL *GetLiteral(void);
int RemoveEscapes(char *destination);
unsigned JIS2Unicode(unsigned character);
SYM_HANDLE NewLiteralHandle(void);
void ReadLiteralChunk(void *destination,SYM_HANDLE handle);
void WriteLiteralChunk(void *source,SYM_HANDLE handle);
void InitStringLeaf(LEAFPTR leaf,unsigned flags,SYM_HANDLE handle);
void StoreLiteral(STRING_LITERAL *literal,SYM_HANDLE handle);
void GenStringQuad(LEAFPTR leaf);
#endif
