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
#endif
