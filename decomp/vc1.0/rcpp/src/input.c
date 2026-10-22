/* Nested input files and shared segmented scanner buffers. */
#include "lexer.h"
#include <string.h>
char *__stdcall rc_copy_text_end(char *, const char *);
RcInput rc_inputs[10];
int rc_input_depth = -1, rc_line_number, rc_input_encoding, rc_expanding_macro;
unsigned char *rc_input_cursor;
FILE *rc_input_file;
static unsigned char byte_buffer[3 * 2050];
static unsigned short wide_buffer[3 * 2050];

/* Original [01009720,010099ad). Three overlapping buffer windows let shallow
 * includes retain more input, while deeper files share the last 2K window.
 * Rewind the parent stream whenever its buffered tail is about to be reused. */
int __stdcall rc_open_include(char *name, int fatal_if_missing)
{
    FILE *stream;
    RcInput *input, *parent;
    unsigned char *bytes, *cursor;
    unsigned short *wide;
    long position;
    stream = name ? fopen(name, "rb") : stdin;
    rc_input_file = stream;
    if (!stream) {
        if (fatal_if_missing == 1) {
            rc_message_format = rc_message(1013);
            rc_format(rc_diagnostic_buffer, rc_message_format, name);
            rc_fatal(1013);
        }
        return 0;
    }
    ++rc_input_depth;
    if (rc_input_depth >= 10) {
        rc_message_format = rc_message(1014);
        rc_format(rc_diagnostic_buffer, rc_message_format);
        rc_fatal(1014);
    }
    input = rc_inputs + rc_input_depth;
    if (!rc_input_depth) {
        bytes = byte_buffer + 1;
        wide = wide_buffer;
        input->capacity = 6144;
    } else {
        parent = input - 1;
        if (rc_input_depth == 1) {
            bytes = byte_buffer + 2051;
            wide = wide_buffer + 2050;
            input->capacity = 4096;
        } else {
            bytes = byte_buffer + 4101;
            wide = wide_buffer + 4100;
            input->capacity = 2048;
        }
        if (parent->buffered > 2048 || rc_input_depth > 2) {
            position = parent->file_position;
            cursor = rc_expanding_macro ? rc_saved_input_cursor : rc_input_cursor;
            if (bytes <= cursor) {
                position += cursor - parent->bytes;
                parent->file_position = position;
                parent->buffered = 0;
                if (rc_expanding_macro) rc_saved_input_cursor = parent->bytes;
                else rc_input_cursor = parent->bytes;
                *parent->bytes = 0;
                *parent->wide = 0;
            } else {
                parent->buffered = 2048;
                parent->bytes[2048] = 0;
                parent->wide[2048] = 0;
                position += 2048;
            }
            fseek(parent->stream, parent->encoding == 1 ? position : position * 2, SEEK_SET);
        }
    }
    input->saved_cursor = rc_input_cursor;
    input->saved_line = rc_line_number;
    input->stream = stream;
    input->bytes = bytes;
    input->wide = wide;
    input->buffered = 0;
    input->file_position = 0;
    input->encoding = rc_detect_encoding(stream);
    if (!input->encoding) {
        rc_message_format = rc_message(4413);
        rc_format(rc_diagnostic_buffer, rc_message_format, name);
        rc_warning(4413);
        input->encoding = 1;
    }
    rc_input_encoding = input->encoding;
    rc_input_cursor = bytes;
    rc_refill_input();
    input->saved_name = rc_copy_string(rc_source_name);
    strncpy(rc_source_name, name, 257);
    rc_line_number = 0;
    if (rc_options.preprocess_lines) {
        rc_emit_line();
        fwrite("\n", 1, 1, rc_output);
    }
    rc_begin_line();
    return 1;
}

/* Original [01009c50,01009d87). Zero bytes delimit buffer segments, while
 * Ctrl-Z encountered before a buffer boundary is the legacy end-of-file mark.
 * Wide input keeps its full words alongside low-byte scanner dispatch data. */
int rc_refill_input(void)
{
    RcInput *input = rc_inputs + rc_input_depth;
    int count, i;
    unsigned short word;
    if (rc_input_cursor - input->bytes < input->buffered) {
        if (rc_input_cursor[-1] == 0x1a) {
            --rc_input_cursor;
            return 1;
        }
        return 0;
    }
    rc_input_cursor = input->bytes;
    if (input->encoding == 1) {
        count = fread(input->bytes, 1, input->capacity, rc_input_file);
    } else {
        count = fread(input->wide, 2, input->capacity, rc_input_file);
        if (input->encoding == 3)
            for (i = 0; i < count; ++i) {
                word = input->wide[i];
                input->wide[i] = (unsigned short)((word << 8) | (word >> 8));
            }
        for (i = 0; i < count; ++i) input->bytes[i] = (unsigned char)input->wide[i];
    }
    input->file_position += input->buffered;
    input->buffered = count;
    input->bytes[count] = 0;
    input->wide[count] = 0;
    return count ? 0 : 1;
}

/* Original [010099b0,01009a69). Restore the parent's logical name/line
 * and scanner position; deep includes refill their reused buffer window. */
unsigned char rc_close_include(void)
{
    RcInput *finished, *parent;
    int final_line;
    if (rc_input_depth == -1) return 0;
    fclose(rc_input_file);
    finished = rc_inputs + rc_input_depth;
    rc_copy_text_end(rc_source_name, finished->saved_name);
    final_line = rc_line_number;
    rc_line_number = finished->saved_line;
    rc_input_cursor = finished->saved_cursor;
    if (--rc_input_depth < 0) { rc_line_number = final_line; return 0; }
    parent = rc_inputs + rc_input_depth;
    rc_input_file = parent->stream;
    rc_input_encoding = parent->encoding;
    if (rc_input_depth >= 2) rc_refill_input();
    if (rc_options.preprocess_lines) rc_emit_line();
    return 1;
}

/* Original [010109b0,010109ca): returns the position of the copied NUL. */
char *__stdcall rc_copy_text_end(char *out, const char *text)
{
    while ((*out++ = *text++) != 0) ;
    return out - 1;
}
