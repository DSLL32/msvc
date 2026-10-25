#ifndef C1XX_PREPROCESSOR_H
#define C1XX_PREPROCESSOR_H
#include "names.h"
#include <recon.h>

/* Only the reached kind and text fields are interpreted. The leading four
 * bytes retain their real storage and remain semantically unmodeled. */
struct CompilerMacroBody {
    BYTE unmodeled_prefix[4];
    signed char kind;
    char payload[1];
};
struct CompilerMacro {
    CompilerNameEntry *identifier;
    CompilerMacroBody *body;
};

#endif
