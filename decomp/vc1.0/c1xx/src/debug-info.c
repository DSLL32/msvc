/* Frontend debug intermediate initialization and CodeView record output. */
#include <stdarg.h>
#include <time.h>
#include "debug-info.h"
#include "global-ir.h"
#include "control-ir.h"
#include <recon.h>

unsigned char *debug_record_buffer; /* original 0x8ccb4 */
unsigned debug_stream_offset; /* original 0x9f45c */

/* Original 0x6d660..0x6d6b8. The ordinary CodeView stream begins
   with its record formats and a four-byte version signature. Line-record
   mode retains its separate original initialization entry. */
void begin_debug_info(void)
{
    if (options.configuration.line_records) {
        begin_line_record_debug_info();
        return;
    }
    debug_record_buffer = checked_malloc(4096);
    write_debug_record(12);
    write_debug_record(10);
    *(unsigned *)debug_record_buffer = 1;
    write_debug_record(3, 4, debug_record_buffer);
    debug_stream_offset += 4;
}

/* Original 0x6d6c0..0x6d6da. Line-record mode closes its own
   debug producer; ordinary CodeView output ends with record seven. */
void finish_debug_info(void)
{
    if (options.configuration.line_records) {
        finish_line_record_debug_info();
        return;
    }
    write_debug_record(7);
}

/* These thirteen encountered record schemas belong to the debug stream.
   The second word of each original eight-byte row is not read by this writer. */
char *debug_record_formats[13] = {
    "mdox", "tpzv", "tsfz", "tl", "ti", "tr", "t", "t", "tn", "td",
    "mox", "tb", "tg"
};

/* Original 0x704a0..0x7067e, switch data 0x70680..0x706e4.
   Formats consume their own argument shapes and temporarily select the debug
   FILE for the ordinary shared intermediate writers. Unrecognized format
   letters are annotations and deliberately consume no argument. */
void write_debug_record(unsigned record, ...)
{
    FILE *saved = auxiliary_ir_stream;
    char *format;
    va_list arguments;
    char *block;
    short length;
    int integer;
    unsigned index;
    CppSymbol *symbol;
    auxiliary_ir_stream = debug_file;
    va_start(arguments, record);
    format = debug_record_formats[record];
    if (!format) return;
    for (; *format; ++format) {
        switch (*format) {
        case 'b':
            block = va_arg(arguments, char *);
            write_counted_ir(*(short *)block, block + 2);
            break;
        case 'f':
            integer = 0;
            write_ir_integer(&integer);
            break;
        case 'g':
            putc(1, auxiliary_ir_stream);
            break;
        case 'l':
            length = (short)va_arg(arguments, int);
            block = va_arg(arguments, char *);
            write_counted_ir(length, block);
            break;
        case 'm':
        case 't':
            putc((char)record, auxiliary_ir_stream);
            break;
        case 'o':
            integer = (int)debug_stream_offset;
            write_ir_integer(&integer);
            break;
        case 'r':
            integer = 1;
            write_ir_integer(&integer);
            break;
        case 's':
            symbol = va_arg(arguments, CppSymbol *);
            if (symbol->attributes & 0x40) {
                if ((symbol->kind & 31) == 3) symbol = static_member_alias(symbol);
                else symbol = symbol_alias(symbol);
            }
            index = symbol->index;
            write_ir_index(&index);
            break;
        case 'z':
            putc((char)va_arg(arguments, int), auxiliary_ir_stream);
            break;
        }
    }
    va_end(arguments);
    auxiliary_ir_stream = saved;
}

DebugDatabase *debug_database; /* original0xa269c */
unsigned char *line_record_buffer; /* original0x8cca0 */
unsigned char *line_record_leaf; /* original0x8cc9c */

/* Original0x70800..0x7089c. Open the separate debug database once,
   using the actual reproducible signature option before beginning its stream. */
void begin_line_record_debug_info(void)
{
    if (debug_database) return;
    if (options.configuration.option_Brepro)
        debug_database = open_debug_database(options.configuration.database_path, "wi", 1);
    else debug_database = open_debug_database(options.configuration.database_path,
        "wi", (unsigned)time(0));
    if (!debug_database) {
        report_syntax_error(470, options.configuration.database_path);
        return;
    }
    initialize_line_record_buffer();
    write_debug_record(12);
    write_debug_record(10);
    *(unsigned *)line_record_buffer = 1;
    write_debug_record(3, 4, line_record_buffer);
    emit_line_record_database_reference();
}
