#ifndef TYPE_DIAGNOSTICS_INTERNAL_H
#define TYPE_DIAGNOSTICS_INTERNAL_H
#include "type-diagnostics.h"
#include <recon.h>
TypeDiagnosticSlot *__stdcall find_type_diagnostic_slot(short text);
char *__stdcall reserve_type_diagnostic_bytes(short text, int append, unsigned bytes);
#endif
