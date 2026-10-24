#include "c2.h"
#include <recon.h>

FILE *c2_current_il;
int c2_il_extended_header, c2_il_source_table;
int c2_il_symbol_count, c2_il_initial_symbol_count;
int c2_il_target_size, c2_il_eight_byte_target;
extern char *c2_pdb_name, *c2_output_name;

FILE *__fastcall c2_open_intermediate(char *base, char *suffix, char *mode,
                                     char *description)
{
    char filename[260];
    FILE *file;

    strcpy(filename, base);
    strcat(filename, suffix);
    if (strlen(filename) > sizeof(filename))
        c2_report_internal("E:\\utc\\src\\\\Common\\ioin.c", 326);
    file = fopen(filename, mode);
    if (!file)
        c2_system_fatal(83, c2_get_diagnostic_string(311), filename);
    return file;
}

void c2_read_header(void)
{
    unsigned short machine;
    long version;

    c2_current_il = c2_globals_input;
    if ((unsigned char)c2_read_il_byte() != 0x11)
        c2_fatal(900, "P1", 0, "P2", 19940811L);
    machine = c2_read_il_word();
    if (machine != 0x2386 && machine != 0x0386)
        c2_report_internal("E:\\utc\\src\\\\P2\\p2symtab.c", 923);
    version = c2_read_il_long();
    if (version == 19940811L) {
        c2_il_extended_header = 1;
        c2_il_source_table = 0;
    } else if (version == 19931217L || version == 19960701L) {
        c2_il_extended_header = 0;
        c2_il_source_table = 0;
    } else if (version == 19960416L || version == 19960702L) {
        c2_il_extended_header = 0;
        c2_il_source_table = 1;
    } else if (version == 19960417L || version == 19960703L ||
               version == 19960704L) {
        c2_il_extended_header = 1;
        c2_il_source_table = 1;
    } else {
        c2_fatal(900, "P1", version, "P2", 19940811L);
    }
    c2_il_symbol_count = c2_read_il_long();
    c2_il_initial_symbol_count = c2_il_symbol_count;
    c2_il_target_size = (signed char)c2_read_il_byte();
    c2_il_eight_byte_target = c2_il_target_size == 8;
    if (c2_il_extended_header && !c2_open_pdb(c2_pdb_name, c2_output_name))
        c2_report_internal("E:\\utc\\src\\\\P2\\p2symtab.c", 977);
}
