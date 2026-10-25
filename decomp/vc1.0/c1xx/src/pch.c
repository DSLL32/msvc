/* Precompiled-header path selection and saved frontend state. */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "initialization.h"
#include "global-ir.h"
#include "pragma.h"
#include "browser.h"
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

/* Original 0x24840..0x248aa. Filename equality folds case and accepts either
   path separator, but preserves exact termination rather than prefix matching. */
int compare_filenames(char *first, char *second)
{
    while (*first && *second) {
        if (toupper((unsigned char)*first) != toupper((unsigned char)*second) &&
            !((*first == '/' || *first == '\\') &&
              (*second == '/' || *second == '\\')))
            return 0;
        ++first;
        ++second;
    }
    return *first == *second;
}

void read_pch_file(void);

/* Original 0x231c0..0x23308. Consume forced inputs and physical source up to
   /Yu's named include or hdrstop, then restore the actual saved frontend. */
void load_precompiled_header(void)
{
    char *filename;
    token_allocation_flags |= 0x10000;
    final_newline_emitted = 1;
    if (options.pch_use_file) {
        while (forced_source_inputs->count > forced_source_index) {
            current_forced_source = forced_source_inputs->items[forced_source_index++];
            if (compare_filenames(options.pch_use_file, current_forced_source)) {
                prepare_pch_output(options.pch_use_file);
                final_newline_emitted = 0;
                break;
            }
        }
    } else {
        while (forced_source_inputs->count > forced_source_index)
            current_forced_source = forced_source_inputs->items[forced_source_index++];
    }
    while (final_newline_emitted) {
        if ((unsigned char)options.pch_use) {
            while (read_skipped_directive() != PP_PRAGMA) ;
            read_pragma_token();
            if (current_token_code == 2 && classify_pragma(current_token_value.name) == 29) {
                synchronize_pch_pragma();
                prepare_pch_output(options.source_file);
                final_newline_emitted = 0;
            } else skip_source_line();
        } else {
            while (read_skipped_directive() != PP_INCLUDE) ;
            filename = read_include_name();
            if (compare_filenames(options.pch_use_file, filename)) {
                prepare_pch_output(options.pch_use_file);
                final_newline_emitted = 0;
            }
        }
    }
    browser_include_begin();
    read_pch_file();
    browser_include_begin();
    token_allocation_flags &= ~0x10000U;
}
