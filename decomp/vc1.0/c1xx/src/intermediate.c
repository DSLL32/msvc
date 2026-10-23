/* Frontend intermediate file creation and output. */
#include <io.h>
#include "initialization.h"
/* Keep the original out-of-line CRT call rather than STDIO.H's macro. */
#undef _fileno
#include <recon.h>

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
int pch_expression_output; /* original 0x9fac8 */
int intermediate_status; /* original 0x8cc7c */
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

/* Original 0x74310..0x74374. Flush while keeping streams available to
   final cleanup. Its historical error path writes the untouched local work
   area before reporting the failure; retain that operation and its size. */
void close_intermediate(FILE *file)
{
    char workspace[512];
    fflush(file);
    if (file->_flag & 0x20) {
        _write(_fileno(file), workspace, sizeof(workspace));
        report_error(85, message_text(339), 0, "..\\..\\Common\\io.c", 241);
    }
}

/* Ownership of partial PCH outputs. Successful writers clear these flags. */
int pch_file_incomplete; /* original 0xa2370 */
int pch_replacement_incomplete; /* original 0xa2374 */
FILE *pch_file; /* original 0x8cfdc */
FILE *pch_replacement_file; /* original 0x8cccc */

/* Original 0x6d040..0x6d138. Remove interrupted PCH output, selecting
   the stream that still owns it and retaining the historical diagnostic site. */
void cleanup_precompiled_files(void)
{
    unsigned line = 0;
    if (!pch_expression_output && !pch_replacement_incomplete && !pch_file_incomplete)
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
    pch_replacement_incomplete = 0;
    pch_file_incomplete = 0;
    pch_expression_output = 0;
}
