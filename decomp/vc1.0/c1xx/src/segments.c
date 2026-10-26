/* Named sections use the compiler's common symbol and interned-name owners. */
#include "segments.h"
#include "function-finalization.h"
#include "function-definitions.h"
#include "macros.h"
#include <recon.h>
extern SegmentSymbol *default_data_segment;
/* Encountered default far-segment roots are initially NULL in original data. */
SegmentSymbol *default_far_data_segment; /* original9f3dc */
SegmentSymbol *default_far_code_segment; /* original9f3ec */

/* Original0x550e0..0x55265, stdcall16. Existing declarations retain
   compatible code/data kinds and class names. Builtin segment indices5/2
   follow the current function and pragma defaults before being returned. */
SegmentSymbol *__stdcall lookup_segment_symbol(char *name, char *class_name,
    unsigned char kind, unsigned unused_mode)
{
    SegmentSymbol *segment = find_segment_symbol(name);
    unsigned char previous;
    unsigned index;
    int requested_code, previous_code;
    if (segment) {
        previous = segment->auxiliary.segment_kind;
        if (kind != 8 && kind != previous) {
            requested_code = kind == 0 || kind == 2;
            previous_code = previous == 0 || previous == 2;
            if (requested_code != previous_code) {
                if (previous_code && segment->index != 5)
                    report_syntax_error(194,segment->name);
                else if (kind != 1)
                    report_syntax_error(195,segment->name);
            }
        }
        if (class_name && class_name != name_unnamed) {
            if (!segment->detail.segment_class || segment->detail.segment_class == name_unnamed)
                segment->detail.segment_class = class_name;
            if (segment->detail.segment_class != class_name)
                compiler_warning(1,613,segment->name);
        }
        if (segment->index == 5 && (token_allocation_flags & 0x80000) &&
            (function_symbol->attributes & 2))
            segment = function_symbol->type->related->data.based_symbol;
        if (default_code_segment && segment->index == 5)
            segment = default_code_segment;
        if (default_data_segment && segment->index == 2)
            segment = default_data_segment;
        if (segment->auxiliary.segment_kind == 2 && !kind)
            segment->auxiliary.segment_kind = kind;
        return segment;
    }
    if (kind == 8 && segment_name_has_suffix(name,"_TEXT")) kind = 2;
    if (next_symbol_index >= 0x7fffffffU) fatal_error(55);
    if (next_symbol_index >= browser_symbol_limit && browser_filename)
        browser_symbol_rollover();
    index = next_symbol_index++;
    return new_segment_symbol(name,class_name,index,kind);
}

/* Original0x54e80..0x54ecd, stdcall4. Segment names are interned;
   preserve the original in-place127-byte truncation before identity lookup. */
SegmentSymbol *__stdcall find_segment_symbol(char *name)
{
    unsigned length;
    for (length = 0; name[length]; ++length) ;
    if (length > 127) name[127] = 0;
    if (!segment_symbols) return 0;
    if (name && name != name_unnamed)
        return find_name_in_chain(segment_symbols,name);
    return 0;
}

/* Original0x54f00..0x54f4c, stdcall16. Named segments join the shared
   symbol list; anonymous placeholders retain the supplied record only. */
SegmentSymbol *__stdcall new_segment_symbol(char *name, char *class_name,
    unsigned index, unsigned char kind)
{
    SegmentSymbol *segment = allocate_permanent(sizeof(SegmentSymbol));
    segment->kind = 13;
    segment->index = index;
    segment->auxiliary.segment_kind = kind;
    segment->detail.segment_class = class_name;
    segment->name = name;
    segment->type = type_0802;
    if (name && name != name_unnamed) {
        segment->next = segment_symbols;
        segment_symbols = segment;
    }
    return segment;
}

/* Original0x62860..0x628ab, stdcall8. A defined function retains its
   current function attributes; other declarations use the explicit segment
   pointer model. The qualifier builder consumes the local segment owner. */
void __stdcall assign_symbol_segment(CppSymbol *symbol, SegmentSymbol *segment)
{
    SegmentSymbol *base;
    unsigned short attributes;
    symbol->attributes |= 2;
    base = segment;
    attributes = (symbol->kind & 31) == 7 ? symbol->type->attributes : 5;
    symbol->type = make_qualified_type(symbol->type,0x400,attributes,&base);
}

/* Original0x55070..0x550db, stdcall8. A suffix must be strictly
   shorter than the segment name; equal complete names deliberately fail. */
int __stdcall segment_name_has_suffix(char *name, char *suffix)
{
    unsigned length, suffix_length;
    for (length = 0; name[length]; ++length) ;
    for (suffix_length = 0; suffix[suffix_length]; ++suffix_length) ;
    if (suffix_length >= length) return 0;
    name += length - suffix_length;
    while (*name && *name == *suffix) { ++name; ++suffix; }
    return *name == *suffix;
}
