#include <string.h>
#include <new.h>
#include "frontend.h"
#include <recon.h>

extern "C" void initialize_output_buffer_03(void);
extern "C" void write_global_ir_record(unsigned, struct CompilerSymbol *);

/* The independently decoded original format for header17 is3,29,30,37,32.
 * A nondefault symbol counter distinguishes the live cell from a constant. */
extern "C" int c1xx_test_global_header(void)
{
    static const BYTE expected[] = {
        17, 0x86, 3, 0x60, 0x92, 0x30, 1, 0x44, 0x33, 0x22, 0x11, 0
    };
    OutputBuffer *buffer = &frontend_bss.output_buffers[2];
    initialize_output_buffer_03();
    frontend_data.option_c9il = 1;
    frontend_data.report_statistics = 0;
    frontend_data.next_symbol_record_id = 0x11223344;
    frontend_data.emit_global_locations = 0;
    frontend_bss.global_output_active = 0;
    write_global_ir_record(17, NULL);
    int result = buffer->ok && buffer->data &&
        buffer->cursor - buffer->data == sizeof(expected) &&
        memcmp(buffer->data, expected, sizeof(expected)) == 0 &&
        frontend_data.next_symbol_record_id == 0x11223344;
    ::operator delete(buffer->data);
    return result;
}
