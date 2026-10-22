/* Declaration and aggregate initializer state. */
#include "initializers.h"
#include "initializer-records.h"

FrontendSymbol *initializer_symbol; /* original 0x6023c */
unsigned initializer_offset; /* original 0x60244 */
int initializer_runtime_override; /* original 0x6045c */
unsigned initializer_reset_words[3]; /* original 0x6cd18..0x6cd24 */

/* Original 0x2fd30..0x2fda9. Automatic/register declarations select the
   runtime initializer path; static storage starts with constant output. */
FrontendSymbol *__stdcall prepare_initializer_symbol(FrontendSymbol *symbol)
{
    unsigned char storage;
    initializer_runtime_override = 0;
    scanner_flags &= ~0x100000UL;
    if (parameter_nesting) {
        storage = symbol->storage[0] & 7;
        if (storage >= 4 && storage <= 6) scanner_flags |= 0x200000;
        else scanner_flags &= ~0x200000UL;
    } else scanner_flags &= ~0x200000UL;
    initializer_symbol = symbol;
    initializer_offset = 0;
    initializer_reset_words[0] = 0;
    initializer_reset_words[1] = 0;
    initializer_reset_words[2] = 0;
    return symbol;
}

InitializerFrame initializer_frames[32]; /* original 0x6025c..0x6045c */
InitializerFrame *initializer_frame; /* original 0x60258 */
int initializer_depth = -1; /* original 0x6cd14 */
int initializer_error_reported; /* original 0x60254 */
CType *initializer_inferred_type; /* original 0x60230 */

/* Original 0x300c0..0x3019a. Start a fresh aggregate traversal and
   defer static initializer output until its first value is encountered. */
void __stdcall begin_aggregate_initializer(FrontendSymbol *symbol)
{
    int sized_array;
    unsigned char storage;
    if (symbol) {
        initializer_error_reported = 0;
        initializer_depth = -1;
        initializer_inferred_type = 0;
        sized_array = (symbol->attributes & 0x500) == 0x500;
        initializer_symbol = symbol;
        initializer_offset = 0;
        push_initializer_frame(symbol);
        initializer_frame->flags = 1;
        if (is_scalar_initializer_symbol(initializer_frame->symbol) || sized_array)
            initializer_frame->flags |= 4;
        storage = initializer_frame->symbol->storage[0] & 7;
        if (!parameter_nesting || storage < 4 || storage > 6) {
            pending_initializer_symbol = symbol;
            if (browser_filename) {
                browser_begin_function_scope();
                browser_function_parameters(pending_initializer_symbol);
            }
        }
    }
    scanner_flags &= ~0x100000UL;
}

/* Original 0x6c070: sizes for symbol kinds 0..19. Tags and enum
   constants have shorter records than the complete declaration variant. */
unsigned initializer_symbol_copy_sizes[20] = {
    44, 44, 40, 38, 44, 28, 36, 32, 40, 44,
    36, 44, 44, 32, 44, 24, 16, 16, 16, 44
};

/* Original 0x30d40..0x30e3d. Each level reuses a persistent symbol/type
   pair while copying only the active symbol variant's declared prefix. */
void __stdcall push_initializer_frame(FrontendSymbol *symbol)
{
    unsigned bytes;
    unsigned char *source, *destination;
    initializer_frame = &initializer_frames[++initializer_depth];
    if (initializer_depth >= 32) fatal_error(54);
    if (!initializer_frame->symbol)
        initializer_frame->symbol = allocate_permanent(sizeof(FrontendSymbol));
    bytes = initializer_symbol_copy_sizes[symbol->kind & 0x1f];
    source = (unsigned char *)symbol;
    destination = (unsigned char *)initializer_frame->symbol;
    while (bytes--) *destination++ = *source++;
    if (!initializer_frame->type) initializer_frame->type = allocate_permanent(sizeof(CType));
    *initializer_frame->type = *symbol->type;
    initializer_frame->symbol->type = initializer_frame->type;
    if (symbol->attributes & 2) {
        initializer_frame->symbol->type->related = initializer_frame->symbol->type->related->related;
        initializer_frame->symbol->attributes &= ~2;
    }
    initializer_frame->string_seen = 0;
    initializer_frame->offset = 0;
    initializer_frame->flags = 0;
}

typedef char InitializerFrame_must_be_16_bytes[sizeof(InitializerFrame) == 16 ? 1 : -1];

/* Original 0x30cf0..0x30d38. Pointer shapes and primitive scalar kinds
   accept a single value; array shapes require aggregate traversal. */
int __stdcall is_scalar_initializer_symbol(FrontendSymbol *symbol)
{
    CType *type = symbol->type;
    TypeShape *shape = type->related;
    if (symbol->attributes & 2) shape = shape->related;
    if (shape) {
        if (shape->flags & 1) return 1;
        if ((shape->flags & 7) == 2) return 0;
    }
    return (type->flags & 0xb40) != 0;
}
