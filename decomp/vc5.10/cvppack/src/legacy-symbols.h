#ifndef CV510_LEGACY_SYMBOLS_H
#define CV510_LEGACY_SYMBOLS_H
#include "pack.h"
#include <recon.h>

typedef struct LegacySymbolSizer {
    unsigned char kind;
    void (__cdecl *narrow)(void);
    void (__cdecl *wide)(void);
} LegacySymbolSizer;
extern LegacySymbolSizer legacy_symbol_sizers[14];
extern unsigned char *legacy_symbol_input, *legacy_symbol_output;
extern unsigned short legacy_symbol_segment;
void convert_legacy_symbols(unsigned char *end);
unsigned short convert_legacy_numeric_leaf(unsigned char **input, unsigned char **output);
typedef struct LegacySymbolConverter {
    unsigned char kind;
    unsigned short narrow_kind;
    void (__cdecl *narrow)(void);
    unsigned short wide_kind;
    void (__cdecl *wide)(void);
} LegacySymbolConverter;
extern LegacySymbolConverter legacy_symbol_converters[14];
void convert_legacy_block16(void);
void convert_legacy_block32(void);
void convert_legacy_procedure16(void);
void convert_legacy_procedure32(void);
void convert_legacy_end(void);
void convert_legacy_bprel16(void);
void convert_legacy_bprel32(void);
void convert_legacy_data16(void);
void convert_legacy_data32(void);
void convert_legacy_label16(void);
void convert_legacy_label32(void);
void convert_legacy_register(void);
void convert_legacy_constant(void);
void convert_legacy_register_relative16(void);
void convert_legacy_skip(void);
void convert_legacy_change_segment(void);
void size_legacy_block16(void);
void size_legacy_procedure16(void);
void size_legacy_end(void);
void size_legacy_register_relative16(void);
void size_legacy_offset16(void);
void size_legacy_segment(void);
void size_legacy_skip(void);
void size_legacy_offset32(void);
void size_legacy_register(void);
void size_legacy_constant(void);
void size_legacy_procedure32(void);
/* Shared/coalesced RET identity at00408c50 remains classified CRT, not new App.
 * The unvisited native boundary deliberately traps, without no-op semantics. */
void __cdecl _setargv(void);
#endif
