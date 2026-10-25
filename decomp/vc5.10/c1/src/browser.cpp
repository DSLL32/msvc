#include <new.h>
#include <string.h>
#include "frontend.h"
#include "keywords.h"
#include <recon.h>

/* The first declared override has its own target body and supplies the ELF
 * key function; the inherited base declaration order owns the vtable slots. */
struct CompilerNullBrowser : CompilerBackend {
    void RECON_THISCALL ResetMode(void);
    void RECON_THISCALL Finish(void);
    void RECON_THISCALL Slot04(void);
    void RECON_THISCALL Slot08(void);
    void RECON_THISCALL Slot0c(void);
    void RECON_THISCALL Slot10(void);
    void RECON_THISCALL Slot14(unsigned value);
    void RECON_THISCALL Slot18(void);
    void RECON_THISCALL Slot1c(void *value);
    void RECON_THISCALL Slot20(void *value);
    void RECON_THISCALL Slot24(void *value);
    void RECON_THISCALL Slot28(void *value);
    void RECON_THISCALL Slot2c(void);
    void RECON_THISCALL Slot30(void *value);
    void RECON_THISCALL Slot34(void);
    void RECON_THISCALL Slot38(void);
    void RECON_THISCALL Slot3c(void *value);
    void RECON_THISCALL Slot40(void);
    int RECON_THISCALL Disable(void);
    void RECON_THISCALL CheckLocation(void);
    unsigned RECON_THISCALL SymbolId(const void *value);
    void RECON_THISCALL SetSymbolId(void *object, unsigned value);
    void RECON_THISCALL Slot58(void);
    void RECON_THISCALL Slot5c(void);
    RECON_FORCEINLINE CompilerNullBrowser() {}
};

struct CompilerNamedBrowser : CompilerBackend {
    void RECON_THISCALL ResetMode(void);
    void RECON_THISCALL Finish(void);
    void RECON_THISCALL Slot04(void);
    void RECON_THISCALL Slot08(void);
    void RECON_THISCALL Slot0c(void);
    void RECON_THISCALL Slot10(void);
    void RECON_THISCALL Slot14(unsigned value);
    void RECON_THISCALL Slot18(void);
    void RECON_THISCALL Slot1c(void *value);
    void RECON_THISCALL Slot20(void *value);
    void RECON_THISCALL Slot24(void *value);
    void RECON_THISCALL Slot28(void *value);
    void RECON_THISCALL Slot2c(void);
    void RECON_THISCALL Slot30(void *value);
    void RECON_THISCALL Slot34(void);
    void RECON_THISCALL Slot38(void);
    void RECON_THISCALL Slot3c(void *value);
    void RECON_THISCALL Slot40(void);
    int RECON_THISCALL Disable(void);
    void RECON_THISCALL CheckLocation(void);
    unsigned RECON_THISCALL SymbolId(const void *value);
    void RECON_THISCALL SetSymbolId(void *object, unsigned value);
    void RECON_THISCALL Slot58(void);
    void RECON_THISCALL Slot5c(void);
    char *filename;
    /* Original allocation is 12 bytes; its unwritten last four bytes remain
     * UNMODELED outside this known prefix. */
    RECON_FORCEINLINE CompilerNamedBrowser(const char *name) : filename(_strdup(name)) {}
};

typedef char NullBrowser_size[sizeof(CompilerNullBrowser) == 4 ? 1 : -1];
typedef char NamedBrowser_prefix[sizeof(CompilerNamedBrowser) == 8 ? 1 : -1];

extern "C" void __cdecl begin_browser_output(const char *filename)
{
    void *memory;
    if (filename) {
        c1_data.browser_mode = 3;
        memory = ::operator new(12);
        c1_data.named_browser = memory ? new(memory) CompilerNamedBrowser(filename) : NULL;
    } else {
        memory = ::operator new(4);
        c1_data.initial_backend = memory ? new(memory) CompilerNullBrowser : NULL;
    }
}

extern "C" CompilerBackend *__cdecl acquire_initial_backend(void)
{
    if (!c1_data.initial_backend) {
        void *memory = ::operator new(4);
        c1_data.initial_backend = memory ? new(memory) CompilerNullBrowser : NULL;
    }
    return c1_data.initial_backend;
}

void RECON_THISCALL CompilerNullBrowser::Slot20(void *value) { __asm { int 3 }  }
void RECON_THISCALL CompilerNullBrowser::Slot28(void *value)
{
    if (c1_data.browser_mode == 3)
        ((CompilerMacro *)value)->identifier->flags |= 0x200000;
}
void RECON_THISCALL CompilerNullBrowser::ResetMode(void) { __asm { int 3 }  }
int RECON_THISCALL CompilerNullBrowser::Disable(void) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerNullBrowser::CheckLocation(void) { __asm { int 3 }  }
unsigned RECON_THISCALL CompilerNullBrowser::SymbolId(const void *value) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerNullBrowser::SetSymbolId(void *object, unsigned value) { __asm { int 3 }  }
void RECON_THISCALL CompilerNullBrowser::Slot5c(void) { __asm { int 3 }  }
void RECON_THISCALL CompilerNamedBrowser::Finish(void) { __asm { int 3 }  }
void RECON_THISCALL CompilerNamedBrowser::Slot04(void) { __asm { int 3 }  }
void RECON_THISCALL CompilerNamedBrowser::Slot08(void) { __asm { int 3 }  }
void RECON_THISCALL CompilerNamedBrowser::Slot0c(void) { __asm { int 3 }  }
void RECON_THISCALL CompilerNamedBrowser::Slot10(void) { __asm { int 3 }  }
void RECON_THISCALL CompilerNamedBrowser::Slot14(unsigned value) { __asm { int 3 }  }
void RECON_THISCALL CompilerNamedBrowser::Slot18(void) { __asm { int 3 }  }
void RECON_THISCALL CompilerNamedBrowser::Slot1c(void *value) { __asm { int 3 }  }
void RECON_THISCALL CompilerNamedBrowser::Slot20(void *value) { __asm { int 3 }  }
void RECON_THISCALL CompilerNamedBrowser::Slot24(void *value) { __asm { int 3 }  }
void RECON_THISCALL CompilerNamedBrowser::Slot28(void *value) { __asm { int 3 }  }
void RECON_THISCALL CompilerNamedBrowser::Slot2c(void) { __asm { int 3 }  }
void RECON_THISCALL CompilerNamedBrowser::Slot30(void *value) { __asm { int 3 }  }
void RECON_THISCALL CompilerNamedBrowser::Slot34(void) { __asm { int 3 }  }
void RECON_THISCALL CompilerNamedBrowser::Slot38(void) { __asm { int 3 }  }
void RECON_THISCALL CompilerNamedBrowser::Slot3c(void *value) { __asm { int 3 }  }
void RECON_THISCALL CompilerNamedBrowser::Slot40(void) { __asm { int 3 }  }
void RECON_THISCALL CompilerNamedBrowser::ResetMode(void) { __asm { int 3 }  }
int RECON_THISCALL CompilerNamedBrowser::Disable(void) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerNamedBrowser::CheckLocation(void) { __asm { int 3 }  }
unsigned RECON_THISCALL CompilerNamedBrowser::SymbolId(const void *value) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerNamedBrowser::SetSymbolId(void *object, unsigned value) { __asm { int 3 }  }
void RECON_THISCALL CompilerNamedBrowser::Slot58(void) { __asm { int 3 }  }
void RECON_THISCALL CompilerNamedBrowser::Slot5c(void) { __asm { int 3 }  }
