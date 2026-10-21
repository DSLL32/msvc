/* Object symbol spelling and compiler-owned copies. */
#include "records.h"
extern unsigned descriptor_bytes[7];
void __fastcall uppercase_name(char *name);
/* Original [0004dab0,0004dcb0), including two decoration dispatch tables. */
char *__fastcall intern_symbol_name(SymbolHeader *symbol, char *name, int arena)
{
    char plain[256], imported[256];
    char *source = name, *destination = plain + 1, *spelling = plain + 1;
    unsigned convention, length;
    int kind = symbol->kind;
    plain[0] = '_';
    plain[1] = 0;
    while ((*destination++ = *source++) != 0) ;
    if (*name != '?' || symbol->field0d) {
        switch (kind) {
        case 1: case 7: case 8: case 11:
            spelling = plain;
            break;
        case 3:
            if (symbol->field0d) return NULL;
            spelling = plain;
            plain[0] = '$';
            break;
        case 4:
            convention = symbol->field0e & 0x1c;
            switch (convention) {
            case 0: case 16: case 28:
                spelling = plain;
                break;
            case 4: case 20:
                spelling = plain;
                plain[0] = '@';
                break;
            case 8: case 12:
                if (symbol->field0d != 0x19) uppercase_name(plain + 1);
                break;
            }
            break;
        }
    }
    if ((kind == 1 || kind == 4) && (((FunctionSymbol *)symbol)->field24 & 0x1000)) {
        imported[0] = '_'; imported[1] = '_'; imported[2] = 'i';
        imported[3] = 'm'; imported[4] = 'p'; imported[5] = '_';
        imported[6] = 0;
        source = spelling;
        destination = imported + 6;
        while ((*destination++ = *source++) != 0) ;
        spelling = imported;
    }
    length = 0;
    while (spelling[length]) ++length;
    descriptor_bytes[arena] += length;
    return arena_strdup(spelling, arena);
}
