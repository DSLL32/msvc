/* Frontend intermediate file creation and output. */
#include <errno.h>
#include <io.h>
#include <stdlib.h>
#include "global-ir.h"
#include "browser.h"
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

static char *pch_transfer_buffer; /* original0x8ccc8, shared stream-copy workspace */
unsigned __stdcall copy_pch_stream(FILE *source, FILE *destination, int bytes);
void stage_initial_pch_streams(void);

/* Original0x6c5f0..0x6cc49. Arrange an updateable PCH stream and append the
   saved IR prefixes, preserving the existing expression output for resumption. */
void stage_pch_intermediates(void)
{
    char filename[64];
    unsigned i;
    if (!pch_read_enabled) {
        if (pch_expression_output) {
            pch_file = expression_file;
            expression_file = open_intermediate(options.intermediate_prefix, "ex", "w");
            pch_expression_output = 0;
            pch_file_incomplete = 1;
        } else {
            for (i = 0; options.intermediate_prefix[i]; ++i)
                filename[i] = options.intermediate_prefix[i];
            filename[i] = 'e';
            filename[i + 1] = 'x';
            filename[i + 2] = 0;
            intermediate_status = fclose(expression_file);
            if (intermediate_status == -1)
                report_error(82, message_text(339), 0, pch_diagnostic_name, 1094);
            if (remove(options.option_Fp) && errno == EACCES)
                report_fatal(83, message_text(344), options.option_Fp);
            if (rename(filename, options.option_Fp)) {
                if (errno == ENOENT || errno == EACCES)
                    report_fatal(83, message_text(344), options.option_Fp);
                else if (errno == EXDEV) {
                    pch_file = fopen(options.option_Fp, "wb");
                    if (!pch_file) report_fatal(83, message_text(344), options.option_Fp);
                    pch_file_incomplete = 1;
                    expression_file = open_intermediate(options.intermediate_prefix, "ex", "r+b");
                    pch_transfer_buffer = malloc(0x8000);
                    copy_pch_stream(expression_file, pch_file, 0x7fffffff);
                    free(pch_transfer_buffer);
                }
            } else {
                pch_file = fopen(options.option_Fp, "r+b");
                if (!pch_file) report_fatal(83, message_text(344), options.option_Fp);
                pch_file_incomplete = 1;
                expression_file = open_intermediate(options.intermediate_prefix, "ex", "wb");
            }
        }
        intermediate_status = fflush(pch_file);
        if (intermediate_status == -1)
            report_error(88, message_text(344), options.option_Fp, pch_diagnostic_name, 1150);
        intermediate_status = fseek(pch_file, 0, 2);
        if (intermediate_status)
            report_error(86, message_text(344), options.option_Fp, pch_diagnostic_name, 1151);
        intermediate_status = ftell(pch_file);
        if (intermediate_status == -1)
            report_error(87, message_text(344), options.option_Fp, pch_diagnostic_name, 1156);
        hidden_frame_bytes = intermediate_status;
        pch_expression_bytes = hidden_frame_bytes - 0x334;
        stage_initial_pch_streams();
    } else {
        pch_replacement_file = fopen(options.option_Fp, "w+b");
        if (!pch_replacement_file) report_fatal(83, message_text(344), options.option_Fp);
        pch_replacement_incomplete = 1;
        rewind(pch_file);
        intermediate_status = fflush(expression_file);
        if (intermediate_status == -1)
            report_error(88, message_text(339), 0, pch_diagnostic_name, 1181);
        rewind(expression_file);
        pch_transfer_buffer = malloc(0x8000);
        copy_pch_stream(pch_file, pch_replacement_file, pch_expression_bytes + 0x334);
        pch_expression_bytes += copy_pch_stream(expression_file, pch_replacement_file, 0x7fffffff);
        hidden_frame_bytes = pch_expression_bytes + 0x334;
        rewind(symbol_file);
        copy_pch_stream(pch_file, pch_replacement_file, pch_symbol_bytes);
        pch_symbol_bytes += copy_pch_stream(symbol_file, pch_replacement_file, 0x7fffffff);
        rewind(global_file);
        pch_header.global_bytes = copy_pch_stream(global_file, pch_replacement_file, 0x7fffffff);
        rewind(line_file);
        pch_header.line_bytes = copy_pch_stream(line_file, pch_replacement_file, 0x7fffffff);
        if (options.configuration.debug_info && options.configuration.option_Yd) {
            rewind(debug_file);
            pch_header.debug_bytes = copy_pch_stream(debug_file, pch_replacement_file, 0x7fffffff);
        }
        intermediate_status = fclose(pch_file);
        if (intermediate_status == -1)
            report_error(82, message_text(339), 0, pch_diagnostic_name, 1208);
        pch_file = pch_replacement_file;
        pch_file_incomplete = 1;
        pch_replacement_incomplete = 0;
    }
    intermediate_status = fflush(symbol_file);
    if (intermediate_status == -1)
        report_error(88, message_text(339), 0, pch_diagnostic_name, 1216);
    rewind(expression_file);
    rewind(symbol_file);
    intermediate_status = _chsize(expression_file->_file, 0);
    if (intermediate_status == -1)
        report_error(89, message_text(339), 0, pch_diagnostic_name, 1224);
    intermediate_status = _chsize(symbol_file->_file, 0);
    if (intermediate_status == -1)
        report_error(89, message_text(339), 0, pch_diagnostic_name, 1225);
}

extern unsigned char browser_symbol_references[8192];

/* Original0x6d200..0x6d465. Append the initial saved symbol/global/line
   streams and optional debug/browser state to the expression-prefix PCH. */
void stage_initial_pch_streams(void)
{
    pch_transfer_buffer = malloc(0x8000);
    intermediate_status = fflush(symbol_file);
    if (intermediate_status == -1)
        report_error(88, message_text(339), 0, pch_diagnostic_name, 1502);
    rewind(symbol_file);
    pch_header.symbol_bytes = copy_pch_stream(symbol_file, pch_file, 0x7fffffff);
    intermediate_status = fflush(global_file);
    if (intermediate_status == -1)
        report_error(88, message_text(339), 0, pch_diagnostic_name, 1503);
    rewind(global_file);
    pch_header.global_bytes = copy_pch_stream(global_file, pch_file, 0x7fffffff);
    intermediate_status = fflush(line_file);
    if (intermediate_status == -1)
        report_error(88, message_text(339), 0, pch_diagnostic_name, 1505);
    rewind(line_file);
    pch_header.line_bytes = copy_pch_stream(line_file, pch_file, 0x7fffffff);
    if (options.configuration.debug_info && options.configuration.option_Yd) {
        intermediate_status = fflush(debug_file);
        if (intermediate_status == -1)
            report_error(88, message_text(339), 0, pch_diagnostic_name, 1512);
        rewind(debug_file);
        pch_header.debug_bytes = copy_pch_stream(debug_file, pch_file, 0x7fffffff);
    }
    if (browser_filename && pch_mode == 1) {
        intermediate_status = fflush(browser_file);
        if (intermediate_status == -1)
            report_error(88, message_text(339), 0, pch_diagnostic_name, 1515);
        rewind(browser_file);
        pch_header.browser_bytes = copy_pch_stream(browser_file, pch_file, 0x7fffffff);
        fwrite(browser_symbol_references, 1, 0x1fff, pch_file);
    }
    free(pch_transfer_buffer);
}

/* Original0x6d140..0x6d1fe. Copy through the shared32K workspace. A short
   read ends the transfer, retaining the original signed limit/low-WORD request. */
unsigned __stdcall copy_pch_stream(FILE *source, FILE *destination, int bytes)
{
    int left = bytes;
    unsigned count, request;
    do {
        request = left < 0x8000 ? (unsigned short)left : 0x8000;
        count = fread(pch_transfer_buffer, 1, request, source);
        left -= count;
        if (source->_flag & 0x20) report_fatal(84, message_text(339), 0);
        intermediate_status = fwrite(pch_transfer_buffer, 1, count, destination);
        if (intermediate_status == -1)
            report_error(85, message_text(339), 0, pch_diagnostic_name, 1485);
    } while (count == 0x8000);
    return (unsigned)bytes - left;
}
