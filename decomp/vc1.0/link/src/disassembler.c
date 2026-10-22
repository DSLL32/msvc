/* Dump disassembler callbacks, shared by the pending decoder helpers. */
#include "dump.h"
#include <recon.h>

FatalRoutine disassembler_fatal;
WarningRoutine disassembler_warning;

/* Original [0004ae00,0004ae13), cdecl; no internal code data or callees. */
void initialize_disassembler(FatalRoutine fatal,WarningRoutine warning)
{
    disassembler_fatal=fatal;
    disassembler_warning=warning;
}
