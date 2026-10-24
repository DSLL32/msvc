#include "statement-scopes.h"
#include "expression-preparation.h"
/* Linked records assembled before frontend expression output. */
#include "frontend-records.h"
#include "parameters.h"
#include "parser.h"
#include "record-processing.h"
#include "cpp-context.h"
#include "hash-table.h"
#include "enumerators.h"
#include <setjmp.h>
#include <recon.h>

void begin_do_body_region(void);
void end_do_body_region(void);
void end_do_condition_region(void);
void begin_for_increment_region(void);
void end_for_increment_region(void);
void end_for_body_region(void);
void begin_switch_condition_region(void);
void end_switch_condition_region(void);
void end_switch_body_region(void);
void __stdcall check_switch_case_initialization(int is_case);

FrontendRecord *ir_record_head; /* original 0xa0a00 */
FrontendRecord *ir_record_tail; /* original 0xa0a04 */
/* Saved old tail during insertion, then the new function's tail after
   moving that suffix ahead of the earlier records. */
FrontendRecord *function_ir_tail; /* original 0xa0a5c */
int ir_function_error_count; /* original 0xa0a60 */
int ir_records_suppressed; /* original 0xa0a64 */

/* Original 0x3f7d0..0x3f809, cdecl. Condition temporaries have an
   independent paired region even when the current loop has no body yet. */
void begin_while_condition_region(void)
{
    FrontendRecord record;
    record.kind = 0x1a;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
}

/* Original 0x3f810..0x3f868, cdecl. Collapse an empty condition
   region; keep a closing delimiter when evaluation created cleanup records. */
void end_while_condition_region(void)
{
    FrontendRecord record;
    if (ir_record_tail && (ir_record_tail->kind & 0x3f) == 0x1a) {
        remove_last_frontend_record();
        return;
    }
    record.kind = 0x1b;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
}

/* Original 0x3f870..0x3f8b7, cdecl. Complete condition temporaries
   before opening the loop body; suppressed record output does neither. */
void begin_while_body(void)
{
    FrontendRecord record;
    if (ir_records_suppressed) return;
    end_while_condition_region();
    record.kind = 0x1c;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
}

/* Original 0x3f8c0..0x3f925, cdecl. Collapse an empty body or
   close its cleanup sequence before emitting the remaining expression. */
void end_while_body(void)
{
    FrontendRecord record;
    if (ir_record_tail && (ir_record_tail->kind & 0x3f) == 0x1c) {
        remove_last_frontend_record();
        return;
    }
    record.kind = 0x1d;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
    emit_nonnull_expression_statement(process_expression_records(0));
}

/* Original 0x3f930..0x3fa5b, cdecl. Close the body and keep its
   condition-evaluation chain behind the loop's closing record. Empty loops
   disappear; constant conditions become flags on the opening loop record. */
void end_while_statement(void)
{
    FrontendRecord record, *closing, *condition, *value;
    LoopFrontendRecord *loop;
    unsigned char kind;
    jmp_buf recovery;
    if (ir_records_suppressed) return;
    end_while_body();
    if (ir_record_tail && (ir_record_tail->kind & 0x3f) == 0x19) {
        remove_last_frontend_record();
        return;
    }
    if (syntax_diagnostic_count && !ir_record_error_context) {
        if (ir_records_suppressed) return;
        ir_record_error_context = recovery;
        if (setjmp(recovery)) {
            ir_record_error_context = 0;
            return;
        }
    }
    record.kind = 0x1e;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
    closing = ir_record_tail;
    loop = (LoopFrontendRecord *)find_frontend_record(closing->previous, 0x19, 1);
    condition = loop->header.next;
    if ((condition->kind & 0x3f) == 0x1a) {
        value = condition->next;
        kind = value->kind & 0x3f;
        if (kind == 0x3b || kind == 0x3c) {
            loop->header.flags |= kind == 0x3b ? 4 : 8;
            remove_frontend_record(value);
        }
    }
    loop->condition_records = loop->header.next;
    loop->header.next = closing;
    closing->previous = &loop->header;
    if (ir_record_error_context == recovery) ir_record_error_context = 0;
}

/* Original 0x3f770..0x3f7cd, cdecl. Begin a while-loop record before
   its separate condition-evaluation region. All loop flags start clear. */
void begin_while_condition(void)
{
    LoopFrontendRecord record;
    record.header.kind = 0x19;
    record.header.flags = 0;
    record.header.field02 = 0;
    record.header.sequence = 0;
    record.header.next = 0;
    record.header.previous = 0;
    record.condition_records = 0;
    if (ir_records_suppressed) return;
    append_frontend_record(&record.header, sizeof(record));
    begin_while_condition_region();
}

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

/* Original 0x3f2c0..0x3f3bd, stdcall4. Empty then regions collapse;
   occupied regions retain their goto-count delta and cleanup delimiter. */
void __stdcall end_if_statement(int complete)
{
    FrontendRecord record;
    ConditionalFrontendRecord *conditional;
    BranchBoundaryRecord *branch;
    jmp_buf recovery;
    if (syntax_diagnostic_count && !ir_record_error_context) {
        if (ir_records_suppressed) return;
        ir_record_error_context = recovery;
        if (setjmp(recovery)) {
            ir_record_error_context = 0;
            return;
        }
    }
    if (ir_record_tail && (ir_record_tail->kind & 0x3f) == 20) {
        conditional = (ConditionalFrontendRecord *)find_frontend_record(ir_record_tail, 17, 1);
        conditional->then_record = 0;
        remove_last_frontend_record();
    } else {
        branch = (BranchBoundaryRecord *)find_frontend_record(ir_record_tail, 20, 1);
        branch->goto_count -= ir_goto_count;
        record.kind = 21;
        record.flags = 0;
        record.field02 = 0;
        record.sequence = 0;
        record.next = 0;
        record.previous = 0;
        append_frontend_record(&record, sizeof(record));
        emit_nonnull_expression_statement(process_expression_records(0));
    }
    if (complete) close_if_records();
    if (ir_record_error_context == recovery) ir_record_error_context = 0;
}

/* Original 0x3f570..0x3f76f, cdecl. Fold empty condition records and
   detach statement-region chains behind the completed conditional node. */
void close_if_records(void)
{
    FrontendRecord record, *closing, *condition_end, *then_end, *last;
    ConditionalFrontendRecord *conditional;
    FrontendRecord *then_record, *else_record;
    unsigned char kind;
    jmp_buf recovery;
    if (ir_record_tail && (ir_record_tail->kind & 0x3f) == 17) {
        remove_last_frontend_record();
        return;
    }
    if (ir_record_tail && (ir_record_tail->kind & 0x3f) == 19) {
        remove_last_frontend_record();
        if (ir_record_tail && ((ir_record_tail->kind & 0x3f) == 0x3b ||
            (ir_record_tail->kind & 0x3f) == 0x3c))
            remove_last_frontend_record();
        if (ir_record_tail && (ir_record_tail->kind & 0x3f) == 18) {
            remove_last_frontend_record();
            remove_last_frontend_record();
            return;
        }
        remove_matching_frontend_record(17);
        remove_matching_frontend_record(18);
        return;
    }
    if (syntax_diagnostic_count && !ir_record_error_context) {
        if (ir_records_suppressed) return;
        ir_record_error_context = recovery;
        if (setjmp(recovery)) {
            ir_record_error_context = 0;
            return;
        }
    }
    record.kind = 24;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
    closing = ir_record_tail;
    conditional = (ConditionalFrontendRecord *)find_frontend_record(closing->previous, 17, 1);
    then_record = conditional->then_record;
    else_record = conditional->else_record;
    if ((conditional->header.next->kind & 0x3f) == 18) {
        condition_end = find_forward_frontend_record(conditional->header.next->next, 19, 1);
        kind = condition_end->previous->kind & 0x3f;
        if (kind == 0x3b || kind == 0x3c) {
            conditional->header.flags |= kind == 0x3b ? 4 : 8;
            remove_frontend_record(condition_end->previous);
        }
        condition_end->next = closing;
        last = condition_end;
    } else {
        conditional->header.next = closing;
        last = &conditional->header;
    }
    closing->previous = last;
    if (then_record && else_record) {
        then_end = find_forward_frontend_record(then_record->next, 21, 1);
        then_end->next = closing;
        else_record->previous = then_record->previous;
    }
    if (ir_record_error_context == recovery) ir_record_error_context = 0;
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

int __stdcall temporary_reference_matches(IrToken *object, unsigned index);
int __stdcall temporary_cleanup_is_suppressed(IrToken *object, unsigned index);
/* Original 0x3de50..0x3debb, stdcall 4. Search only the current
   expression's temporary-cleanup suffix and remove its matching index. */
void __stdcall cancel_temporary_cleanup(unsigned index)
{
    FrontendRecord *record;
    IrToken *object;
    unsigned char kind;
    if (ir_records_suppressed || allow_incomplete_class) return;
    for (record = ir_record_tail; record; record = record->previous) {
        kind = record->kind & 0x3f;
        if (kind != 13 && kind != 0x3a) return;
        if (kind != 13 || (record->kind & 0x40)) continue;
        object = ((TemporaryFrontendRecord *)record)->object;
        if (!(record->kind & 0x80)) {
            if (temporary_reference_matches(object, index)) {
                remove_frontend_record(record);
                return;
            }
        } else if (temporary_cleanup_is_suppressed(object, index)) return;
    }
}

/* Original 0x3dec0..0x3defc, stdcall 8. A cast wrapper does not
   change the numeric identity of the temporary it encloses. */
int __stdcall temporary_reference_matches(IrToken *object, unsigned index)
{
    if (allow_incomplete_class) return 0;
    if ((object->flags & 7) == 1 && object->kind == 0x6e)
        object = object->value.pointer;
    return object->value.integer == index ? 1 : 0;
}

/* Original 0x40e50..0x40ead, stdcall 4. Detached records enter the
   same doubly linked free list used by tail removal. */
void __stdcall remove_frontend_record(FrontendRecord *record)
{
    FrontendRecord *next = record->next;
    if (!next) {
        remove_last_frontend_record();
        return;
    }
    if (!record->previous) {
        /* The original updates both globals on this head-removal path. */
        ir_record_tail = next;
        ir_record_head = next;
    } else record->previous->next = next;
    record->next->previous = record->previous;
    record->previous = ir_free_records;
    record->next = 0;
    if (ir_free_records) ir_free_records->next = record;
    ir_free_records = record;
}

/* Original 0x3fd60..0x3fdbd. A for-loop opens its loop record before
   the independently paired condition-evaluation region. */
void begin_for_condition(void)
{
    LoopFrontendRecord record;
    record.header.kind = 0x25;
    record.header.flags = 0;
    record.header.field02 = 0;
    record.header.sequence = 0;
    record.header.next = 0;
    record.header.previous = 0;
    record.condition_records = 0;
    if (!ir_records_suppressed) {
        append_frontend_record(&record.header, sizeof(record));
        begin_for_condition_region();
    }
}

/* Original 0x3fdc0..0x3fdf9. The condition evaluation has its own
   sixteen-byte paired delimiter, independent of the outer loop record. */
void begin_for_condition_region(void)
{
    FrontendRecord record;
    record.kind = 0x26;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
}

/* Original 0x3fe00..0x3fe6b. Drop an empty condition region;
   otherwise close it before opening the increment evaluation region. */
void begin_for_increment(void)
{
    FrontendRecord record;
    if (ir_records_suppressed) return;
    if (ir_record_tail && (ir_record_tail->kind & 0x3f) == 0x26)
        remove_last_frontend_record();
    else {
        record.kind = 0x27;
        record.flags = 0;
        record.field02 = 0;
        record.sequence = 0;
        record.next = 0;
        record.previous = 0;
        append_frontend_record(&record, sizeof(record));
    }
    begin_for_increment_region();
}

/* Original 0x3fe70..0x3fea9. Open the increment's paired evaluation
   region with a fully defined sixteen-byte frontend record. */
void begin_for_increment_region(void)
{
    FrontendRecord record;
    record.kind = 0x28;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
}

/* Original 0x3ff10..0x3ff57. Close increment evaluation before opening
   the for-loop body; record suppression skips both operations. */
void begin_for_body(void)
{
    FrontendRecord record;
    if (ir_records_suppressed) return;
    end_for_increment_region();
    record.kind = 0x2a;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
}

/* Original 0x3feb0..0x3ff08. An empty increment region disappears;
   a nonempty one keeps its closing delimiter before the loop body. */
void end_for_increment_region(void)
{
    FrontendRecord record;
    if (ir_record_tail && (ir_record_tail->kind & 0x3f) == 0x28) {
        remove_last_frontend_record();
        return;
    }
    record.kind = 0x29;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
}

/* Original 0x40220..0x40289. Link a switch opener into the nesting
   chain, then start its independently paired condition-evaluation region. */
void begin_switch_condition(void)
{
    SwitchFrontendRecord record;
    record.header.kind = 0x2d;
    record.header.flags = 0;
    record.header.field02 = 0;
    record.header.sequence = 0;
    record.header.next = 0;
    record.header.previous = 0;
    record.outer_switch = 0;
    record.case_records = 0;
    record.constant_condition = 0;
    record.enum_tag = 0;
    if (ir_records_suppressed) return;
    record.outer_switch = ir_switch_records;
    append_frontend_record(&record.header, sizeof(record));
    ir_switch_records = ir_record_tail;
    begin_switch_condition_region();
}

/* Original 0x40290..0x402c9. The switch condition's evaluation
   region is a fully defined sixteen-byte paired delimiter. */
void begin_switch_condition_region(void)
{
    FrontendRecord record;
    record.kind = 0x2e;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
}

/* Original 0x40360..0x403f7, stdcall4. Open the switch body after
   condition cleanup, retaining an enum tag for later case diagnostics. */
void __stdcall begin_switch_body_region(CType *type)
{
    FrontendRecord record;
    SwitchFrontendRecord *opening;
    if (ir_records_suppressed) return;
    end_switch_condition_region();
    record.kind = 0x30;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
    if (type && (type->flags & 0x200) && !type->related &&
        cpp_context_stack->items[type->field08]) {
        opening = (SwitchFrontendRecord *)find_frontend_record(ir_record_tail, 0x2d, 1);
        opening->header.flags |= 4;
        opening->enum_tag = (CppSymbol *)cpp_context_stack->items[type->field08];
    }
}

/* Original 0x402d0..0x4035c. Fold an available constant into the
   switch opener and discard an empty condition-evaluation region. */
void end_switch_condition_region(void)
{
    FrontendRecord record;
    SwitchFrontendRecord *opening;
    if (ir_record_tail && (ir_record_tail->kind & 0x3f) == 0x3d) {
        opening = (SwitchFrontendRecord *)ir_record_tail->previous->previous;
        opening->constant_condition = ((ConstantFrontendRecord *)ir_record_tail)->value;
        opening->header.kind |= 0x80;
        remove_last_frontend_record();
    }
    if (ir_record_tail && (ir_record_tail->kind & 0x3f) == 0x2e) {
        remove_last_frontend_record();
        return;
    }
    record.kind = 0x2f;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
}

/* Original 0x40400..0x40534, stdcall4. Retain case/default source
   records in their switch's reverse label list, with existing error recovery. */
void __stdcall record_case_label(IrToken *expression)
{
    jmp_buf recovery;
    SwitchLabelFrontendRecord default_record, *label;
    CaseFrontendRecord case_record;
    SwitchFrontendRecord *opening;
    if (syntax_diagnostic_count && !ir_record_error_context) {
        if (ir_records_suppressed) return;
        ir_record_error_context = recovery;
        if (setjmp(recovery)) {
            ir_record_error_context = 0;
            return;
        }
    }
    check_switch_case_initialization(expression != 0);
    if (!expression) {
        default_record.header.kind = 0x34;
        default_record.header.flags = 0;
        default_record.header.field02 = 0;
        default_record.header.sequence = 0;
        default_record.header.next = 0;
        default_record.header.previous = 0;
        default_record.next_label = 0;
        if (ir_switch_records) ir_switch_records->kind |= 0x40;
        append_frontend_record(&default_record.header, sizeof(default_record));
    } else {
        case_record.label.header.kind = 0x33;
        case_record.label.header.flags = 0;
        case_record.label.header.field02 = 0;
        case_record.label.header.sequence = 0;
        case_record.label.header.next = 0;
        case_record.label.header.previous = 0;
        case_record.label.next_label = 0;
        case_record.value = expression->value.integer;
        append_frontend_record(&case_record.label.header, sizeof(case_record));
    }
    if (ir_switch_records) {
        opening = (SwitchFrontendRecord *)ir_switch_records;
        label = (SwitchLabelFrontendRecord *)ir_record_tail;
        label->next_label = opening->case_records;
        opening->case_records = label;
    }
    if (ir_record_error_context == recovery) ir_record_error_context = 0;
}

/* Original 0x41b40..0x41bbe, stdcall4. A new case/default cannot
   bypass a marked initialization between it and the switch body opener. */
void __stdcall check_switch_case_initialization(int is_case)
{
    FrontendRecord *record = ir_record_tail;
    unsigned char kind;
    while ((record->kind & 0x3f) != 0x30) {
        kind = record->kind & 0x3f;
        if (kind == 0x0e) {
            if (record->kind & 0x40)
                report_syntax_error(is_case ? 360 : 361,
                    format_symbol_name(((SymbolFrontendRecord *)record)->symbol));
        } else if (kind == 0x10)
            record = find_frontend_record(record->previous, 0x0f, 1);
        record = record->previous;
    }
}

/* Original 0x3fa60..0x3fabd. A do-loop opens its loop record
   before the independently paired body-evaluation region. */
void begin_do_statement(void)
{
    LoopFrontendRecord record;
    record.header.kind = 0x1f;
    record.header.flags = 0;
    record.header.field02 = 0;
    record.header.sequence = 0;
    record.header.next = 0;
    record.header.previous = 0;
    record.condition_records = 0;
    if (!ir_records_suppressed) {
        append_frontend_record(&record.header, sizeof(record));
        begin_do_body_region();
    }
}

/* Original 0x3fac0..0x3faf9. Open a fully initialized sixteen-byte
   body delimiter after the do-loop's outer record. */
void begin_do_body_region(void)
{
    FrontendRecord record;
    record.kind = 0x20;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
}

/* Original 0x3fb70..0x3fbb7. Complete the do-body's pending
   evaluation before opening the condition region. */
void begin_do_condition(void)
{
    FrontendRecord record;
    if (ir_records_suppressed) return;
    end_do_body_region();
    record.kind = 0x22;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
}

/* Original 0x3fb00..0x3fb65. Collapse an empty do-body region,
   or close its evaluation sequence and emit the remaining expression. */
void end_do_body_region(void)
{
    FrontendRecord record;
    if (ir_record_tail && (ir_record_tail->kind & 0x3f) == 0x20) {
        remove_last_frontend_record();
        return;
    }
    record.kind = 0x21;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
    emit_nonnull_expression_statement(process_expression_records(0));
}

/* Original 0x3fc20..0x3fd51. Close the do-loop and detach its
   evaluation chain, retaining a known constant condition on the opener. */
void end_do_statement(void)
{
    FrontendRecord record, *closing, *condition, *value;
    LoopFrontendRecord *loop;
    unsigned char kind;
    jmp_buf recovery;
    if (ir_records_suppressed) return;
    end_do_condition_region();
    if (ir_record_tail && (ir_record_tail->kind & 0x3f) == 0x1f) {
        remove_last_frontend_record();
        return;
    }
    if (syntax_diagnostic_count && !ir_record_error_context) {
        if (ir_records_suppressed) return;
        ir_record_error_context = recovery;
        if (setjmp(recovery)) {
            ir_record_error_context = 0;
            return;
        }
    }
    record.kind = 0x24;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
    closing = ir_record_tail;
    loop = (LoopFrontendRecord *)find_frontend_record(closing->previous, 0x1f, 1);
    condition = closing->previous;
    if ((condition->kind & 0x3f) == 0x23) {
        value = condition->previous;
        kind = value->kind & 0x3f;
        if (kind == 0x3b || kind == 0x3c) {
            loop->header.flags |= kind == 0x3b ? 4 : 8;
            remove_frontend_record(value);
        }
    }
    loop->condition_records = loop->header.next;
    loop->header.next = closing;
    closing->previous = &loop->header;
    if (ir_record_error_context == recovery) ir_record_error_context = 0;
}

/* Original 0x3fbc0..0x3fc18. Discard an empty do-condition region
   or retain its fully initialized closing delimiter. */
void end_do_condition_region(void)
{
    FrontendRecord record;
    if (ir_record_tail && (ir_record_tail->kind & 0x3f) == 0x22) {
        remove_last_frontend_record();
        return;
    }
    record.kind = 0x23;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
}

/* Original 0x40a60..0x40acd. Keep a break's cleanup boundary unless
   the preceding frontend record already terminates this sequence. */
void record_break_statement(void)
{
    FrontendRecord record;
    unsigned char kind;
    if (ir_records_suppressed) return;
    kind = ir_record_tail->kind & 0x3f;
    if (kind == 9 || kind == 11 || (kind >= 0x36 && kind <= 0x39)) return;
    record.kind = 0x39;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
    emit_nonnull_expression_statement(process_expression_records(0));
}

/* Original 0x409f0..0x40a5d. A continue has its own cleanup
   boundary, with the same preceding-termination suppression as break. */
void record_continue_statement(void)
{
    FrontendRecord record;
    unsigned char kind;
    if (ir_records_suppressed) return;
    kind = ir_record_tail->kind & 0x3f;
    if (kind == 9 || kind == 11 || (kind >= 0x36 && kind <= 0x39)) return;
    record.kind = 0x38;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
    emit_nonnull_expression_statement(process_expression_records(0));
}

/* Original 0x40540..0x40547. A null case expression selects the
   common default-label record path. */
void record_default_label(void)
{
    record_case_label(0);
}

/* Original 0x408e0..0x409ea, stdcall8. A known label can emit its
   cleanup now; a forward goto retains a separate cleanup-label record. */
void __stdcall record_goto(CppSymbol *label, int known)
{
    SymbolFrontendRecord direct;
    GotoFrontendRecord forward;
    unsigned char kind;
    if (ir_records_suppressed) return;
    kind = ir_record_tail->kind & 0x3f;
    if (kind == 9 || kind == 11 || (kind >= 0x36 && kind <= 0x39)) return;
    ++ir_goto_count;
    if (known && !find_label_record(label)) known = 0;
    if (known) {
        direct.header.kind = 0x36;
        direct.header.flags = 0;
        direct.header.field02 = 0;
        direct.header.sequence = 0;
        direct.header.next = 0;
        direct.header.previous = 0;
        direct.symbol = label;
        append_frontend_record(&direct.header, sizeof(direct));
        emit_nonnull_expression_statement(process_expression_records(0));
        return;
    }
    forward.header.kind = 0x37;
    forward.header.flags = 0;
    forward.header.field02 = 0;
    forward.header.sequence = 0;
    forward.header.next = 0;
    forward.header.previous = 0;
    forward.label = label;
    forward.cleanup_label = new_branch_label(0x47);
    forward.next_goto = (GotoFrontendRecord *)ir_goto_records;
    append_frontend_record(&forward.header, sizeof(forward));
    ir_goto_records = ir_record_tail;
    write_ir_record(0x3a, forward.cleanup_label);
}

/* Original 0x405c0..0x4085f. Finish switch nesting, restore source
   order to its label records, and diagnose enum values missing from cases. */
void end_switch_statement(void)
{
    FrontendRecord record, *closing;
    SwitchFrontendRecord *opening;
    SwitchLabelFrontendRecord *label, *previous;
    HashIterator iterator;
    EnumeratorSymbol *enumerator;
    CppSymbol *tag;
    char *tag_name, *enumerator_name;
    int found;
    if (ir_records_suppressed) return;
    end_switch_body_region();
    if (ir_record_tail && (ir_record_tail->kind & 0x3f) == 0x2d) {
        ir_switch_records = ((SwitchFrontendRecord *)ir_switch_records)->outer_switch;
        remove_last_frontend_record();
        compiler_warning(3, 60);
        return;
    }
    if (ir_record_tail && (ir_record_tail->kind & 0x3f) == 0x2f) {
        remove_last_frontend_record();
        remove_matching_frontend_record(0x2e);
        remove_matching_frontend_record(0x2d);
        ir_switch_records = ((SwitchFrontendRecord *)ir_switch_records)->outer_switch;
        if (ir_record_tail && (ir_record_tail->kind & 0x3f) == 0x3d)
            remove_last_frontend_record();
        compiler_warning(3, 60);
        return;
    }
    record.kind = 0x32;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
    opening = (SwitchFrontendRecord *)ir_switch_records;
    closing = ir_record_tail;
    ir_switch_records = opening->outer_switch;
    label = opening->case_records;
    if (label) {
        previous = 0;
        if (!label->next_label && (label->header.kind & 0x3f) == 0x34)
            compiler_warning(3, 65);
        while (label->next_label) {
            opening->case_records = label->next_label;
            label->next_label = previous;
            previous = label;
            label = opening->case_records;
        }
        label->next_label = previous;
    } else compiler_warning(3, 60);
    opening->evaluation_records = opening->header.next;
    opening->header.next = closing;
    closing->previous = &opening->header;
    if (!(opening->header.flags & 4)) return;
    tag = opening->enum_tag;
    if (!(tag->flags & 4)) {
        compiler_warning(3, 64, format_symbol_name(tag));
        return;
    }
    begin_hash_iteration(&iterator, tag->auxiliary.enumerators);
    while (next_hash_entry(&iterator)) {
        enumerator = (EnumeratorSymbol *)iterator.value;
        found = 0;
        label = opening->case_records;
        while (label && !found) {
            if ((label->header.kind & 0x3f) == 0x33 &&
                ((CaseFrontendRecord *)label)->value == enumerator->value) found = 1;
            label = label->next_label;
        }
        if (!found) {
            /* Retain the original formatting order: tag, then enumerator. */
            if (opening->header.kind & 0x40) {
                tag_name = format_symbol_name(tag);
                enumerator_name = format_symbol_name((CppSymbol *)enumerator);
                compiler_warning(4, 61, enumerator_name, tag_name);
            } else {
                tag_name = format_symbol_name(tag);
                enumerator_name = format_symbol_name((CppSymbol *)enumerator);
                compiler_warning(3, 62, enumerator_name, tag_name);
            }
        }
    }
    for (label = opening->case_records; label; label = label->next_label) {
        if ((label->header.kind & 0x3f) != 0x33) continue;
        found = 0;
        begin_hash_iteration(&iterator, tag->auxiliary.enumerators);
        while (!found && next_hash_entry(&iterator)) {
            enumerator = (EnumeratorSymbol *)iterator.value;
            if (((CaseFrontendRecord *)label)->value == enumerator->value) found = 1;
        }
        if (!found) {
            tag_name = format_symbol_name(tag);
            compiler_warning(3, 63, ((CaseFrontendRecord *)label)->value, tag_name);
        }
    }
}

/* Original 0x40550..0x405b5. Complete a nonempty switch body's
   pending evaluation; an empty opening delimiter simply disappears. */
void end_switch_body_region(void)
{
    FrontendRecord record;
    if (ir_record_tail && (ir_record_tail->kind & 0x3f) == 0x30) {
        remove_last_frontend_record();
        return;
    }
    record.kind = 0x31;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    append_frontend_record(&record, sizeof(record));
    emit_nonnull_expression_statement(process_expression_records(0));
}

/* Original 0x3ffd0..0x4021f. Detach a for-loop's evaluation chain,
   moving its increment after the body while preserving condition markers. */
void end_for_statement(void)
{
    FrontendRecord record, *closing, *condition, *increment, *body;
    FrontendRecord *condition_end, *body_end, *increment_end, *value;
    LoopFrontendRecord *loop;
    unsigned char kind;
    jmp_buf recovery;
    if (ir_records_suppressed) return;
    end_for_body_region();
    if (ir_record_tail && (ir_record_tail->kind & 0x3f) == 0x25) {
        remove_last_frontend_record();
        return;
    }
    condition = increment = body = 0;
    record.kind = 0x2c;
    record.flags = 0;
    record.field02 = 0;
    record.sequence = 0;
    record.next = 0;
    record.previous = 0;
    if (syntax_diagnostic_count && !ir_record_error_context) {
        if (ir_records_suppressed) return;
        ir_record_error_context = recovery;
        if (setjmp(recovery)) {
            ir_record_error_context = 0;
            return;
        }
    }
    append_frontend_record(&record, sizeof(record));
    closing = ir_record_tail;
    loop = (LoopFrontendRecord *)find_frontend_record(closing->previous, 0x25, 1);
    condition = find_forward_frontend_record(loop->header.next, 0x26, 0);
    increment = find_forward_frontend_record(loop->header.next, 0x28, 0);
    body = find_forward_frontend_record(loop->header.next, 0x2a, 0);
    if (condition) {
        loop->condition_records = condition;
        condition_end = find_forward_frontend_record(condition->next, 0x27, 1);
        value = condition_end->previous;
        kind = value->kind & 0x3f;
        if (kind == 0x3b || kind == 0x3c) {
            loop->header.flags |= kind == 0x3b ? 4 : 8;
            remove_frontend_record(value);
        }
        if (body && increment) {
            body_end = find_forward_frontend_record(body->next, 0x2b, 1);
            increment_end = find_forward_frontend_record(increment->next, 0x29, 1);
            condition_end->next = body;
            body->previous = condition_end;
            body_end->next = increment;
            increment->previous = body_end;
            increment_end->next = closing;
        }
    } else if (body) {
        loop->condition_records = body;
        if (increment) {
            body_end = find_forward_frontend_record(body->next, 0x2b, 1);
            increment_end = find_forward_frontend_record(increment->next, 0x29, 1);
            body->previous = &loop->header;
            body_end->next = increment;
            increment->previous = body_end;
            increment_end->next = closing;
        }
    } else loop->condition_records = increment;
    loop->header.next = closing;
    closing->previous = &loop->header;
    if (ir_record_error_context == recovery) ir_record_error_context = 0;
}
