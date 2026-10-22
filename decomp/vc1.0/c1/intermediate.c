/* Frontend intermediate file creation and output. */
#include <io.h>
#include "initialization.h"

/* Preserve the original out-of-line CRT entry. */
#undef _fileno

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
unsigned char pch_include_state; /* original 0x6be70 */
unsigned pch_expression_bytes, pch_symbol_bytes; /* original 0x6dde4 / 0x6dddc */
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

/* Original 0x4bb60..0x4bbc5. These streams stay open until CRT exit;
   this checkpoint flushes them and diagnoses an output error. */
void close_intermediate(FILE *file)
{
    char recovery_buffer[512];
    fflush(file);
    if (file->_flag & 0x20) {
        /* The original error path attempts this write from an uninitialized
           automatic buffer before reporting the failed stream. */
        _write(_fileno(file), recovery_buffer, sizeof(recovery_buffer));
        report_error(85, message_text(339), 0, "..\\..\\Common\\io.c", 241);
    }
}

/* PCH streams whose incomplete output must be deleted on compiler exit. */
int pch_file_incomplete; /* original 0x6de30 */
int pch_replacement_incomplete; /* original 0x6de34 */
FILE *pch_file; /* original 0x5b580 */
FILE *pch_replacement_file; /* original 0x5b264 */

/* Original 0x46e30..0x46f29. Completed PCH streams clear these ownership
   flags; an interrupted write closes its active stream and removes /Fp. */
void cleanup_precompiled_files(void)
{
    unsigned line = 0;
    if (!(pch_file_incomplete | pch_replacement_incomplete | pch_expression_output))
        return;
    if (pch_file_incomplete) {
        intermediate_status = fclose(pch_file);
        if (intermediate_status == -1) line = 1445;
    } else if (pch_replacement_incomplete) {
        intermediate_status = fclose(pch_replacement_file);
        if (intermediate_status == -1) line = 1447;
    } else if (pch_expression_output) {
        intermediate_status = fclose(expression_file);
        if (intermediate_status == -1) line = 1449;
    }
    if (line)
        report_error(82, message_text(344), options.option_Fp, pch_diagnostic_name, line);
    if (_unlink(options.option_Fp))
        report_fatal(83, message_text(340), options.option_Fp);
    pch_expression_output = 0;
    pch_replacement_incomplete = 0;
    pch_file_incomplete = 0;
}
