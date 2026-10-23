#include "statement-scopes.h"
#include "expression-preparation.h"
/* Linked records assembled before frontend expression output. */
#include "frontend-records.h"
#include "parameters.h"
#include "parser.h"
#include "cpp-context.h"
#include <setjmp.h>
#include <recon.h>

FrontendRecord *ir_record_head; /* original 0xa0a00 */
FrontendRecord *ir_record_tail; /* original 0xa0a04 */
/* Saved old tail during insertion, then the new function's tail after
   moving that suffix ahead of the earlier records. */
FrontendRecord *function_ir_tail; /* original 0xa0a5c */
int ir_function_error_count; /* original 0xa0a60 */
int ir_records_suppressed; /* original 0xa0a64 */

/* Original 0x3f0e0..0x3f154, cdecl. Open an if's conditional cleanup
   context before opening its expression-evaluation region. */
void begin_if_condition(void)
{
    ConditionalFrontendRecord record;
    record.header.kind = 17;
    record.header.flags = 0;
    record.header.field02 = 0;
    record.header.sequence = 0;
    record.header.next = 0;
    record.header.previous = 0;
    record.then_record = 0;
    record.else_record = 0;
    if (ir_records_suppressed) return;
    append_frontend_record(&record.header, sizeof(record));
    begin_if_condition_region();
}

/* Original 0x3f160..0x3f199, cdecl. The condition evaluation has its
   own linked-record boundary, even when it creates no temporary objects. */
void begin_if_condition_region(void)
{
    FrontendRecord record;
    record.kind = 18;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
}

/* Original 0x3f200..0x3f2bd, cdecl. Link the then-region opener to
   its if record after closing condition temporaries. Recovery owns its
   automatic jump buffer only while no outer recovery context is installed. */
void begin_if_body(void)
{
    BranchBoundaryRecord record;
    ConditionalFrontendRecord *conditional;
    jmp_buf recovery;
    record.header.kind = 20;
    record.header.flags = 0;
    record.header.field02 = 0;
    record.header.sequence = 0;
    record.header.next = 0;
    record.header.previous = 0;
    record.goto_count = 0;
    if (syntax_diagnostic_count && !ir_record_error_context) {
        if (ir_records_suppressed) return;
        ir_record_error_context = recovery;
        if (setjmp(recovery)) {
            ir_record_error_context = 0;
            return;
        }
    }
    end_if_condition_region();
    record.goto_count = ir_goto_count;
    append_frontend_record(&record.header, sizeof(record));
    conditional = (ConditionalFrontendRecord *)find_frontend_record(ir_record_tail, 17, 1);
    conditional->then_record = ir_record_tail;
    if (ir_record_error_context == recovery) ir_record_error_context = 0;
}

/* Original 0x3f1a0..0x3f1f8, cdecl. Empty condition-evaluation regions
   collapse; occupied regions retain a matching close record. */
void end_if_condition_region(void)
{
    FrontendRecord record;
    if (ir_record_tail && (ir_record_tail->kind & 0x3f) == 18) {
        remove_last_frontend_record();
        return;
    }
    record.kind = 19;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
}

/* Original 0x3d6a0..0x3d823. Begin function records and parameter cleanup
   scope, preserving a prior record prefix by rotating the new suffix to
   the front of the doubly linked list. */
void __stdcall begin_function_ir(CppSymbol *function, CppSymbol *parameters)
{
    FunctionRecord record;
    CppSymbol *parameter, *first;
    FrontendRecord *start, *old_head;
    record.header.kind = 1;
    record.header.flags = 0;
    record.header.field02 = 0;
    record.header.sequence = 0;
    record.header.next = 0;
    record.header.previous = 0;
    record.function = function;
    record.field14 = 0;
    function_ir_tail = ir_record_tail;
    if (ir_records_suppressed || !ir_record_tail || (ir_record_tail->kind & 0x3f) != 13)
        reset_frontend_records();
    ir_records_suppressed = 0;
    append_frontend_record(&record.header, sizeof(record));
    ir_function_error_count = error_count;
    parameter = parameters;
    while (parameter && (parameter->kind & 31) != 10 && (parameter->kind & 31) != 11)
        parameter = parameter->next;
    if (parameter) {
        first = parameter = parameter->detail.parameter_next;
        begin_frontend_scope();
        do {
            record_parameter_cleanup(parameter);
            if (!parameter) break;
            parameter = parameter->detail.parameter_next;
        } while (parameter != first);
        end_frontend_scope();
    }
    if (function_ir_tail) {
        start = function_ir_tail->next;
        old_head = ir_record_head;
        old_head->previous = ir_record_tail;
        ir_record_tail->next = old_head;
        start->previous = 0;
        function_ir_tail->next = 0;
        ir_record_head = start;
        ir_record_tail = function_ir_tail;
        function_ir_tail = old_head->previous;
    }
}

FrontendRecord *ir_free_records; /* original 0xa0a08 */
int ir_expression_depth; /* original 0xa0a0c, prepare_expression nesting */
unsigned ir_record_sequence; /* original 0xa0a10, stored in each record */
unsigned ir_goto_count; /* original 0xa0a14, incremented by record_goto */
CppSymbol *initializing_symbol; /* original 0xa0a18 */
int initializing_reference; /* original 0xa0a1c, mark_initialized_symbol */
int member_initializer_active; /* original 0xa0a20, class-member initialization */
FrontendRecord *ir_label_records; /* original 0xa0a28, label record chain */
FrontendRecord *ir_goto_records; /* original 0xa0a2c, goto record chain */
FrontendRecord *ir_switch_records; /* original 0xa0a30, nested switch records */

/* Original 0x40eb0..0x40f3c. Reset the encountered work-record context,
   including the parser's canonical braced-initializer flag at 0xa0a24. */
void reset_frontend_records(void)
{
    ir_free_records = 0;
    ir_record_head = 0;
    ir_record_tail = 0;
    function_ir_tail = 0;
    ir_expression_depth = 0;
    ir_record_sequence = 0;
    ir_goto_count = 0;
    ir_label_records = 0;
    ir_goto_records = 0;
    ir_switch_records = 0;
    initializing_symbol = 0;
    initializing_reference = 0;
    member_initializer_active = 0;
    initializer_list_started = 0;
}

int *ir_record_error_context; /* original 0xa0a68 */
typedef char RecordJumpBufferSizeMustBe32[(sizeof(jmp_buf) == 32) ? 1 : -1];

/* Original 0x40b80..0x40d51. Copy an encountered record into a rounded
   allocation, link it, and assign its sequence number. The generic arena
   selection retains the original macro's constant-one selector and its
   dormant permanent/-1 paths, preserving those native call edges. */
void __stdcall append_frontend_record(FrontendRecord *record, unsigned bytes)
{
    FrontendRecord *node, *function_record;
    ArenaPool *pool;
    jmp_buf overflow_context;
    unsigned char size_class;
    unsigned allocation_bytes, index;
    int arena;
    if (ir_records_suppressed) return;
    if (ir_record_sequence == (unsigned)-1) {
        if (syntax_diagnostic_count && !ir_record_error_context) {
            if (ir_records_suppressed) return;
            ir_record_error_context = overflow_context;
            if (setjmp(overflow_context)) {
                ir_record_error_context = 0;
                return;
            }
        }
        function_record = find_frontend_record(ir_record_tail, 1, 1);
        fatal_error(38, format_symbol_name(((FunctionRecord *)function_record)->function));
        if (ir_record_error_context == overflow_context) ir_record_error_context = 0;
    }
    if (bytes <= 16) size_class = 0;
    else if (bytes <= 32) size_class = 1;
    else if (bytes <= 48) size_class = 2;
    else size_class = 3;
    node = ir_free_records ? reuse_frontend_record(size_class) : 0;
    if (!node) {
        allocation_bytes = (size_class + 1) * 16;
        arena = 1;
        if (!arena) node = allocate_permanent(allocation_bytes);
        else {
            if (arena == -1) arena = 0;
            pool = &compiler_arena.pools[arena];
            if (pool->remaining < (int)allocation_bytes) refill_arena(&compiler_arena, pool);
            node = (FrontendRecord *)pool->next;
            pool->remaining -= allocation_bytes;
            pool->next += allocation_bytes;
        }
    }
    /* Keep the inlined word/byte copy and its forward copy order. */
    for (index = 0; index < bytes / 4; ++index)
        ((unsigned *)node)[index] = ((unsigned *)record)[index];
    for (index *= 4; index < bytes; ++index)
        ((unsigned char *)node)[index] = ((unsigned char *)record)[index];
    if (!ir_record_tail) ir_record_head = node;
    else ir_record_tail->next = node;
    node->previous = ir_record_tail;
    ir_record_tail = node;
    node->flags = (node->flags & ~0x30) | (size_class << 4);
    ir_record_tail->sequence = ir_record_sequence;
    ir_record_tail->next = 0;
    ++ir_record_sequence;
}

/* Original 0x3d830..0x3d869. A scope opener needs only the common header. */
void begin_frontend_scope(void)
{
    FrontendRecord record;
    record.kind = 2;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
}

/* Original 0x3da00..0x3dabd. Parameters of a class requiring nontrivial
   destruction receive a cleanup record; pointer/scalar parameters do not. */
void __stdcall record_parameter_cleanup(CppSymbol *parameter)
{
    CppSymbol *tag;
    SymbolFrontendRecord record;
    if (!is_unqualified_class_type(parameter->type)) return;
    tag = cpp_context_stack->items[parameter->type->field08];
    if (!tag || (tag->kind & 31) != 4 || !(tag->declaration_flags & 0x2000) ||
        (tag->detail.class_info->trivial_special_members & 8)) return;
    record.header.kind = 5;
    record.header.flags = 0;
    record.header.field02 = 0;
    record.header.sequence = 0;
    record.header.next = 0;
    record.header.previous = 0;
    record.symbol = parameter;
    append_frontend_record(&record.header, sizeof(record));
}

/* Original 0x3d870..0x3d8c8. Cancel an empty opener; otherwise append the
   matching close record to retain a nonempty cleanup scope. */
void end_frontend_scope(void)
{
    FrontendRecord record;
    if (ir_record_tail && (ir_record_tail->kind & 0x3f) == 2) {
        remove_last_frontend_record();
        return;
    }
    record.kind = 3;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
}

/* Original 0x40d60..0x40dd3. Move the nonempty active list's tail to the
   reusable-record list and undo its sequence-number allocation. */
void remove_last_frontend_record(void)
{
    FrontendRecord *previous = ir_record_tail->previous;
    ir_record_tail->previous = ir_free_records;
    ir_record_tail->next = 0;
    if (ir_free_records) ir_free_records->next = ir_record_tail;
    ir_free_records = ir_record_tail;
    if (!previous) ir_record_head = ir_record_tail = 0;
    else {
        ir_record_tail = previous;
        ir_record_tail->next = 0;
    }
    --ir_record_sequence;
}

/* Original 0x3e1d0..0x3e297. Move trailing plain kind-13 symbol records
   into a source-order list owned by the current expression. A preceding
   kind-0x3a marker is consumed even when no symbols were moved. */
ExpressionSymbolList *detach_expression_symbols(void)
{
    ExpressionSymbolList *symbols = 0, *entry;
    SymbolFrontendRecord *record;
    ArenaPool *pool;
    if (ir_records_suppressed) return 0;
    while (ir_record_tail && ir_record_tail->kind == 13) {
        record = (SymbolFrontendRecord *)ir_record_tail;
        if (token_allocation_flags & 0x20000020UL)
            entry = allocate_permanent(sizeof(ExpressionSymbolList));
        else {
            pool = &compiler_arena.pools[1];
            if (pool->remaining < (int)sizeof(ExpressionSymbolList))
                refill_arena(&compiler_arena, pool);
            entry = (ExpressionSymbolList *)pool->next;
            pool->remaining -= sizeof(ExpressionSymbolList);
            pool->next += sizeof(ExpressionSymbolList);
        }
        entry->next = symbols;
        entry->symbol = record->symbol;
        symbols = entry;
        remove_last_frontend_record();
    }
    if (ir_record_tail && (ir_record_tail->kind & 0x3f) == 0x3a)
        remove_last_frontend_record();
    return symbols;
}

/* Original 0x3e610..0x3e6af. Insert the compound marker after the saved
   function boundary, then restore the previously active list tail. */
void record_compound_scope(void)
{
    FrontendRecord record;
    FrontendRecord *following, *previous;
    record.kind = 15;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    if (!ir_records_suppressed) {
        append_frontend_record(&record, sizeof(record));
        if (function_ir_tail) {
            following = function_ir_tail->next;
            previous = ir_record_tail->previous;
            ir_record_tail->next = following;
            following->previous = ir_record_tail;
            ir_record_tail->previous = function_ir_tail;
            function_ir_tail->next = ir_record_tail;
            previous->next = 0;
            ir_record_tail = previous;
            function_ir_tail = 0;
        }
    }
}

/* Original 0x40ad0..0x40b79. Free records are grouped in sixteen-byte
   size classes; retain a larger block's unused suffix as a new free record. */
FrontendRecord *__stdcall reuse_frontend_record(unsigned char size_class)
{
    FrontendRecord *record = ir_free_records, *remainder;
    while (record && ((record->flags & 0x30) >> 4) < size_class)
        record = record->previous;
    if (record) {
        if (record->previous) record->previous->next = record->next;
        if (record->next) record->next->previous = record->previous;
        else ir_free_records = record->previous;
        if (size_class < ((record->flags & 0x30) >> 4)) {
            remainder = (FrontendRecord *)((char *)record + (size_class + 1) * 16);
            remainder->flags = (remainder->flags & ~0x30) |
                ((record->flags - ((size_class + 1) << 4)) & 0x30);
            remainder->next = 0;
            remainder->previous = ir_free_records;
            if (ir_free_records) ir_free_records->next = remainder;
            ir_free_records = remainder;
        }
    }
    return record;
}

/* Original 0x42a60..0x42a91. Share the same initializer context used
   by expression preparation and member-constructor processing. */
void __stdcall mark_initialized_symbol(CppSymbol *symbol)
{
    initializing_symbol = symbol;
    if (symbol && symbol_has_reference_shape(symbol)) initializing_reference = 1;
    else initializing_reference = 0;
}

/* Original 0x3e6b0..0x3e779. Cancel an empty compound opener;
   other scopes retain a close marker and process non-control records. */
void end_compound_scope_record(void)
{
    FrontendRecord marker;
    unsigned char kind;
    if (ir_records_suppressed) return;
    if (ir_record_tail && (ir_record_tail->kind & 0x3f) == 15) {
        remove_last_frontend_record();
        return;
    }
    kind = ir_record_tail->kind & 0x3f;
    marker.kind = 16;
    marker.flags = 0;
    marker.field02 = 0;
    marker.sequence = 0;
    marker.next = 0;
    marker.previous = 0;
    append_frontend_record(&marker, sizeof(marker));
    if (kind != 9 && kind != 11 && (kind < 0x36 || kind > 0x39))
        emit_nonnull_expression_statement(process_expression_records(0));
}

/* Original 0x3e380..0x3e3ba: start a return-cleanup region. */
void begin_return_expression(void)
{
    FrontendRecord record;
    record.kind = 8;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
}

/* Original 0x3dac0..0x3dc2f. End a declaration initializer, retaining a
   symbol record when construction or automatic destruction needs bookkeeping. */
void __stdcall finish_object_initializer(CppSymbol *symbol)
{
    SymbolFrontendRecord record;
    CppSymbol *tag;
    unsigned char storage;
    record.header.kind = 14;
    record.header.flags = 0;
    record.header.field02 = 0;
    record.header.sequence = 0;
    record.header.next = 0;
    record.header.previous = 0;
    record.symbol = 0;
    if (ir_records_suppressed) return;
    if (symbol) {
        record.header.kind |= (unsigned char)((symbol->attributes & 4) << 4);
        if ((classify_incomplete_type(symbol->type) & 0x82) &&
            (tag = cpp_context_stack->items[symbol->type->field08]) != 0 &&
            (tag->kind & 31) == 4) {
            if ((symbol->attributes & 4) || !(tag->detail.class_info->trivial_special_members & 1))
                record.header.kind |= 0x40;
            storage = symbol->storage[0] & 7;
            if ((tag->declaration_flags & 0x2000) &&
                !(tag->detail.class_info->trivial_special_members & 8) &&
                (storage == 4 || storage == 5 || storage == 6)) record.header.kind |= 0x80;
        }
    }
    if (ir_record_tail && (ir_record_tail->kind & 0x3f) == 13)
        emit_nonnull_expression_statement(prepare_expression(0, 2));
    if (record.header.kind & 0xc0) {
        record.symbol = symbol;
        append_frontend_record(&record.header, sizeof(record));
    }
    initializing_symbol = 0;
    initializing_reference = 0;
    initializer_list_started = 0;
}

/* Original 0x3f070..0x3f0b2. Unevaluated contexts suppress the boundary
   that separates conditional and short-circuit expression record regions. */
void mark_evaluation_boundary(void)
{
    FrontendRecord record;
    if (allow_incomplete_class) return;
    record.kind = 0x3a;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
}
