#include <new.h>
#include <string.h>
#include "frontend-init.h"
#include "preprocessor.h"
#include <recon.h>

/* The real mode-reset override is declared first to supply the native key
 * function. The base declaration order still determines all vtable slots. */
struct CompilerNullBrowser : CompilerBackend {
    void RECON_THISCALL ResetMode(void);
    void RECON_THISCALL Finish(void);
    void RECON_THISCALL Slot04(void);
    void RECON_THISCALL Slot08(void);
    void RECON_THISCALL Slot0c(void);
    void RECON_THISCALL Slot10(void);
    void RECON_THISCALL Slot14(unsigned);
    void RECON_THISCALL Slot18(void);
    void RECON_THISCALL Slot1c(unsigned);
    void RECON_THISCALL Slot20(unsigned);
    void RECON_THISCALL Slot24(unsigned);
    void RECON_THISCALL Slot28(CompilerMacro *);
    void RECON_THISCALL Slot2c(void);
    void RECON_THISCALL Slot30(unsigned);
    void RECON_THISCALL Slot34(void);
    void RECON_THISCALL Slot38(void);
    void RECON_THISCALL Slot3c(unsigned);
    void RECON_THISCALL Slot40(void);
    void RECON_THISCALL Slot44(unsigned);
    void RECON_THISCALL Slot48(void);
    void RECON_THISCALL Slot4c(unsigned);
    void RECON_THISCALL Slot50(unsigned);
    int RECON_THISCALL Slot54(unsigned, unsigned);
    void RECON_THISCALL Slot5c(void);
    RECON_FORCEINLINE CompilerNullBrowser() {}
};

/* The real mode-reset override is declared first to supply the native key
 * function. The base declaration order still determines all vtable slots. */
struct CompilerNamedBrowser : CompilerBackend {
    void RECON_THISCALL ResetMode(void);
    void RECON_THISCALL Finish(void);
    void RECON_THISCALL Slot04(void);
    void RECON_THISCALL Slot08(void);
    void RECON_THISCALL Slot0c(void);
    void RECON_THISCALL Slot10(void);
    void RECON_THISCALL Slot14(unsigned);
    void RECON_THISCALL Slot18(void);
    void RECON_THISCALL Slot1c(unsigned);
    void RECON_THISCALL Slot20(unsigned);
    void RECON_THISCALL Slot24(unsigned);
    void RECON_THISCALL Slot28(CompilerMacro *);
    void RECON_THISCALL Slot2c(void);
    void RECON_THISCALL Slot30(unsigned);
    void RECON_THISCALL Slot34(void);
    void RECON_THISCALL Slot38(void);
    void RECON_THISCALL Slot3c(unsigned);
    void RECON_THISCALL Slot40(void);
    void RECON_THISCALL Slot44(unsigned);
    void RECON_THISCALL Slot48(void);
    void RECON_THISCALL Slot4c(unsigned);
    void RECON_THISCALL Slot50(unsigned);
    int RECON_THISCALL Slot54(unsigned, unsigned);
    void RECON_THISCALL Slot5c(void);
    char *filename;
    /* Original allocation is 12 bytes; its unwritten final DWORD is unmodeled. */
    RECON_FORCEINLINE CompilerNamedBrowser(const char *name) : filename(_strdup(name)) {}
};

extern "C" void __cdecl begin_browser_output(const char *filename)
{
    void *memory;
    if (filename) {
        frontend_data.browser_mode = 3;
        memory = ::operator new(12);
        frontend_data.named_browser = memory ? new(memory) CompilerNamedBrowser(filename) : NULL;
    } else {
        memory = ::operator new(4);
        frontend_data.initial_backend = memory ? new(memory) CompilerNullBrowser : NULL;
    }
}

extern "C" CompilerBackend *__cdecl acquire_initial_backend(void)
{
    if (!frontend_data.initial_backend) {
        void *memory = ::operator new(4);
        frontend_data.initial_backend = memory ? new(memory) CompilerNullBrowser : NULL;
    }
    return frontend_data.initial_backend;
}

void RECON_THISCALL CompilerNullBrowser::Slot20(unsigned value0) { __asm { int 3 } }
void RECON_THISCALL CompilerNullBrowser::Slot28(CompilerMacro *macro)
{
    if (frontend_data.browser_mode == 3)
        macro->identifier->flags |= 0x200000;
}
int RECON_THISCALL CompilerNullBrowser::Slot54(unsigned value0, unsigned value1) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerNullBrowser::ResetMode(void) { __asm { int 3 } }
void RECON_THISCALL CompilerNullBrowser::Slot5c(void) { __asm { int 3 } }
void RECON_THISCALL CompilerNamedBrowser::Finish(void) { __asm { int 3 } }
void RECON_THISCALL CompilerNamedBrowser::Slot04(void) { __asm { int 3 } }
void RECON_THISCALL CompilerNamedBrowser::Slot08(void) { __asm { int 3 } }
void RECON_THISCALL CompilerNamedBrowser::Slot0c(void) { __asm { int 3 } }
void RECON_THISCALL CompilerNamedBrowser::Slot10(void) { __asm { int 3 } }
void RECON_THISCALL CompilerNamedBrowser::Slot14(unsigned value0) { __asm { int 3 } }
void RECON_THISCALL CompilerNamedBrowser::Slot18(void) { __asm { int 3 } }
void RECON_THISCALL CompilerNamedBrowser::Slot1c(unsigned value0) { __asm { int 3 } }
void RECON_THISCALL CompilerNamedBrowser::Slot20(unsigned value0) { __asm { int 3 } }
void RECON_THISCALL CompilerNamedBrowser::Slot24(unsigned value0) { __asm { int 3 } }
void RECON_THISCALL CompilerNamedBrowser::Slot28(CompilerMacro *value0) { __asm { int 3 } }
void RECON_THISCALL CompilerNamedBrowser::Slot2c(void) { __asm { int 3 } }
void RECON_THISCALL CompilerNamedBrowser::Slot30(unsigned value0) { __asm { int 3 } }
void RECON_THISCALL CompilerNamedBrowser::Slot34(void) { __asm { int 3 } }
void RECON_THISCALL CompilerNamedBrowser::Slot38(void) { __asm { int 3 } }
void RECON_THISCALL CompilerNamedBrowser::Slot3c(unsigned value0) { __asm { int 3 } }
void RECON_THISCALL CompilerNamedBrowser::Slot40(void) { __asm { int 3 } }
void RECON_THISCALL CompilerNamedBrowser::Slot44(unsigned value0) { __asm { int 3 } }
void RECON_THISCALL CompilerNamedBrowser::Slot48(void) { __asm { int 3 } }
void RECON_THISCALL CompilerNamedBrowser::Slot4c(unsigned value0) { __asm { int 3 } }
void RECON_THISCALL CompilerNamedBrowser::Slot50(unsigned value0) { __asm { int 3 } }
int RECON_THISCALL CompilerNamedBrowser::Slot54(unsigned value0, unsigned value1) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerNamedBrowser::ResetMode(void) { __asm { int 3 } }
void RECON_THISCALL CompilerNamedBrowser::Slot5c(void) { __asm { int 3 } }
