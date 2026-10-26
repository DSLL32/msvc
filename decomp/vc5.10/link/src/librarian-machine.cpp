#include <windows.h>
#include "librarian.h"
#include "options.h"
#include <recon.h>

/* These are output COFF thunk templates, never executed by the linker itself.
 * Each relocation records its byte offset, symbol selector and COFF type. */

#pragma pack(push,1)
struct X86ImportJump { BYTE opcode[2]; DWORD address; };
#pragma pack(pop)
static const X86ImportJump x86_jump={{0xff,0x25},0};
static const ImportThunkRelocation x86_relocations[]={{2,1,6}};
static const DWORD mips_jump[]={0x3c080000,0x8d080000,0x01000008,0};
static const ImportThunkRelocation mips_relocations[]={{0,1,4},{0,0,0x25},{4,1,5}};
static const DWORD alpha_jump[]={0x277f0000,0xa37b0000,0x6bfb0000};
static const ImportThunkRelocation alpha_relocations[]={{0,1,0xa},{0,0,0xc},{4,1,0xb}};
static const DWORD ppc_jump[]={0x3d800000,0x818c0000,0x7d8903a6,0x4e800420};
static const ImportThunkRelocation ppc_relocations[]={{0,1,0x10},{0,0,0x12},{4,1,0x11}};
static const WORD sh_jump[]={0xd001,0x6002,0x402b,0x0009,0,0};
static const ImportThunkRelocation sh_relocations[]={{8,1,2}};
static const DWORD arm_jump[]={0xe59fc000,0xe59cf000,0};
static const ImportThunkRelocation arm_relocations[]={{8,1,1}};
static const ImportThunkDescription x86_thunk={&x86_jump,x86_relocations,6,1,{7,7,7},0xb,0xa,0,0};
static const ImportThunkDescription mips_thunk={mips_jump,mips_relocations,16,3,{0x22,0x22,0x22},0xb,0xa,0,0};
static const ImportThunkDescription alpha_thunk={alpha_jump,alpha_relocations,12,3,{0x10,0x10,0x10},0xf,0xe,0,0};
static const ImportThunkDescription ppc_thunk={ppc_jump,ppc_relocations,16,3,{0xa,0xa,0xa},0xb,0xc,0,0};
static const ImportThunkDescription sh_thunk={sh_jump,sh_relocations,12,1,{0x10,0x10,0x10},0xf,0xe,0,0};
static const ImportThunkDescription arm_thunk={arm_jump,arm_relocations,12,1,{2,2,2},0xf,0xe,0,1};

extern "C" {
extern BYTE underscore_symbols;
BYTE librarian_undecorated_exports;
/* Encountered CodeView object-name and compiler records at467308/467310.
 * Initialize the fields, retaining InitializeMachine's actual BYTE alias. */
LibrarianCodeViewObject librarian_codeview_object={0,9,0};
LibrarianCodeViewCompile librarian_codeview_compile={32,1,0,7,0,8,24,"Microsoft LINK 5.10.7303"};
}

void RECON_THISCALL LibrarianContext::InitializeMachine(DWORD has_inputs)
{
    switch(machine) {
    case 0x14c:
        thunk=&x86_thunk;underscore_symbols=1;librarian_codeview_compile.machine=3;break;
    case 0x166:
        thunk=&mips_thunk;librarian_codeview_compile.machine=0x10;break;
    case 0x184:
        thunk=&alpha_thunk;librarian_codeview_compile.machine=0x30;break;
    case 0x1a2:
        thunk=&sh_thunk;underscore_symbols=1;librarian_codeview_compile.machine=0x50;break;
    case 0x1a6:
        thunk=&sh_thunk;underscore_symbols=1;librarian_codeview_compile.machine=0x51;break;
    case 0x1c0:
        thunk=&arm_thunk;librarian_codeview_compile.machine=0x61;break;
    case 0x601:
        thunk=NULL;underscore_symbols=1;librarian_codeview_compile.machine=0x40;break;
    case 0x1f0:
        thunk=&ppc_thunk;librarian_codeview_compile.machine=0x40;break;
    default:link_fatal(NULL,1115);break;
    }
    /* The complete original body consumes only AL from its DWORD formal. */
    if(underscore_symbols && !(BYTE)has_inputs)librarian_undecorated_exports=1;
}
