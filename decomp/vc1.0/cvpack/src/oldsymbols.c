/* C6 local-symbol sizing dispatch, shared by the 16/32-bit record families. */
#include "oldsymbols.h"
#include <recon.h>

OldSymbolSizer old_symbol_sizers[14] = {
    { 0x00, size_old_block16, size_old_block32 },
    { 0x01, size_old_procedure16, size_old_procedure32 },
    { 0x02, size_old_end, size_old_end },
    { 0x04, size_old_bprel16, size_old_bprel32 },
    { 0x05, size_old_data16, size_old_data32 },
    { 0x0b, size_old_label16, size_old_label32 },
    { 0x0c, size_old_block16, size_old_block32 },
    { 0x0d, size_old_register, size_old_register },
    { 0x0e, size_old_constant, size_old_constant },
    { 0x0f, size_old_procedure16, size_old_procedure32 },
    { 0x10, size_old_skip, size_old_skip },
    { 0x11, size_old_change_segment, size_old_change_segment },
    { 0x16, size_old_register_relative16, size_old_register_relative32 },
    { 0x12, size_old_symbol12, size_old_symbol12 },
};
