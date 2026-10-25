/* Precompiled-header path selection and saved frontend state. */
#include <stdlib.h>
#include <string.h>
#include "initialization.h"
#include "global-ir.h"
#include <recon.h>

/* Original 0x23310..0x233dd. The default name retains the source drive and
   directory, replaces the extension, and becomes the current pragma name.
   The explicit /Fp path and pragma_text share their original pointer owners. */
void __stdcall prepare_pch_output(char *filename)
{
    char drive[4], directory[256], basename[256];
    unsigned length, part;
    if (!options.option_Fp) {
        _splitpath(filename, drive, directory, basename, 0);
        for (length = 0; drive[length]; ++length) ;
        for (part = 0; directory[part]; ++part) ;
        length += part;
        for (part = 0; basename[part]; ++part) ;
        length += part + 5;
        options.option_Fp = malloc(length);
        if (!options.option_Fp) fatal_error(60);
        _makepath(options.option_Fp, drive, directory, basename, ".pch");
    }
    pragma_text = options.option_Fp;
}
