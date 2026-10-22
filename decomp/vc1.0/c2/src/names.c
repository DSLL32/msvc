/* Object symbol spelling and compiler-owned copies. */
#include "records.h"
#include "machine.h"
#include "function-input.h"
#include "options.h"
#include <recon.h>
extern unsigned descriptor_bytes[7];
void __fastcall uppercase_name(char *name);
unsigned __fastcall format_integer(unsigned value, char *text, unsigned radix);
static char symbol_name_buffer[256]; /* 0007e3b0: reused object-name workspace */
/* Original [0004e000,0004e2b0). Names are already decorated by the input
   reader. Internal objects and labels receive an identifier suffix. Copies
   and concatenations were compiler intrinsics and introduce no call nodes. */
char *__fastcall symbol_display_name(SymbolHeader *symbol)
{
    char *destination = symbol_name_buffer, *source = symbol->name;
    char *suffix = NULL;
    int numbered = 0, truncate = 1;
    unsigned storage;
    switch (symbol->kind) {
    case 1:
        if (!source) {
            *destination++ = '$';
            if (symbol->field0d == '$') *destination++ = 'S';
            else if (symbol->field0d == '%') {
                *destination++ = 'S'; *destination++ = 'G';
            } else *destination++ = 'T';
            format_integer(symbol->identifier, destination, 10);
            return symbol_name_buffer;
        }
        if (symbol->field0d == '$') {
            suffix = "$S";
            numbered = 1;
        } else {
            storage = ((BackendSymbol *)symbol)->attributes & 0x70;
            if (storage == 0x10 || storage == 0x30) {
                suffix = "$";
                numbered = (signed char)((TemporarySymbol *)symbol)->anonymous > 2;
            }
        }
        break;
    case 3:
        if (symbol->field0d) {
            *destination++ = '$'; *destination++ = 'L';
            format_integer(symbol->identifier, destination, 10);
            return symbol_name_buffer;
        }
        suffix = "$"; numbered = 1;
        break;
    case 4:
        if (!source) {
            *destination++ = '$'; *destination++ = 'E';
            format_integer(symbol->identifier, destination, 10);
            return symbol_name_buffer;
        }
        break;
    case 9:
        truncate = 0;
        break;
    default:
        compiler_internal_error("@(#)p2symtab.c:1.89", 0x362);
        return symbol_name_buffer;
    }
    while ((*destination++ = *source++) != 0) ;
    if (truncate) symbol_name_buffer[identifier_limit] = 0;
    if (suffix) {
        destination = symbol_name_buffer;
        while (*destination) ++destination;
        while ((*destination++ = *suffix++) != 0) ;
    }
    if (numbered) {
        destination = symbol_name_buffer;
        while (*destination) ++destination;
        format_integer(symbol->identifier, destination, 10);
    }
    return symbol_name_buffer;
}
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
