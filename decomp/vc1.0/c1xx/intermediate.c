/* Frontend intermediate file creation and output. */
#include "initialization.h"

/* Original 0x3d310..0x3d311 is an empty target-specific hook. */
void initialize_intermediate_output(void)
{
}

/* Original 0x74250..0x7430d. */
FILE *open_intermediate(char *prefix, char *extension, char *mode)
{
    char filename[260];
    char *output = filename;
    FILE *file;
    while ((*output++ = *prefix++) != 0)
        ;
    if (extension) {
        --output;
        while ((*output++ = *extension++) != 0)
            ;
    }
    file = fopen(filename, mode);
    if (!file)
        report_fatal(83, message_text(339), filename);
    setvbuf(file, 0, 0, 4096);
    return file;
}

unsigned pch_symbol_bytes; /* original 0xa231c */
unsigned pch_expression_bytes; /* original 0xa2324 */
int pch_expression_output, intermediate_status;
char *pch_diagnostic_name = ""; /* original 0x9fad4 */

/* Original 0x6c4d0..0x6c5e1. Reserve the expression stream's header area. */
void open_expression_intermediate(void)
{
    if (pch_write_enabled && (options.pch_create_file || options.option_Fp) &&
        !pch_read_enabled && !options.pch_auto) {
        prepare_pch_output(options.pch_create_file);
        expression_file = open_intermediate(options.option_Fp, "", "w");
        pch_expression_output = 1;
    } else
        expression_file = open_intermediate(options.intermediate_prefix, "ex", "w+");
    write_ir_record(0x5b, make_integer_constant(0x334));
    intermediate_status = fseek(expression_file, 0x333, 0);
    if (intermediate_status)
        report_error(86, message_text(339), 0, pch_diagnostic_name, 1058);
    intermediate_status = fputc(0, expression_file);
    if (intermediate_status)
        report_error(86, message_text(339), 0, pch_diagnostic_name, 1059);
}
