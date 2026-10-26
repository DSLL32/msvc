#ifndef LINK_DEBUG_OUTPUT_H
#define LINK_DEBUG_OUTPUT_H
#include <windows.h>
#include <recon.h>
typedef struct PdbCodeViewHeader {
    DWORD signature,directory_offset,pdb_signature,pdb_age;
} PdbCodeViewHeader;
typedef char PdbCodeViewHeaderSize[(sizeof(PdbCodeViewHeader)==16)?1:-1];
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
