/* Compiler intrinsic declarations and their scope registration. */
#include "builtins.h"
#include "keywords.h"
#include "parser.h"
#include <recon.h>

unsigned next_symbol_index = 100; /* original 0x6b7f4 */
unsigned browser_symbol_limit = 65535; /* original 0x6b7f8 */
unsigned char current_storage[3]; /* original 0x5b258 */

/* Original 0x2d860..0x2da99. */
void initialize_intermediate_output(void)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    TypeDescription *description;
    FrontendSymbol *symbol;
    BuiltinDefinition *definition;
    char name[1024], *source, *destination, *interned;
    unsigned group, index;
    unsigned short flags, shape;
    int underscore;
    if (pool->remaining < (int)sizeof(TypeDescription))
        refill_arena(&compiler_arena, pool);
    description = (TypeDescription *)pool->next;
    pool->remaining -= sizeof(TypeDescription);
    pool->next += sizeof(TypeDescription);
    description->field08 = 0;
    for (group = 0; group < 28; ++group) {
        for (index = builtin_group_boundaries[group];
             index < builtin_group_boundaries[group + 1]; ++index) {
            definition = &builtin_definitions[index];
            if (options.option_MT && !definition->argument_flags)
                continue;
            if (!builtin_enabled(index))
                continue;
            name[0] = (char)(group + '_');
            source = definition->name + 1;
            destination = name + 1;
            do { *destination++ = *source++; } while (destination[-1]);
            interned = intern_identifier(name);
            symbol = allocate_permanent(sizeof(FrontendSymbol));
            symbol->name = interned;
            if (next_symbol_index >= 0x7fffffffUL)
                fatal_error(55);
            if (next_symbol_index >= browser_symbol_limit && browser_filename)
                browser_index_overflow();
            symbol->index = next_symbol_index++;
            mark_builtin(symbol, index);
            flags = definition->return_type;
            if ((flags & 0x2fdf) == 0x41 && options.extensions_enabled && options.option_J && !(flags & 0x8000))
                flags |= 0x4000;
            description->flags = flags;
            underscore = options.option_MT || (interned[0] == '_' && interned[1] == 'f');
            shape = definition->return_shape;
            if (shape) {
                shape &= 7;
                description->related = new_type_shape(shape,
                    normalize_type_attributes(shape, (unsigned short)(underscore ? 0 : 0)), 0);
            } else
                description->related = 0;
            symbol->type = make_function_type((unsigned short)0x10, description);
            insert_scope_symbol(global_scope, symbol);
            if (browser_filename)
                browser_symbol_definition(symbol);
            if (definition->flags & 0x80)
                symbol->attributes |= 1;
        }
    }
}

/* Original 0x2dbd0..0x2dc55. */
int __stdcall builtin_enabled(unsigned index)
{
    BuiltinDefinition *definition;
    if ((options.option_M & 4) && index >= 172 && index <= 173)
        return 0;
    definition = &builtin_definitions[index];
    if (definition->flags & 0x80)
        return 1;
    if (options.option_Oi) {
        if (!options.extensions_enabled && (definition->flags & 0x40))
            return 0;
        if (normalized_Op && !(definition->return_type & 0x100))
            return 0;
        return 1;
    }
    return 0;
}

/* Original 0x2db30..0x2db8a. */
void __stdcall mark_builtin(FrontendSymbol *symbol, unsigned index)
{
    unsigned char storage;
    symbol->kind = 6;
    storage = (symbol->storage[0] & 0xf9) | 1;
    symbol->storage[0] = (current_storage[0] & ~7) | (storage & 7);
    symbol->storage[1] = current_storage[1];
    symbol->storage[2] = current_storage[2];
    symbol->detail.intrinsic.operation = (unsigned short)builtin_definitions[index].operation;
    symbol->detail.intrinsic.flags = builtin_definitions[index].flags;
}

/* Original 0x2dce0..0x2dd6a. #pragma function converts a known intrinsic
   declaration into an external function, except for mandatory intrinsics. */
void __stdcall disable_intrinsic_function(char *name)
{
    FrontendSymbol *symbol;
    unsigned kind;
    if (find_builtin_index(name) == -1) {
        compiler_warning(1, 163, name);
        return;
    }
    symbol = find_identifier_symbol(name);
    if (!symbol) {
        compiler_warning(1, 164, name);
        return;
    }
    kind = symbol->kind & 0x1f;
    if (kind != 6 && kind != 7) {
        report_syntax_error(170, name);
        return;
    }
    if (symbol->detail.intrinsic.flags & 0x80) {
        compiler_warning(1, 167, name);
        return;
    }
    make_intrinsic_external(symbol);
}

/* Original 0x2daa0..0x2db2b. Group interned identifier spellings by their
   initial underscore/lowercase character, then compare their suffixes. */
int __stdcall find_builtin_index(char *name)
{
    int group = (signed char)*name - '_';
    unsigned index, end;
    char *left, *right;
    if (group < 0) return -1;
    index = builtin_group_boundaries[group];
    end = builtin_group_boundaries[group + 1];
    while (index != end) {
        left = builtin_definitions[index].name + 1;
        right = name + 1;
        while (*left && *left == *right) { ++left; ++right; }
        if (*left == *right) return index;
        ++index;
    }
    return -1;
}

/* Original 0x2db90..0x2dbce. Retain the intrinsic payload while changing
   the symbol kind and installing external-function storage with the disable bit. */
void __stdcall make_intrinsic_external(FrontendSymbol *symbol)
{
    symbol->kind = 7;
    symbol->storage[0] = (current_storage[0] & ~7) | 0x21;
    symbol->storage[1] = current_storage[1];
    symbol->storage[2] = current_storage[2];
}

/* Original0x2dc60..0x2dcdd. Explicit intrinsic selection can restore a
   declaration previously made external by #pragma function. */
void __stdcall enable_intrinsic_function(char *name)
{
    int index;
    FrontendSymbol *symbol;
    unsigned kind;
    if (options.option_qc) return;
    index = find_builtin_index(name);
    if (index == -1) { compiler_warning(1,163,name); return; }
    symbol = find_identifier_symbol(name);
    if (!symbol) { compiler_warning(1,164,name); return; }
    kind = symbol->kind & 31;
    if (kind != 6 && kind != 7) { report_syntax_error(170,name); return; }
    mark_builtin(symbol,index);
}
