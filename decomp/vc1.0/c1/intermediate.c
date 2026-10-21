/* Frontend intermediate file creation and output. */
#include "initialization.h"

/* Original 0x4baa0..0x4bb5d. */
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

int pch_expression_output, intermediate_status;
char *pch_diagnostic_name = ""; /* original 0x6be8c */

/* Original 0x462f0..0x46401. Reserve the expression stream's header area. */
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
