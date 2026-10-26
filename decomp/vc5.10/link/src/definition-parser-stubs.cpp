#include "definition-parser.h"
#include <recon.h>
extern "C" {
BYTE __stdcall same_librarian_export(StringNode *,const char *,const char *,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD) { __asm { int 3 } return 0; }
WORD __stdcall read_definition_size(const char *,DWORD *,DWORD *) { __asm { int 3 } return 0; }
WORD __stdcall read_definition_sections(const char *,char *) { __asm { int 3 } return 0; }
WORD __stdcall read_definition_stub(char *) { __asm { int 3 } return 0; }
}
