/* Qualified C++ identifiers, including constructor/destructor/operator names. */
#include "lexer.h"
#include "class-lookup.h"
#include <recon.h>

SymbolScope *last_lookup_scope; /* original 0x914bc */

/* Original 0x332c0..0x332f8. Deferred inline text names its class using
   __nctag; the empty form leaves only a scope opened by that directive. */
void __stdcall declare_nctag(char *name)
{
    CppSymbol *tag;
    if (!name) {
        if ((current_scope->flags & 0xff) == 0xfd) {
            retire_scope_directory_entry();
            pop_symbol_scope();
        }
        return;
    }
    tag = lookup_tag_name(name);
    if (tag) enter_class_scope(tag, (signed char)-3);
}

/* The operator-name bit (0x40) of original lexical properties at 0xa1d47.
   Keep the encountered category as token membership, rather than copying the
   unrelated property bits. Recovery also accepts several internal token forms. */
unsigned char operator_name_tokens[] = {
    OP_BIT_AND, OP_LOGICAL_AND, OP_AND_ASSIGN, OP_ASSIGN, 63, 64,
    OP_POST_DECREMENT, OP_DIVIDE, OP_DIVIDE_ASSIGN, OP_EQUAL, OP_NOT,
    OP_DEREFERENCE, OP_GREATER, OP_GREATER_EQUAL, OP_POST_INCREMENT,
    OP_SUBSCRIPT, OP_SHIFT_LEFT_ASSIGN, OP_SHIFT_LEFT, OP_LESS, OP_LESS_EQUAL,
    OP_SUBTRACT, OP_SUBTRACT_ASSIGN, OP_REMAINDER, OP_REMAINDER_ASSIGN,
    OP_MULTIPLY, OP_MULTIPLY_ASSIGN, OP_NOT_EQUAL, OP_BIT_OR, OP_OR_ASSIGN,
    OP_LOGICAL_OR, 89, OP_ADD, OP_ADD_ASSIGN, OP_ARROW, OP_PRE_DECREMENT,
    OP_PRE_INCREMENT, 95, OP_SHIFT_RIGHT_ASSIGN, OP_SHIFT_RIGHT,
    OP_COMPLEMENT, OP_NEGATE, OP_BIT_XOR, OP_XOR_ASSIGN,
    104, 107, 114, 120, 125, 134, 139, TOK_SIZEOF, 142, OP_UNARY_PLUS,
    TOK_NEW, TOK_DELETE, 157, OP_MEMBER_POINTER, 159, 160, 171, 172, 177, 0
};

/* Original 0x1ad30..0x1af0c, cdecl, byte result. The semantic value
   keeps the raw operator byte and interned spelling in separate words. */
unsigned char read_operator_function_id(void)
{
    unsigned char code, token;
    unsigned index;
    char *name;
    token_allocation_flags &= ~1;
    cpp_parser_flags |= 0x21;
    token_allocation_flags |= 0x10;
    read_parser_token();
    token_allocation_flags &= ~0x10;
    cpp_parser_flags &= ~0x21;
    code = current_token_code;
    switch (code) {
    case 3: return 3;
    case 35:
        token_allocation_flags |= 4;
        return 154;
    case OP_ASSIGN:
        name = name_assignment;
        goto operator_name;
    case OP_SUBSCRIPT:
        token = scan_token();
        if (current_token_code != 132) {
            report_syntax_error(543);
            unget_token(token);
        }
        name = intern_identifier("[]");
        goto operator_name;
    case OP_CALL:
        token = scan_token();
        if (current_token_code != 136) {
            report_syntax_error(544);
            unget_token(token);
        }
        name = intern_identifier("()");
        if (declaration_depth > 0) --declaration_depth;
        goto operator_name;
    case OP_COMPLEMENT:
        token_allocation_flags &= ~1;
        break;
    case TOK_NEW:
        name = name_new;
        goto operator_name;
    case TOK_DELETE:
        name = name_delete;
        goto operator_name;
    }
    for (index = 0; operator_name_tokens[index]; ++index) {
        if (operator_name_tokens[index] == code) {
            name = intern_identifier(token_spellings[code]);
            goto operator_name;
        }
    }
    report_syntax_error(833, code == 2 ? current_token_value.name : token_spellings[code]);
    return 1;
operator_name:
    current_token_value.words.high = (unsigned)name;
    current_token_value.bytes[0] = code;
    return 153;
}

/* Original 0x1cc50..0x1d480. The interned spelling is retained separately
   from constructor/destructor internal names throughout recovery. */
unsigned char __stdcall resolve_identifier_token(char *name)
{
    QualifierResolution *scope = 0, *previous;
    QualifiedDeclarator *qualified;
    CppSymbol *symbol, *target;
    char *member, *original_member;
    unsigned char token, operator_code;
    unsigned short first_modifier = 0;
    unsigned i;
    int errors = 0, global = name == 0;
    int constructor = 0, destructor = 0, inherited = 0, temporary = 0;

    token = scan_token();
    if (token != 160) {
        unget_token(token);
        current_token_value.name = name;
        current_token_code = 2;
        return 2;
    }
    if (name) {
        symbol = find_qualifier_name(name);
        if (!symbol) {
            symbol = find_outer_name(name);
            if (symbol) {
                if ((symbol->kind & 31) == 18)
                    symbol = (CppSymbol *)((QualifiedDeclarator *)symbol)->resolution->binding->target;
                if (!(symbol->flags & 0x10) || !is_unqualified_class_type(symbol->type)) symbol = 0;
                else symbol = cpp_context_stack->items[symbol->type->field08];
            }
            if (!symbol) {
                report_syntax_error(653, name);
                errors = 1;
            }
        }
        if (symbol) {
            if ((symbol->kind & 31) == 18)
                scope = ((QualifiedDeclarator *)symbol)->resolution;
            else {
                begin_qualification_allocation(1);
                scope = append_qualifier_reference(0, 0, symbol);
                end_qualification_allocation();
            }
            if (!(token_allocation_flags & 0x80)) {
                target = (CppSymbol *)scope->binding->target;
                if (symbol_scope_mode == 255 && (last_lookup_scope->flags & 0x100) &&
                    (!(target->flags & 0x10) ||
                     (is_unqualified_class_type(target->type) && last_lookup_scope->buckets[0] == target)))
                    inherited = 1;
                else if (symbol_scope_mode == 252) temporary = 1;
            }
            enter_qualified_class((CppSymbol *)scope->binding->target, name, 1);
        }
    }

    do {
        original_member = 0;
        member = 0;
        operator_code = 1;
read_member:
        token = scan_token();
        switch (token) {
        case 130: /* storage/model modifier before a qualified member */
            if (!first_modifier) {
                for (i = 0; i < declaration_code_count; ++i)
                    if (declaration_codes[i].token == current_token_code) break;
                first_modifier = (unsigned short)(i < declaration_code_count ? declaration_codes[i].value : 0x9e);
                report_syntax_error(615);
            }
            goto read_member;
        case 151: /* new/delete */
        case 152:
            if (!global) report_syntax_error(834, token_spellings[151]);
            if (cpp_parser_flags & 0x20)
                report_syntax_error(722, token_spellings[token], token_spellings[token]);
            current_token_value.bytes[0] = 2;
            return token;
        case 162: /* operator */
            token = read_operator_function_id();
            if (token == 1) goto next_qualifier;
            if (token == 3) {
                if (token_allocation_flags & 0x400) {
                    member = intern_identifier(current_token_value.name);
                    goto lookup_member;
                }
                report_syntax_error(59, token_spellings[3]);
                ++errors;
                if (scope) unwind_qualified_scopes(1);
                scope = 0;
                goto next_qualifier;
            }
            if (token == 153) {
                operator_code = current_token_value.bytes[0];
                member = (char *)current_token_value.words.high;
                goto resolve_member;
            }
            if (token == 154) {
                token_allocation_flags |= 1;
                goto unread;
            }
            member = current_token_value.name;
            goto resolve_member;
        case 98: /* ~class-name */
            token = scan_token();
            if (token != 2) {
                report_syntax_error(611, token_spellings[token]);
                return token;
            }
            member = current_token_value.name;
            if (scope) {
                target = (CppSymbol *)scope->binding->target;
                if (member != target->name) report_syntax_error(523, target->name, member);
                if (destructor) report_syntax_error(596, member);
                if (constructor) report_syntax_error(595, member);
                destructor = 1;
                original_member = member;
                member = name_dtor;
                goto lookup_member;
            }
            if (!errors) {
                report_syntax_error(588, member);
                errors = 1;
            }
            goto next_qualifier;
        case 0:
            fatal_error(4);
            goto lookup_member;
        case 2:
            member = current_token_value.name;
            goto resolve_member;
        case 83: /* qualified member pointer */
            if (!scope) {
                report_syntax_error(645);
                token = 83;
            } else {
                unwind_qualified_scopes(1);
                token = 170;
            }
            goto finish;
        default:
            report_syntax_error(589, token_spellings[token]);
            if (scope) unwind_qualified_scopes(1);
            scope = 0;
            goto unread;
        }

resolve_member:
        if (!scope) {
            if (!errors) {
                symbol = find_scope_symbol(global_scope, member, 0);
                if (!symbol) {
                    if (token == 153) return 162;
                    report_syntax_error(581, member);
                    ++errors;
                    scope = 0;
                } else {
                    if ((symbol->kind & 31) == 9 && (symbol->flags & 0x10) &&
                        is_unqualified_class_type(symbol->type)) {
                        symbol = cpp_context_stack->items[symbol->type->field08];
                        if (cpp_parser_flags & 0x10000) {
                            token = scan_token();
                            unget_token(token);
                            if (token != 160) report_syntax_error(242);
                        }
                    }
                    begin_qualification_allocation(1);
                    scope = append_qualifier_reference(0, 0, symbol);
                    end_qualification_allocation();
                    enter_qualified_class((CppSymbol *)scope->binding->target, member, 1);
                }
            }
            goto next_qualifier;
        }
        if (((CppSymbol *)scope->binding->target)->name == member) {
            if (constructor) report_syntax_error(595, member);
            if (destructor) report_syntax_error(596, member);
            constructor = 1;
            original_member = member;
            member = name_ctor;
        }
lookup_member:
        previous = scope;
        scope = lookup_qualified_member(scope, member, 0);
        if (!scope) {
            if (current_class_context < 0) constructor = destructor = 0;
            if (destructor) {
                current_token_value.symbol = new_declarator(name_dtor);
                current_token_value.symbol->declaration_flags |= 0x2000;
                current_token_code = 184;
                return 184;
            }
            if (constructor) {
                current_token_value.symbol = new_declarator(name_ctor);
                current_token_value.symbol->declaration_flags |= 0x1000;
                current_token_code = 184;
                return 184;
            }
            target = (CppSymbol *)previous->binding->target;
            if (!(target->flags & 4) && current_class_name() == target->name) {
                if (operator_code != 1) {
                    current_token_value.bytes[0] = operator_code;
                    current_token_value.words.high = (unsigned)member;
                    return 153;
                }
                if (token == 2) {
                    current_token_value.name = member;
                    return 2;
                }
            }
            ++errors;
        }
        if (token == 3) {
            token = 161;
            current_token_code = 3;
            goto finish;
        }
next_qualifier:
        token = scan_token();
    } while (token == 160);
unread:
    unget_token(token);
    token = 161;
finish:
    if (!scope) {
        if (original_member) member = original_member;
        if (member) {
            current_token_value.name = member;
            current_token_code = 2;
            return 2;
        }
        if (token == 83) {
            current_token_code = 83;
            current_token_value.bytes[0] = 83;
            return token_categories[83];
        }
        current_token_code = 160;
        current_token_value.bytes[0] = 160;
        return token_categories[160];
    }
    qualified = new_qualified_declarator(scope, member, declaration_mode != 0);
    current_token_value.symbol = (CppSymbol *)qualified;
    if (cpp_parser_flags & 0x10000) {
        target = (CppSymbol *)qualified->resolution->binding->target;
        if ((target->kind & 31) == 9 && (target->flags & 0x10) &&
            is_unqualified_class_type(target->type)) report_syntax_error(242);
    }
    resolve_declaration_alias(current_token_value.symbol, 0);
    qualified = (QualifiedDeclarator *)current_token_value.symbol;
    if (constructor) qualified->declaration_flags |= 0x1000;
    if (destructor) qualified->declaration_flags |= 0x2000;
    if (inherited) qualified->declaration_flags |= 0x8000;
    if (temporary) qualified->flags |= 0x80;
    if (current_token_code == 3) qualified->declaration_flags |= 0x4000;
    else current_token_code = token;
    return token;
}

/* Original 0xa8100/0xa8104: the qualification factories share one saved
   arena selector, rather than a general nested allocator stack. */
int qualification_arena;
int saved_qualification_arena = -1;

/* Original 0x77de0..0x77df6. */
void __stdcall begin_qualification_allocation(int arena)
{
    saved_qualification_arena = qualification_arena;
    qualification_arena = arena;
}

/* Original 0x77e20..0x77edb. Append a twelve-byte reference node to the
   eight-byte path object, choosing the shared qualification arena for each
   allocation. Arena -1 means the compiler's pool zero, not permanence. */
QualifierResolution *__stdcall append_qualifier_reference(QualifierResolution *resolution,
    QualifierBinding *parent, CppSymbol *target)
{
    ArenaPool *pool;
    QualifierBinding *reference;
    int arena;
    if (!resolution) {
        if (!qualification_arena) resolution = allocate_permanent(sizeof(QualifierResolution));
        else {
            arena = qualification_arena == -1 ? 0 : qualification_arena;
            pool = &compiler_arena.pools[arena];
            if (pool->remaining < (int)sizeof(QualifierResolution)) refill_arena(&compiler_arena, pool);
            resolution = (QualifierResolution *)pool->next;
            pool->remaining -= sizeof(QualifierResolution);
            pool->next += sizeof(QualifierResolution);
        }
    }
    if (!qualification_arena) reference = allocate_permanent(sizeof(QualifierBinding));
    else {
        arena = qualification_arena == -1 ? 0 : qualification_arena;
        pool = &compiler_arena.pools[arena];
        if (pool->remaining < (int)sizeof(QualifierBinding)) refill_arena(&compiler_arena, pool);
        reference = (QualifierBinding *)pool->next;
        pool->remaining -= sizeof(QualifierBinding);
        pool->next += sizeof(QualifierBinding);
    }
    /* The target prefix is a tag or an inherited ClassReference by kind. */
    reference->target = (ClassReference *)target;
    reference->next = parent;
    if (!resolution->binding) resolution->binding = resolution->tail = reference;
    else {
        resolution->tail->following = reference;
        resolution->tail = reference;
    }
    return resolution;
}

/* Original 0x77e00..0x77e15. Restore the one saved selection and mark
   its slot unused again, preserving the native -1 sentinel. */
void end_qualification_allocation(void)
{
    qualification_arena = saved_qualification_arena;
    saved_qualification_arena = -1;
}

/* Original 0x33300..0x3336a. Enter a class qualifier's temporary scope,
   retaining inherited-reference and exact-name flags on the resulting scope. */
void __stdcall enter_qualified_class(CppSymbol *symbol, char *name, int temporary)
{
    SymbolScope *scope;
    int inherited = 0;
    if (!symbol) return;
    if ((symbol->kind & 31) == 15) {
        symbol = cpp_context_stack->items[((ClassReference *)symbol)->context_index];
        inherited = 1;
    } else if (!symbol_has_tag_scope(symbol)) return;
    scope = enter_class_scope(symbol, (signed char)(temporary ? -4 : -2));
    if (scope && (inherited || symbol->name == name)) scope->flags |= 0x100;
}

/* Original 0x1d480..0x1d6df. Resolve a member through its complete qualifier
   path, preserving alias substitution, incomplete-class handling and the
   distinction between a member spelling and a conversion's type spelling. */
QualifierResolution *__stdcall lookup_qualified_member(QualifierResolution *scope,
    char *name, CType *type)
{
    CppSymbol *tag = (CppSymbol *)scope->binding->target, *resolved;
    QualifierBinding *parent;
    QualifiedDeclarator *qualified;
    char *spelling, *formatted;
    short text;
    unsigned kind = tag->kind & 31;
    if (kind != 4 && kind != 15) {
        spelling = tag->name;
        parent = scope->binding->next;
        if ((tag->flags & 0x10) && is_unqualified_class_type(tag->type)) {
            tag = cpp_context_stack->items[tag->type->field08];
            scope->binding->target = (ClassReference *)tag;
            goto resolved_tag;
        }
        if (tag->kind & 0x80) {
            token_allocation_flags |= 0x10;
            resolved = resolve_declaration_alias(tag, 0);
            token_allocation_flags &= ~0x10UL;
        } else if (parent)
            resolved = (CppSymbol *)lookup_class_name((CppSymbol *)parent->target, spelling, 1, 385);
        else if (token_allocation_flags & 0x80)
            resolved = (CppSymbol *)lookup_class_name(class_lookup_context, spelling, 1, 385);
        else resolved = lookup_tag_name(spelling);
        if (!resolved) {
            report_syntax_error(510, spelling);
            return 0;
        }
        if ((resolved->kind & 31) == 18)
            scope->binding = attach_qualifier_parent(((QualifiedDeclarator *)resolved)->resolution->binding, parent);
        else {
            begin_qualification_allocation(1);
            scope = append_qualifier_reference(0, parent, resolved);
            end_qualification_allocation();
        }
        enter_qualified_class((CppSymbol *)scope->binding->target, 0, 0);
        tag = (CppSymbol *)scope->binding->target;
        kind = tag->kind & 31;
    }
    if ((tag->kind & 31) == 15)
        tag = cpp_context_stack->items[((ClassReference *)tag)->context_index];
resolved_tag:
    if (tag->flags & 4) {
        qualified = lookup_class_name(tag, name, 0x102, 385);
        if (qualified) {
            enter_qualified_class((CppSymbol *)qualified->resolution->binding->target, 0, 0);
            attach_qualifier_parent(qualified->resolution->binding, scope->binding);
            return qualified->resolution;
        }
        if (!type) report_syntax_error(39, name, tag->name);
        else {
            begin_type_diagnostic();
            text = format_declarator(0, type);
            formatted = type_diagnostic_text(text);
            report_syntax_error(39, formatted, tag->name);
            end_type_diagnostic();
        }
        if (scope) unwind_qualified_scopes(1);
        return 0;
    }
    if (!(token_allocation_flags & 0x20000000UL)) {
        if (current_class_name() != tag->name && !options.prototypes_only)
            report_syntax_error(27, tag->name);
        if (scope) unwind_qualified_scopes(1);
        return 0;
    }
    resolved = new_declarator(name);
    return append_qualifier_reference(0, scope->binding, resolved);
}

/* Original 0x787a0..0x787d5. Replace the first enclosing class-tag link
   while retaining inherited-base links. The forward markers connect this
   selected path to its parent, including the parent's native self marker. */
QualifierBinding *__stdcall attach_qualifier_parent(QualifierBinding *binding,
    QualifierBinding *parent)
{
    QualifierBinding *cursor = binding;
    while (cursor->next && (((CppSymbol *)cursor->next->target)->kind & 31) != 4)
        cursor = cursor->next;
    cursor->next = parent;
    binding->following = parent;
    parent->following = parent;
    return binding;
}

/* Original 0x3b130..0x3b187. The qualifier and interned spelling occupy
   the final words of this sixteen-byte temporary declaration record. */
QualifiedDeclarator *__stdcall new_qualified_declarator(QualifierResolution *resolution,
    char *name, int arena)
{
    QualifiedDeclarator *declaration;
    ArenaPool *pool;
    if (!arena) declaration = allocate_permanent(sizeof(QualifiedDeclarator));
    else {
        if (arena == -1) arena = 0;
        pool = &compiler_arena.pools[arena];
        if (pool->remaining < (int)sizeof(QualifiedDeclarator))
            refill_arena(&compiler_arena, pool);
        declaration = (QualifiedDeclarator *)pool->next;
        pool->remaining -= sizeof(QualifiedDeclarator);
        pool->next += sizeof(QualifiedDeclarator);
    }
    declaration->kind = 18;
    declaration->resolution = resolution;
    declaration->name = name;
    declaration->declaration_flags |= 0x800;
    return declaration;
}
