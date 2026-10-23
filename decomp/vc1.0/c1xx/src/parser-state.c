/* Shared yacc state and its observed macro-workspace boundary aliases. */
#include "parser.h"
#include "parameter-scopes.h"
#include <recon.h>

ParserStackStorage parser_stacks; /* original 0x94e44..0x956e4 */
ParserValue parser_value; /* original 0x8c1f0..0x8c1f8 */
ParserValue current_token_value; /* original 0x8c150..0x8c158 */
int parser_lookahead = -1; /* original 0x9eb9c */
int parser_error_count; /* original 0x9eba0 */
/* Both native yacc diagnostic reporters increment this distinct word. */
unsigned syntax_diagnostic_count; /* original 0x914cc */
short parser_recovery_count; /* original 0x9eba4 */

ClassContext class_contexts[16]; /* original 0x8c1fc..0x8c3fc */
LiteralWorkspace literal_workspace; /* original 0x8c404..0x8cc0c */
unsigned char *text_work_cursor = expression_storage; /* original 0x9fc48 */
int initializer_list_started; /* original 0xa0a24 */
int declarator_nesting; /* original 0x91894 */
/* A parenthesized-declaration lookahead also addresses the preceding entry. */
DeclarationType declaration_spec_storage[61]; /* original 0x948fc..0x94ae4 */
unsigned enumerator_value = (unsigned)-1; /* original 0x9fc0c */
CType *enumerator_context; /* original 0x9f1d8 */
void *based_symbol; /* original 0x91164 */
unsigned array_bound; /* original 0x97bbc */
unsigned new_array_bound; /* original 0x97bc4 */
unsigned char member_lookup_operator; /* original 0x97bb4 */
IrToken *member_lookup_context; /* original 0x97bb8 */
LabelStack *control_labels, *break_labels, *continue_labels;
LabelStack *switch_labels, *default_labels;
/* The five original heads are 0x9155c, 0x91560, 0x91564, 0x91568, 0x9156c. */
int label_stack_count; /* original 0x91558 */
IrToken *default_label; /* original 0x91570 */

/* Original 0x32eb0..0x32fb2. Reuse an established declaration context when
   no additional specification is supplied; otherwise rebuild implicit int
   storage and merge the supplied constructor declaration's modifiers. */
DeclarationType *__stdcall current_declaration_type(DeclarationType *explicit_type)
{
    DeclarationType *context;
    StorageSpec storage;
    if (declaration_spec_flags[declaration_depth] & 1) {
        if (!explicit_type) return &declaration_spec_stack[declaration_depth];
        if (!(explicit_type->storage[0] & 0x10) &&
            explicit_type->type->field08 != class_contexts[current_class_context].type->field08)
            report_syntax_error(533, class_contexts[current_class_context].symbol->name);
        else {
            storage.flags = explicit_type->storage[0] | (explicit_type->storage[1] << 8);
            storage.extended = explicit_type->storage[2];
            check_constructor_storage(storage, class_contexts[current_class_context].symbol->name);
        }
    }
    context = &declaration_spec_stack[declaration_depth];
    context->type = type_0064;
    context->storage[0] = (declaration_storage[0] & ~7) | 0x30;
    context->storage[1] = declaration_storage[1];
    context->storage[2] = declaration_storage[2];
    if (explicit_type) {
        storage.flags = explicit_type->storage[0] | (explicit_type->storage[1] << 8);
        storage.extended = explicit_type->storage[2];
        merge_storage_spec((StorageSpec *)context->storage, storage);
    }
    declaration_spec_flags[declaration_depth] |= 1;
    return &declaration_spec_stack[declaration_depth];
}

/* Original 0x32e80..0x32ea4. Query the outermost declaration context while
   preserving the caller's depth and the helper's returned context pointer. */
DeclarationType *__stdcall global_declaration_type(DeclarationType *context)
{
    int saved_depth = declaration_depth;
    DeclarationType *result;
    declaration_depth = 0;
    result = current_declaration_type(context);
    declaration_depth = saved_depth;
    return result;
}

/* Original 0x26830..0x268c9. Complete one external definition, preserving
   error recovery's arena lifetime and any pending scanned-text replay. */
void __stdcall finish_external_definition(int recovering)
{
    if (!recovering) release_arena(2);
    text_work_cursor = expression_storage;
    parsed_parameter_symbols = 0;
    active_parameter_symbols = 0;
    function_declaration_kind = 0;
    active_function_declaration_kind = 0;
    token_allocation_flags &= ~0x03000000UL;
    cpp_parser_flags &= ~4UL;
    token_allocation_flags &= ~1UL;
    declaration_depth = 0;
    declaration_spec_flags[0] &= ~3;
    deferred_declarations[0] = 0;
    if (cpp_parser_flags & 8) flush_scanned_declaration(0);
}
