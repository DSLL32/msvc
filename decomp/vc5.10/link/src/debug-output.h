#ifndef LINK_DEBUG_OUTPUT_H
#define LINK_DEBUG_OUTPUT_H
#include <windows.h>
#include "full-link.h"
#include <recon.h>
typedef struct PdbCodeViewHeader {
    DWORD signature,directory_offset,pdb_signature,pdb_age;
} PdbCodeViewHeader;
typedef char PdbCodeViewHeaderSize[(sizeof(PdbCodeViewHeader)==16)?1:-1];
/* FullLink owns this zero-allocated table, one record per debug module. */
typedef struct DebugModuleRecord {
    OutputModule *module;
    const char *name;
    DWORD public_file_offset,public_bytes,reserved10;
    DWORD symbol_file_offset,symbol_bytes,reserved1c;
    DWORD type_file_offset,type_bytes,precompiled_types;
    DWORD reserved2c,reserved30,reserved34;
    DWORD module_file_offset,module_bytes,reserved40;
} DebugModuleRecord;
typedef char DebugModuleRecordSize[(sizeof(DebugModuleRecord)==68 &&
    offsetof(DebugModuleRecord,public_file_offset)==8 &&
    offsetof(DebugModuleRecord,symbol_file_offset)==0x14 &&
    offsetof(DebugModuleRecord,type_file_offset)==0x20 &&
    offsetof(DebugModuleRecord,module_file_offset)==0x38)?1:-1];
typedef struct RelativeFixupBlock {
    DWORD sites[32];
    struct RelativeFixupBlock *next;
} RelativeFixupBlock;
typedef char RelativeFixupBlockSize[(sizeof(RelativeFixupBlock)==132)?1:-1];
#ifdef __cplusplus
extern "C" {
#endif
extern PdbCodeViewHeader pdb_codeview_header;
extern RelativeFixupBlock *relative_fixup_blocks;
extern DWORD relative_fixup_count;
#ifdef __cplusplus
}
#endif
#define pdb_current_signature pdb_codeview_header.pdb_signature
#define pdb_current_age pdb_codeview_header.pdb_age
#endif
