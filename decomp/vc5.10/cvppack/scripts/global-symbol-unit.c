/* Native-only focused duplicate/conflict branches, not an original oracle. */
#define RECON_SHIM_IMPL
#include "pack.h"
#include <recon.h>
#undef main

extern unsigned global_symbol_bytes, global_symbol_count, duplicate_public_symbols;

int main(void)
{
    unsigned char initial[] = "\027\000\003\020\024\020\000\000\020tagRPCOLEMESSAGE";
    unsigned char record[sizeof(initial)];
    type_table_local = calloc(4096, sizeof(void *));
    memcpy(record, initial, sizeof(initial));
    if (insert_global_symbol(record, symbol_record_key) != 1 ||
        *(unsigned short *)(record + 2) != 8 ||
        global_symbol_count != 1 || global_symbol_bytes != 28) return 120;
    memcpy(record, initial, sizeof(initial));
    if (insert_global_symbol(record, symbol_record_key) ||
        *(unsigned short *)(record + 2) != 8 || duplicate_public_symbols != 1 ||
        global_symbol_count != 1 || global_symbol_bytes != 28) return 121;
    memcpy(record, initial, sizeof(initial));
    ++*(unsigned *)(record + 4);
    if (insert_global_symbol(record, symbol_record_key) != 2 ||
        *(unsigned short *)(record + 2) != 0x1003 || duplicate_public_symbols != 1 ||
        global_symbol_count != 1 || global_symbol_bytes != 28) return 122;
    return 0;
}
