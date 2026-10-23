#include "c2.h"
#include "warnings.h"
#include "options.h"
#include <stdarg.h>
#include <recon.h>
int warning_override_count;
/* Original [000592f0,00059350). A null pointer leaves prior state intact. */
void __fastcall apply_warning_state(const SavedWarning *state)
{
    if (state) {
        warning_override_count = 0;
        while (state->number != -1) {
            set_warning_override(state->number, state->level);
            ++state;
        }
    }
}

/* Original7ea70..7eb38:50 native four-byte warning overrides, distinct
   from the packed three-byte SavedWarning transport records. The setter's
   original70637/7063b count check establishes this contextual capacity. */
typedef struct WarningOverride {
    short number;
    signed char level;
    unsigned char reserved;
} WarningOverride;
typedef char WarningOverrideSizeCheck[sizeof(WarningOverride) == 4 ? 1 : -1];
static WarningOverride warning_overrides[50];

int __fastcall warning_override_level(int number);
void __fastcall change_warning_override(short number, signed char level);
char *__fastcall diagnostic_format(unsigned number);
void __cdecl emit_diagnostic(unsigned category, unsigned number, char *format, va_list arguments);
extern int warning_level, warnings_as_errors;

/* Original [00070490,000705a4), including15 dispatch DWORDs70568..705a4.
   Per-warning state can select a level, suppress subsequent occurrences,
   or promote this diagnostic to an error. Filter before retrieving text
   or consuming the variadic arguments, as the original does. */
void __cdecl compiler_warning(int level, int number, ...)
{
    int override = warning_override_level(number);
    char *format;
    va_list arguments;
    switch (override) {
    case 0: break;
    case 1: case 2: case 3: case 4:
        level = override;
        break;
    case 5:
        change_warning_override((short)number, 13);
        break;
    case 13:
        return;
    case 14:
        if (level <= warning_level) {
            format = diagnostic_format(number + 4000);
            va_start(arguments, number);
            emit_diagnostic(2000, number + 2000, format, arguments);
            va_end(arguments);
            if (!compiler_errors) compiler_errors = 1;
        }
        return;
    default:
        compiler_internal_error("@(#)error.c:1.63", 0x76);
        return;
    }
    if (level > warning_level) return;
    format = diagnostic_format(number + 4000);
    va_start(arguments, number);
    emit_diagnostic(4000, number, format, arguments);
    va_end(arguments);
    if (warnings_as_errors && !compiler_errors) compiler_error(0xdc);
}

/* Original [000705e0,00070612). Preserve first-match order and signed
   short/signed byte extension. An empty override list uses the call's level. */
int __fastcall warning_override_level(int number)
{
    int index;
    for (index = 0; index < warning_override_count; ++index)
        if (warning_overrides[index].number == number)
            return warning_overrides[index].level;
    return 0;
}
