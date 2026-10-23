/* Stream ownership and copying at precompiled-header boundaries. */
#include <stdlib.h>
#include <errno.h>
#include <io.h>
#include "pch.h"
#include "source-input.h"
#include "browser.h"
#include <recon.h>

int pch_replace_expression; /* original 0x6be78 */
/* The browser owns8192 bits-bytes for all sixteen-bit symbol IDs. The
   original PCH format transfers only8191 bytes; retain that observed length. */
void *pch_copy_buffer; /* original 0x5b570 */

/* These diagnostic expansions are inline in the original functions. */
#define PCH_IO_STATUS(operation_, code_, message_, file_, line_) do { \
    intermediate_status = (operation_); \
    if (intermediate_status == -1) \
        report_error(code_, message_text(message_), file_, pch_diagnostic_name, line_); \
} while (0)
#define PCH_IO_SEEK(operation_, message_, file_, line_) do { \
    intermediate_status = (operation_); \
    if (intermediate_status) \
        report_error(86, message_text(message_), file_, pch_diagnostic_name, line_); \
} while (0)

/* Original 0x46410..0x46a69. Move the expression prefix into /Fp, append
   the other streams, then truncate the active expression/symbol work files. */
void stage_pch_intermediates(void)
{
    char expression_name[64];
    unsigned length;
    int result;
    if (!pch_replace_expression) {
        if (!pch_expression_output) {
            for (length = 0; (expression_name[length] = options.intermediate_prefix[length]) != 0; ++length) ;
            expression_name[length++] = 'e';
            expression_name[length++] = 'x';
            expression_name[length] = 0;
            PCH_IO_STATUS(fclose(expression_file), 82, 339, 0, 1094);
            result = remove(options.option_Fp);
            if (result && errno == EACCES)
                report_fatal(83, message_text(344), options.option_Fp);
            if (!rename(expression_name, options.option_Fp)) {
                pch_file = fopen(options.option_Fp, "r+b");
                if (!pch_file) report_fatal(83, message_text(344), options.option_Fp);
                pch_file_incomplete = 1;
                expression_file = open_intermediate(options.intermediate_prefix, "ex", "wb");
            } else if (errno == ENOENT || errno == EACCES)
                report_fatal(83, message_text(344), options.option_Fp);
            else if (errno == EXDEV) {
                pch_file = fopen(options.option_Fp, "wb");
                if (!pch_file) report_fatal(83, message_text(344), options.option_Fp);
                pch_file_incomplete = 1;
                expression_file = open_intermediate(options.intermediate_prefix, "ex", "r+b");
                pch_copy_buffer = malloc(0x8000);
                copy_pch_stream(expression_file, pch_file, 0x7fffffff);
                free(pch_copy_buffer);
            }
        } else {
            pch_file = expression_file;
            expression_file = open_intermediate(options.intermediate_prefix, "ex", "w");
            pch_expression_output = 0;
            pch_file_incomplete = 1;
        }
        PCH_IO_STATUS(fflush(pch_file), 88, 344, options.option_Fp, 1150);
        PCH_IO_SEEK(fseek(pch_file, 0, 2), 344, options.option_Fp, 1151);
        PCH_IO_STATUS(ftell(pch_file), 87, 344, options.option_Fp, 1156);
        pch_expression_bytes = intermediate_status - 0x334;
        pch_symbol_offset_adjustment = intermediate_status;
        append_pch_intermediates();
    } else {
        pch_replacement_file = fopen(options.option_Fp, "w+b");
        if (!pch_replacement_file) report_fatal(83, message_text(344), options.option_Fp);
        pch_replacement_incomplete = 1;
        rewind(pch_file);
        PCH_IO_STATUS(fflush(expression_file), 88, 339, 0, 1181);
        rewind(expression_file);
        pch_copy_buffer = malloc(0x8000);
        copy_pch_stream(pch_file, pch_replacement_file, pch_expression_bytes + 0x334);
        pch_expression_bytes += copy_pch_stream(expression_file, pch_replacement_file, 0x7fffffff);
        pch_symbol_offset_adjustment = pch_expression_bytes + 0x334;
        rewind(symbol_file);
        copy_pch_stream(pch_file, pch_replacement_file, pch_symbol_bytes);
        pch_symbol_bytes += copy_pch_stream(symbol_file, pch_replacement_file, 0x7fffffff);
        rewind(global_file);
        pch_global_bytes = copy_pch_stream(global_file, pch_replacement_file, 0x7fffffff);
        rewind(line_file);
        pch_line_bytes = copy_pch_stream(line_file, pch_replacement_file, 0x7fffffff);
        if (options.debug_info && options.option_Yd) {
            rewind(debug_file);
            pch_debug_bytes = copy_pch_stream(debug_file, pch_replacement_file, 0x7fffffff);
        }
        PCH_IO_STATUS(fclose(pch_file), 82, 339, 0, 1208);
        pch_file = pch_replacement_file;
        pch_file_incomplete = 1;
        pch_replacement_incomplete = 0;
    }
    PCH_IO_STATUS(fflush(symbol_file), 88, 339, 0, 1216);
    rewind(expression_file);
    rewind(symbol_file);
    PCH_IO_STATUS(_chsize(expression_file->_file, 0), 89, 339, 0, 1224);
    PCH_IO_STATUS(_chsize(symbol_file->_file, 0), 89, 339, 0, 1225);
}

/* Original 0x46ff0..0x47255. Flush each current stream, append its bytes
   to the PCH, and remember the individual extents for later extraction. */
void append_pch_intermediates(void)
{
    pch_copy_buffer = malloc(0x8000);
    PCH_IO_STATUS(fflush(symbol_file), 88, 339, 0, 1502);
    rewind(symbol_file);
    pch_symbol_bytes = copy_pch_stream(symbol_file, pch_file, 0x7fffffff);
    PCH_IO_STATUS(fflush(global_file), 88, 339, 0, 1503);
    rewind(global_file);
    pch_global_bytes = copy_pch_stream(global_file, pch_file, 0x7fffffff);
    PCH_IO_STATUS(fflush(line_file), 88, 339, 0, 1505);
    rewind(line_file);
    pch_line_bytes = copy_pch_stream(line_file, pch_file, 0x7fffffff);
    if (options.debug_info && options.option_Yd) {
        PCH_IO_STATUS(fflush(debug_file), 88, 339, 0, 1512);
        rewind(debug_file);
        pch_debug_bytes = copy_pch_stream(debug_file, pch_file, 0x7fffffff);
    }
    if (browser_filename && pch_mode == 1) {
        PCH_IO_STATUS(fflush(browser_output), 88, 339, 0, 1515);
        rewind(browser_output);
        pch_browser_bytes = copy_pch_stream(browser_output, pch_file, 0x7fffffff);
        fwrite(browser_symbol_bits, 1, (sizeof(browser_symbol_bits) - 1), pch_file);
    }
    free(pch_copy_buffer);
}

/* Original 0x46f30..0x46fee. Copy in at most 32K chunks. The native
   routine stops on a short read, including a zero-byte read at an exact limit. */
unsigned __stdcall copy_pch_stream(FILE *input, FILE *output, unsigned bytes)
{
    unsigned remaining = bytes;
    unsigned count;
    do {
        count = 0x8000;
        if ((int)remaining < 0x8000) count = remaining & 0xffff;
        count = fread(pch_copy_buffer, 1, count, input);
        remaining -= count;
        if (input->_flag & 0x20) report_fatal(84, message_text(339), 0);
        intermediate_status = fwrite(pch_copy_buffer, 1, count, output);
        if (intermediate_status == -1)
            report_error(85, message_text(339), 0, pch_diagnostic_name, 1485);
    } while (count == 0x8000);
    return bytes - remaining;
}

/* Original 0x47260..0x47430. Copy the saved global/line streams and
   optional debug/browser data into this invocation's active intermediates. */
void extract_pch_intermediates(int restore_debug)
{
    int byte;
    pch_copy_buffer = malloc(0x8000);
    PCH_IO_SEEK(fseek(pch_file, pch_expression_bytes + pch_symbol_bytes + 0x334, 0),
                344, options.option_Fp, 1540);
    rewind(global_file);
    copy_pch_stream(pch_file, global_file, pch_global_bytes);
    rewind(line_file);
    copy_pch_stream(pch_file, line_file, pch_line_bytes);
    if (restore_debug) {
        rewind(debug_file);
        copy_pch_stream(pch_file, debug_file, pch_debug_bytes);
    } else if (pch_debug_bytes) {
        PCH_IO_SEEK(fseek(pch_file, pch_debug_bytes, 1), 344, options.option_Fp, 1554);
    }
    if (pch_browser_bytes) {
        rewind(browser_output);
        copy_pch_stream(pch_file, browser_output, browser_saved_position);
        do {
            --pch_browser_bytes;
            byte = fgetc(pch_file);
        } while (byte);
        browser_open_file();
        browser_include_begin();
        copy_pch_stream(pch_file, browser_output, pch_browser_bytes - browser_saved_position);
        fread(browser_symbol_bits, 1, (sizeof(browser_symbol_bits) - 1), pch_file);
    }
    free(pch_copy_buffer);
}
