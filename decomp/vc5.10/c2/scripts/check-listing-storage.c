/* Code-side condition check for the actual reconstructed section routine.
 * Child mocks deliberately cover no compiler behavior beyond this condition.
 */
#include "c2.h"
#include <recon.h>

int c2_internal_listing, stack_alignment = 4;
C2Symbol *current_symbol;
static C2FunctionSymbol owner;
static C2InputSymbol input;
static C2Section chosen;
static int known, selected;
static void fail(void) { abort(); }
int __fastcall is_constant_symbol(C2Symbol *symbol) { if(symbol != &input.symbol) fail(); return 0; }
int __fastcall is_known_symbol(unsigned serial) { if(serial != 42) fail(); return known; }
C2Section *__fastcall select_symbol_section(C2FunctionSymbol *function, C2Symbol *symbol)
{
    if(function != &owner || symbol != &input.symbol) fail();
    ++selected;
    return &chosen;
}
unsigned next_symbol_serial(void) { fail(); return 0; }
C2Symbol *__fastcall lookup_global_symbol(unsigned serial) { fail(); return 0; }
void *__fastcall c2_allocate_node(signed char kind, int pool) { fail(); return 0; }
void __fastcall insert_symbol(C2Symbol *symbol, int local) { fail(); }
void *__fastcall c2_pool_allocate(int pool, unsigned bytes) { fail(); return 0; }
void __fastcall c2_report_internal(char *filename, int line) { fail(); }

int main(int argc, char **argv, char **environment)
{
    int flags[4] = {0, 1, -1, 1};
    unsigned expected[4] = {4, 73, 73, 4};
    int index;
    current_symbol = &owner.symbol;
    chosen.symbol.serial = 73;
    for(index = 0; index != 4; ++index) {
        memset(&input, 0, sizeof(input));
        input.symbol.kind = 1;
        input.symbol.serial = 42;
        input.attributes = 0xa0;
        c2_internal_listing = flags[index];
        known = index == 3;
        selected = 0;
        update_symbol_storage(&input.symbol);
        if(input.symbol.association.section_serial != expected[index] ||
           selected != (index == 1 || index == 2)) fail();
    }
    return 0;
}
