/* Focused injected native checks; callee mocks are not reconstruction credit. */
#include "browser.h"
#include "initialization.h"
#include <recon.h>
int error_count;
unsigned char declaration_mode;
unsigned browser_symbol_limit = 100;
char *name_main, *name_winmain, *name_dllmain;
static CppSymbol alias;
void __stdcall fill_browser_symbol(BrowserRecord *record, CppSymbol *symbol)
{
    record->name = symbol->name;
    record->name_identity = symbol->name;
    record->symbol_index = symbol->index;
}
char *__stdcall write_decorated_name(FILE *file, CppSymbol *symbol,
    CppSymbol *context, int flags) { return symbol->name; }
void suppress_browser_records(void) { error_count = 1; }
CppSymbol *__stdcall static_member_alias(CppSymbol *symbol) { return &alias; }
CppSymbol *__stdcall symbol_alias(CppSymbol *symbol) { return &alias; }
CppSymbol *__stdcall static_member_from_alias(CppSymbol *symbol) { return &alias; }
void __stdcall set_browser_access(unsigned short *flags, unsigned char access) { }
extern unsigned char browser_declaration_categories[12];
int main(int argc, char **argv)
{
    BrowserRecord record = {0};
    CppSymbol symbol = {0};
    int test = argc > 1 ? argv[1][0] : '0';
    symbol.name = "plain";
    symbol.kind = 7;
    symbol.declaration_flags = 0x200; /* suppress decoration in focused checks */
    alias.name = "alias";
    alias.kind = 17; /* name-only alias must not replace classification owner */
    if (test == '1') symbol.kind = 12;
    if (test == '2') { symbol.kind = 12; symbol.flags = 0x80; }
    if (test == '3') symbol.kind = 11;
    if (test == '4') { symbol.kind = 9; symbol.attributes = 0x40; }
    if (test == '5') browser_declaration_categories[7] = 2;
    fill_browser_declaration(&record, &symbol);
    if (test == '2' || test == '3') return record.header.kind != 0xfc;
    if (test == '4') return record.category != 4 || record.name != alias.name;
    return record.category != 1;
}
