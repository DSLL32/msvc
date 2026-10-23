/* QuickC inline-region and loop-query bookkeeping. */
#include <stdlib.h>
#include "initialization.h"
#include "global-ir.h"
#include "parser.h"
#include "tokens.h"
#include "lexer.h"
#include "expressions.h"
#include <recon.h>

extern LabelStack *break_labels, *continue_labels, *do_labels;

/* Region reconciliation helpers retain separate original call boundaries. */
int __stdcall align_inline_slots(unsigned short first, unsigned short second);
int __stdcall describe_qc_operand(IrToken *expression, QcOperand *operand);
IrToken *__stdcall expand_qc_operand(QcOperand *operand);
void free_inline_query_tables(void);

int inline_regions_changed; /* original0x60470 */
FrontendSymbol *inline_unused_parameter; /* original0x6623c */
unsigned short inline_current_region; /* original0x60480 */
unsigned short inline_last_region; /* original0x60474 */
unsigned inline_jump_count; /* original0x60478 */
unsigned short inline_parent_region; /* original0x6047c */
unsigned char inline_saved_slot_count; /* original0x6046c */

/* Original0x2e370..0x2e375 is a tail transfer to the table allocator. */
void initialize_inline_queries(void)
{
    allocate_inline_query_tables();
}

/* Original0x2edef..0x2ee47. Both arrays are owned until query cleanup;
   every byte is cleared before any region/scope is consumed. */
void allocate_inline_query_tables(void)
{
    unsigned i;
    unsigned *words;
    inline_regions = malloc(255U * sizeof(InlineRegion));
    inline_scopes = malloc(8U * sizeof(InlineScope));
    if (!inline_regions || !inline_scopes) fatal_error(60);
    words = (unsigned *)inline_regions;
    for (i = 0; i < 255U * sizeof(InlineRegion) / sizeof(unsigned); ++i)
        words[i] = 0;
    words = (unsigned *)inline_scopes;
    for (i = 0; i < 8U * sizeof(InlineScope) / sizeof(unsigned); ++i)
        words[i] = 0;
}

/* Original0x2e375..0x2e401. Region zero is the root of each function's
   sibling/child tree. Only the encountered fields are reset here. */
void reset_inline_function_state(void)
{
    inline_current_region = 0;
    inline_last_region = 0;
    inline_region_index = 0;
    inline_base_region = 0;
    inline_scope_depth = -1;
    inline_slot_count = 0;
    inline_jump_count = 0;
    inline_regions[0].next_sibling = inline_regions[0].first_child = 0xffff;
    inline_regions[0].left_slot = inline_regions[0].right_slot = 0xff;
    ++inline_scope_depth;
    inline_scopes[inline_scope_depth].region = 0;
}

/* Original0x2eab7..0x2eb5c, one stack pointer/RET4. Only eligible
   small scalar parameters enter the parallel index/symbol tables. */
void __stdcall record_inline_parameter(FrontendSymbol *parameter)
{
    CType *type = parameter->type;
    int eligible;
    if (type->related) {
        if (parameter->attributes & 0x400) eligible = 0;
        else eligible = pointer_type_size(type->related->attributes) == 2;
    } else
        eligible = (type->flags & 0xf) == 2 && !(type->flags & 0x400);
    if (eligible && !(type->attributes & 0x2000) && inline_slot_count < 10 &&
        inline_scope_depth < 8 && inline_region_index < 255) {
        inline_parameters.indices[inline_slot_count] = parameter->index;
        inline_parameters.symbols[inline_slot_count] = parameter;
        write_ir_record(0x91, 0);
        ++inline_slot_count;
    }
}

/* Original0x2e419..0x2e603, one stack value/RET4. Region indices are
   serialized as sixteen-bit links; scope snapshots hold ten usage bytes. */
void __stdcall begin_inline_region(unsigned flags)
{
    InlineRegion *region, *parent;
    InlineScope *scope;
    unsigned index;
    ++inline_region_index;
    ++inline_scope_depth;
    if (inline_scope_depth < 8 && inline_region_index < 255) {
        scope = inline_scopes + inline_scope_depth;
        scope->region = inline_current_region;
        region = inline_regions + inline_region_index;
        region->parameter_count = inline_slot_count;
        inline_saved_slot_count = inline_slot_count;
        parent = inline_regions + inline_current_region;
        if (parent->first_child == 0xffff)
            parent->first_child = inline_region_index;
        else
            inline_regions[inline_last_region].next_sibling = inline_region_index;
        region->left_slot = region->right_slot = 0;
        region->left_symbol = region->right_symbol = 0;
        region->next_sibling = region->first_child = 0xffff;
        region->flags = (unsigned char)flags;
        inline_parent_region = inline_current_region;
        inline_current_region = inline_region_index;
        for (index = 0; index < inline_slot_count; ++index) {
            scope->parameter_attributes[index] = (unsigned char)inline_parameters.symbols[index]->references;
            inline_parameters.symbols[index]->references = 0;
        }
        scope->flags = 0;
        write_ir_record(0x93, 0);
    } else if (inline_region_index < 255) {
        region = inline_regions + inline_region_index;
        region->next_sibling = region->first_child = 0xffff;
        region->parameter_count = region->flags = region->left_slot = region->right_slot = 0xff;
        region->left_symbol = region->right_symbol = 0xffffffff;
    }
}

/* Original0x2e603..0x2eab7. Rank the two most-used parameter slots,
   then restore the enclosing scope's saturating usage counts. */
void finish_inline_region(void)
{
    InlineRegion *region;
    InlineScope *scope;
    FrontendSymbol *symbol;
    unsigned i, total;
    unsigned short maximum, threshold;
    int depth;
    if (inline_scope_depth >= 8 || inline_region_index >= 255) {
        --inline_region_index;
        --inline_scope_depth;
        return;
    }
    region = inline_regions + inline_current_region;
    scope = inline_scopes + inline_scope_depth;
    if (inline_jump_count) {
        region->left_slot = region->right_slot = 0xff;
        region->flags |= 6;
        for (i = 0; i < inline_slot_count; ++i) {
            symbol = inline_parameters.symbols[i];
            if (!scope->parameter_attributes[i] && symbol->references)
                symbol->references = 1;
            else symbol->references = scope->parameter_attributes[i];
        }
        inline_slot_count = 0;
    } else {
        region->flags |= scope->flags;
        if (!inline_unused_parameter)
            inline_unused_parameter = allocate_permanent(sizeof(FrontendSymbol));
        inline_unused_parameter->references = 0;
        for (i = 0; i < inline_slot_count; ++i) {
            if (inline_parameters.indices[i] == 0xffff) {
                total = 0;
                for (depth = inline_scope_depth; depth >= 0; --depth)
                    total += inline_scopes[depth].parameter_attributes[i];
                symbol = inline_parameters.symbols[i];
                if (!scope->parameter_attributes[i] && (symbol->references || total))
                    symbol->references = 1;
                else symbol->references = scope->parameter_attributes[i];
                inline_parameters.symbols[i] = inline_unused_parameter;
            }
        }
        if (region->left_slot == 0xff) {
            for (i = 0; i < inline_slot_count; ++i) {
                symbol = inline_parameters.symbols[i];
                if (!scope->parameter_attributes[i] && symbol->references)
                    symbol->references = 1;
                else symbol->references = scope->parameter_attributes[i];
            }
            inline_slot_count = 0;
        } else {
            region->left_slot = region->right_slot = 0xff;
            if (inline_slot_count) {
                region->left_slot = 0;
                threshold = region->first_child == 0xffff ? 2 : 8;
                maximum = 0;
                region->left_symbol = 0;
                for (i = 0; i < inline_slot_count; ++i) {
                    if (inline_parameters.symbols[i]->references > maximum) {
                        maximum = inline_parameters.symbols[i]->references;
                        region->left_slot = (unsigned char)i;
                        region->left_symbol = inline_parameters.indices[i];
                    }
                }
                if (maximum < threshold) {
                    region->left_slot = 0xff;
                    region->left_symbol = 0;
                } else if (inline_slot_count >= 2) {
                    maximum = 0;
                    for (i = 0; i < inline_slot_count; ++i) {
                        if (i != region->left_slot &&
                            inline_parameters.symbols[i]->references >= maximum) {
                            region->right_slot = (unsigned char)i;
                            region->right_symbol = inline_parameters.indices[i];
                            maximum = inline_parameters.symbols[i]->references;
                        }
                    }
                    if (maximum < threshold) {
                        region->right_slot = 0xff;
                        region->right_symbol = 0;
                    }
                }
            }
        }
    }
    for (i = 0; i < inline_slot_count; ++i) {
        symbol = inline_parameters.symbols[i];
        if (!scope->parameter_attributes[i] && symbol->references)
            symbol->references = 1;
        else symbol->references = scope->parameter_attributes[i];
    }
    if (region->left_slot != 0xff)
        inline_parameters.symbols[region->left_slot]->references += 4;
    if (region->right_slot != 0xff)
        inline_parameters.symbols[region->right_slot]->references += 4;
    write_ir_record(0x94, 0);
    inline_last_region = inline_current_region;
    inline_slot_count = region->parameter_count;
    inline_saved_slot_count = inline_regions[inline_parent_region].parameter_count;
    inline_current_region = scope->region;
    --inline_scope_depth;
    inline_parent_region = inline_scopes[inline_scope_depth].region;
}

/* Original0x2e401..0x2e414. */
void finish_inline_function(void)
{
    reconcile_inline_regions();
    write_global_record(0xf, function_symbol);
}

/* Original0x2eb5c..0x2eb9a. The bounded passes propagate compatible
   parameter selections through the function's region tree. */
void reconcile_inline_regions(void)
{
    int pass;
    for (pass = 0; pass <= 9; ++pass) {
        inline_regions_changed = 0;
        propagate_inline_regions(0);
        if (!inline_regions_changed) return;
    }
    if (inline_regions_changed) fatal_error(1, pch_diagnostic_name, 751);
}

/* Original0x2eb9a..0x2ed14, one sixteen-bit stack argument/RET4. */
void __stdcall propagate_inline_regions(unsigned short index)
{
    InlineRegion *region = inline_regions + index;
    InlineRegion *child;
    unsigned short child_index = region->first_child;
    unsigned slot;
    unsigned char *parent_slot, *child_slot;
    unsigned *parent_symbol, *child_symbol;
    while (child_index != 0xffff) {
        align_inline_slots(index, child_index);
        child = inline_regions + child_index;
        if (child->left_slot == 0xff) {
            child->left_slot = region->left_slot;
            child->left_symbol = region->left_symbol;
        }
        if (child->right_slot == 0xff) {
            child->right_slot = region->right_slot;
            child->right_symbol = region->right_symbol;
        }
        if (child->left_slot == child->right_slot) {
            child->right_slot = 0xff;
            child->right_symbol = 0;
        }
        if (child->next_sibling != 0xffff)
            align_inline_slots(child_index, child->next_sibling);
        propagate_inline_regions(child_index);
        if (index && !(region->flags & 2)) {
            for (slot = 0; slot < 2; ++slot) {
                if (!slot) {
                    parent_slot = &region->left_slot;
                    child_slot = &child->left_slot;
                    parent_symbol = &region->left_symbol;
                    child_symbol = &child->left_symbol;
                } else {
                    parent_slot = &region->right_slot;
                    child_slot = &child->right_slot;
                    parent_symbol = &region->right_symbol;
                    child_symbol = &child->right_symbol;
                }
                if (*parent_slot == 0xff && child->parameter_count > *child_slot) {
                    inline_regions_changed = 1;
                    *parent_slot = *child_slot;
                    *parent_symbol = *child_symbol;
                }
            }
        }
        child_index = child->next_sibling;
    }
}

/* Original0x2ed14..0x2ed95, two sixteen-bit stack values/RET8. */
int __stdcall align_inline_slots(unsigned short first, unsigned short second)
{
    InlineRegion *left = inline_regions + first;
    InlineRegion *right = inline_regions + second;
    unsigned char slot;
    unsigned symbol;
    if ((right->right_slot != 0xff && left->left_slot == right->right_slot) ||
        (right->left_slot != 0xff && left->right_slot == right->left_slot)) {
        slot = right->left_slot;
        right->left_slot = right->right_slot;
        right->right_slot = slot;
        symbol = right->left_symbol;
        right->left_symbol = right->right_symbol;
        right->right_symbol = symbol;
        return 0;
    }
    return -1;
}

/* Original0x2ee64..0x2ee90, three stack values/RET12. */
int __stdcall find_inline_parameter_slot(unsigned *slots, unsigned count, unsigned symbol_index)
{
    unsigned i;
    for (i = 0; i < count; ++i)
        if (slots[i] == symbol_index) return i;
    return -1;
}

/* Original0x2efb0..0x2f1a5. Preserve arena allocation even when this
   expression cannot use the compact QuickC loop representation. */
int __stdcall recognize_qc_loop(IrToken *condition, IrToken *increment, LabelStack *entry)
{
    ArenaPool *pool = &compiler_arena.pools[3];
    QcLoopState *loop;
    ExpressionNode *node;
    unsigned char operation, category;
    if (pool->remaining < (int)sizeof(QcLoopState)) refill_arena(&compiler_arena, pool);
    loop = (QcLoopState *)pool->next;
    pool->remaining -= sizeof(QcLoopState);
    pool->next += sizeof(QcLoopState);
    entry->value.qc_loop = loop;
    if (!condition) return 0;
    operation = condition->kind;
    category = token_categories[operation];
    node = (ExpressionNode *)condition;
    if (category == TOK_LOGICAL_NOT) {
        if (!describe_qc_operand(node->value.children.left, &loop->left)) return 0;
        loop->right.value.integer = 0;
        loop->right.operation = TOK_INT_CONSTANT;
        operation = TOK_EQUAL;
    } else if (category == TOK_CONSTANT) {
        if (condition->kind >= TOK_INT_CONSTANT && condition->kind <= TOK_ULONG_CONSTANT &&
            !condition->value.integer) {
            write_ir_record(0x3a, break_labels->value.label);
            return 0;
        }
        operation = 0;
    } else if (category == TOK_EQUALITY_OPERATOR || category == TOK_RELATIONAL_OPERATOR) {
        if (!describe_qc_operand(node->value.children.left, &loop->left)) return 0;
        if (!describe_qc_operand(node->value.children.right, &loop->right)) return 0;
    } else {
        if (!describe_qc_operand(condition, &loop->left)) return 0;
        loop->right.value.integer = 0;
        loop->right.operation = TOK_INT_CONSTANT;
        operation = TOK_NOT_EQUAL;
    }
    if (increment) {
        node = (ExpressionNode *)increment;
        category = token_categories[increment->kind];
        if (category != TOK_ASSIGNMENT_OPERATOR && category != TOK_INCREMENT_OPERATOR) return 0;
        if (!describe_qc_operand(node->value.children.left, &loop->target)) return 0;
        if (!describe_qc_operand(node->value.children.right, &loop->step)) return 0;
        loop->increment = increment->kind;
    }
    loop->condition = operation;
    write_ir_record(0x3a, qc_loop_labels->value.label);
    emit_code_label(continue_labels->value.label);
    return 1;
}

/* Original0x2ee90..0x2efa5. Compact operands retain symbol identity;
   an indexed address owns two adjacent compact operands in arena two. */
int __stdcall describe_qc_operand(IrToken *expression, QcOperand *operand)
{
    ExpressionNode *node = (ExpressionNode *)expression;
    IrToken *left, *right;
    QcOperand *pair;
    ArenaPool *pool = &compiler_arena.pools[3];
    operand->operation = expression->kind;
    if (expression->kind == TOK_IDENTIFIER ||
        (expression->kind >= TOK_INT_CONSTANT && expression->kind <= TOK_ULONG_CONSTANT)) {
        operand->value.integer = expression->value.integer;
        return 1;
    }
    if (expression->kind == TOK_DEREFERENCE) {
        left = node->value.children.left;
        if (!left || left->kind != TOK_IDENTIFIER) return 0;
        operand->value.integer = left->value.integer;
        return 1;
    }
    if (expression->kind == TOK_LEFT_BRACKET) {
        left = node->value.children.left;
        right = node->value.children.right;
        if (!left || !right || left->kind != TOK_IDENTIFIER) return 0;
        if (token_categories[right->kind] != TOK_CONSTANT || !(right->type->flags & 0x40)) return 0;
        if (pool->remaining < 2 * (int)sizeof(QcOperand)) refill_arena(&compiler_arena, pool);
        pair = (QcOperand *)pool->next;
        pool->remaining -= 2 * sizeof(QcOperand);
        pool->next += 2 * sizeof(QcOperand);
        operand->value.pair = pair;
        pair[0].value.integer = left->value.integer;
        pair[0].operation = left->kind;
        pair[1].value.integer = right->value.integer;
        pair[1].operation = right->kind;
        return 1;
    }
    return 0;
}

/* Original0x2ed95..0x2edef. Unstructured jumps disable slot selection
   for the function; structured exits retain their own region records. */
void __stdcall emit_inline_jump(int kind)
{
    if (!kind) ++inline_jump_count;
    else if (inline_scope_depth < 8 && inline_region_index < 255) {
        switch (kind) {
        case 0: write_ir_record(0x95, 0); break;
        case 1: write_ir_record(0x96, 0); break;
        case 2: write_ir_record(0x98, 0); break;
        }
    }
}

/* Original0x2f250..0x2f301. Compact loop expressions are expanded at
   their back edge, after the body and its line-information record. */
void finish_qc_loop(void)
{
    QcLoopState *loop = qc_loop_work->value.qc_loop;
    IrToken *left, *right;
    emit_code_label(do_labels->value.label);
    if (current_token_code == TOK_RIGHT_BRACE) record_line_information(1);
    if (loop->increment) {
        right = expand_qc_operand(&loop->step);
        left = expand_qc_operand(&loop->target);
        emit_expression_statement(make_ir_expression(loop->increment, left, right));
    }
    emit_code_label(qc_loop_labels->value.label);
    if (!loop->condition)
        write_ir_record(0x3a, continue_labels->value.label);
    else {
        right = expand_qc_operand(&loop->right);
        left = expand_qc_operand(&loop->left);
        emit_conditional_branch(make_binary_expression(loop->condition, left, right),
            continue_labels->value.label, 1);
    }
    emit_code_label(break_labels->value.label);
}

/* Original0x2f1b0..0x2f249. Recreate an ordinary expression tree from
   the compact symbol/constant/address representation. */
IrToken *__stdcall expand_qc_operand(QcOperand *operand)
{
    IrToken *result, *left, *right;
    unsigned value;
    if (operand->operation == TOK_IDENTIFIER)
        return make_symbol_reference(operand->value.symbol);
    if (operand->operation == TOK_DEREFERENCE) {
        result = make_indirection(make_symbol_reference(operand->value.symbol));
        if (operand->value.symbol->attributes & 0x400)
            result->type = drop_first_type_shape(result->type);
        return result;
    }
    if (operand->operation == TOK_LEFT_BRACKET) {
        right = expand_qc_operand(operand->value.pair + 1);
        left = expand_qc_operand(operand->value.pair);
        return make_ir_expression(TOK_LEFT_BRACKET, left, right);
    }
    value = operand->value.integer;
    return make_constant_token(operand->operation, &value);
}

/* Original0x2e414..0x2e419 is a tail transfer to the owner cleanup. */
void generate_inline_queries(void)
{
    free_inline_query_tables();
}

/* Original0x2ee47..0x2ee64. Preserve the original dangling globals;
   a subsequent initialization allocates both arrays afresh. */
void free_inline_query_tables(void)
{
    free(inline_regions);
    free(inline_scopes);
}
