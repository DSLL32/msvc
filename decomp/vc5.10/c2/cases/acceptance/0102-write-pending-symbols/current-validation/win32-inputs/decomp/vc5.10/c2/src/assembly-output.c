#include "functions.h"
#include <recon.h>

extern int c2_assembly_enabled, c2_mixed_assembly;
int assembly_output_active;
unsigned char assembly_comment_character;

void __fastcall initialize_source_output(char *source)
{
    assembly_output_active = c2_assembly_enabled || c2_mixed_assembly ? 1 : 0;
    if (assembly_output_active) {
        assembly_comment_character = get_assembly_comment_character();
        write_assembly_header(source);
    }
}
