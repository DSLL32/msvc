#ifndef CVPACK_OLDSYMBOLS_H
#define CVPACK_OLDSYMBOLS_H
#include <recon.h>

/* The two genuine indirect callsites pass no arguments. These sizing
 * callbacks read the current record and counters through application globals. */
typedef struct OldSymbolSizer {
    unsigned char kind;
    void (__cdecl *narrow)(void);
    void (__cdecl *wide)(void);
} OldSymbolSizer;
typedef char OldSymbolSizerSize[(sizeof(OldSymbolSizer)==12)?1:-1];
extern OldSymbolSizer old_symbol_sizers[14];
void size_old_block16(void);
void size_old_block32(void);
void size_old_end(void);
void size_old_label16(void);
void size_old_label32(void);
void size_old_register_relative16(void);
void size_old_register_relative32(void);
void size_old_symbol12(void);
void size_old_change_segment(void);
void size_old_skip(void);
void size_old_bprel16(void);
void size_old_bprel32(void);
void size_old_register(void);
void size_old_constant(void);
void size_old_data16(void);
void size_old_data32(void);
void size_old_procedure16(void);
void size_old_procedure32(void);

void convert_old_symbols(unsigned char *);
typedef struct OldSymbolConverter {
    unsigned char kind;
    unsigned short narrow_kind;
    void (__cdecl *narrow)(void);
    unsigned short wide_kind;
    void (__cdecl *wide)(void);
} OldSymbolConverter;
typedef char OldSymbolConverterSize[(sizeof(OldSymbolConverter)==16)?1:-1];
extern OldSymbolConverter old_symbol_converters[14];
void convert_old_symbol_block16(void);
void convert_old_symbol_block32(void);
void convert_old_symbol_procedure16(void);
void convert_old_symbol_procedure32(void);
void convert_old_symbol_end(void);
void convert_old_symbol_bprel16(void);
void convert_old_symbol_bprel32(void);
void convert_old_symbol_data16(void);
void convert_old_symbol_data32(void);
void convert_old_symbol_label16(void);
void convert_old_symbol_label32(void);
void convert_old_symbol_register(void);
void convert_old_symbol_constant(void);
void convert_old_symbol_register_relative16(void);
void convert_old_symbol_register_relative32(void);
void convert_old_symbol12(void);
void convert_old_symbol_skip(void);
void convert_old_symbol_change_segment(void);

#endif
