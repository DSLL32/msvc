#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "coff-output.h"
#include "coff-writer.h"
#include "output.h"
#include <recon.h>
#undef printf
#undef strcpy
int codeview_debug, line_debug;
static char filename[] = "dbglist.c", events[128];
static unsigned used;
void __fastcall coff_line_marker(unsigned short marker)
{
    char text[32]; sprintf(text, "L%u;", (unsigned)marker);
    strcpy(events + used, text); used += strlen(text);
}
void __fastcall coff_record_filename(char *value)
{
    if (value != filename) abort(); strcpy(events + used, "F;"); used += 2;
}
int main(void)
{
    int states[] = {-1, 0, 1};
    unsigned markers[] = {0, 1, 65535};
    unsigned emitted, codeview, line, marker;
    printf("initial %d\n", coff_emitted_function);
    for (emitted = 0; emitted < 3; ++emitted)
        for (codeview = 0; codeview < 3; ++codeview)
            for (line = 0; line < 3; ++line)
                for (marker = 0; marker < 3; ++marker) {
                    coff_emitted_function = states[emitted]; codeview_debug = states[codeview];
                    line_debug = states[line]; coff_has_function_lines = markers[marker];
                    used = 0; events[0] = 0; coff_output_debug_pair(filename);
                    printf("%d %d %d %u %u %s\n", states[emitted], states[codeview], states[line], markers[marker], (unsigned)coff_has_function_lines, events);
                }
    return 0;
}
