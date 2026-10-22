/* Constructor base/member initialization, original 0x35f90..0x363c3. */
#include "constructor-initializers.h"
#include "statement-scopes.h"
#include "class-members.h"
#include "cpp-context.h"
#include "inline-vector.h"
#include "class-lookup.h"
#include "special-members.h"

int __stdcall emit_constructor_initializers(CppSymbol *tag, IrToken *initializers)
{
    CppSymbol *member, *subobject_class, *target;
    BaseClassLink *base;
    IrToken *arguments, *expression, *initializer, *callee;
    TypeShape *shape;
    unsigned short attributes;
    unsigned displacement;
    int saved_access, member_subobject, copying, required;
    char *name;

    /* The native helper consumes this formal argument even though its body
       uses the global function_symbol. Keep its original stdcall 4 ABI. */
    attributes = constructor_pointer_attributes(function_symbol);
    prepare_class_initialization(tag);
    if (tag->detail.class_info->flags & 0x10) {
        saved_access = get_base_initialization_access();
        if (!saved_access) set_base_initialization_access(1);
        emit_virtual_base_initializers(tag, &initializers, attributes);
        if (!saved_access) set_base_initialization_access(0);
    }
    for (member = tag->auxiliary.scope_members; member; member = member->next) {
        if ((member->kind & 0x1f) == 17 || member->name == name_alignment ||
            is_static_data_member(member) || (member->flags & 0x10) ||
            (member->kind & 0x1f) == 5)
            continue;
        if ((member->kind & 0x1f) == 15) {
            base = (BaseClassLink *)member;
            if (base->state & 1) continue;
            subobject_class = cpp_context_stack->items[base->context_index];
            displacement = base->displacement;
            member_subobject = 0;
        } else {
            subobject_class = class_type_of_member(member);
            displacement = member->detail.value;
            member_subobject = subobject_class != 0;
        }
        arguments = take_member_initializer(&initializers, member->name);
        if (!arguments) {
            if (subobject_class) {
                if (!has_class_initialization(make_class_pointer_type(subobject_class, attributes))) {
                    report_syntax_error(512, subobject_class->name);
                    return 0;
                }
                if (member->flags & 0x40)
                    emit_class_array_initialization(member, subobject_class, 0,
                        displacement, 0, attributes);
                else {
                    if (!member_subobject) set_base_initialization_access(1);
                    make_constructor_call(subobject_class,
                        make_subobject_pointer(subobject_class, attributes, displacement),
                        0, 1, member_subobject);
                    set_base_initialization_access(0);
                }
                continue;
            }
            shape = member->type->related;
            if (shape && !(shape->flags & 0x77)) shape = shape->related;
            if (!shape) required = (member->type->attributes & 0x1000) != 0;
            else required = ((shape->attributes & 0x1000) && !(shape->flags & 3)) ||
                            (shape->flags & 0x40);
            if (required) report_syntax_error(758, member->name);
            continue;
        }
        if ((member->kind & 0x1f) == 3 && member->type->related) {
            shape = member->type->related;
            if (!(shape->flags & 0x77)) shape = shape->related;
            if (shape && (shape->flags & 0x77) == 2 &&
                (!subobject_class || (subobject_class->declaration_flags & 0x1000))) {
                copying = (function_symbol->cpp.parts.member_flags & 0x20) &&
                    (tag->detail.class_info->implicit_member_requirements & 2);
                if (copying) {
                    if (subobject_class && (member->flags & 0x40))
                        emit_class_array_initialization(member, subobject_class, arguments,
                            displacement, 1, attributes);
                    else
                        emit_array_copy_initialization(member, displacement, arguments, attributes);
                } else report_syntax_error(536, tag->name, member->name);
                continue;
            }
        }
        if (!subobject_class) {
            expression = prepare_expression(arguments, 1);
            mark_initialized_symbol(member);
            member_initializer_active = 1;
            if (!initialize_member_expression(displacement, member->type, expression, member))
                report_syntax_error(439, member->name);
            member_initializer_active = 0;
            initializing_symbol = 0;
            initializing_reference = 0;
        } else {
            if (!(subobject_class->declaration_flags & 0x1000)) {
                report_syntax_error(542, member->name);
                continue;
            }
            if (!member_subobject) set_base_initialization_access(1);
            expression = make_constructor_call(subobject_class,
                make_subobject_pointer(subobject_class, attributes, displacement),
                arguments, 0, member_subobject);
            expression = prepare_expression(expression, 0);
            if (expression) emit_expression_statement(expression);
            set_base_initialization_access(0);
        }
    }
    /* Entries left after consuming real subobjects are invalid or duplicate
       member initializers. Their designator retains the symbol's name. */
    for (initializer = initializers; initializer; initializer = initializer->tail.right) {
        expression = initializer->value.pointer;
        callee = expression->value.pointer;
        target = callee->value.pointer;
        name = target->name;
        for (member = tag->auxiliary.scope_members; member; member = member->next)
            if (member->name == name) break;
        if (!member) report_syntax_error(614, tag->name, name);
        else if ((member->kind & 0x1f) == 17) report_syntax_error(436, name);
        else if (is_static_data_member(member)) report_syntax_error(438, name);
        else report_syntax_error(437, name);
    }
    required = !(((function_symbol->cpp.parts.member_flags & 0x20) &&
                  (tag->detail.class_info->implicit_member_requirements & 2)) ||
                 ((function_symbol->cpp.parts.member_flags & 0x10) &&
                  (tag->detail.class_info->implicit_member_requirements & 1)));
    emit_class_table_initializers(tag, required);
    return 1;
}

/* Original 0x363d0..0x363fa. The unused formal is part of the original
   stdcall ABI; the selected function and implicit-this type are global. */
unsigned short __stdcall constructor_pointer_attributes(CppSymbol *unused_function)
{
    TypeShape *function = function_symbol->type->related;
    if (function && !(function->flags & 0x77)) function = function->related;
    return function->data.parameters->associated_type->related->attributes & 7;
}

InlineVector *classes_pending_table_output; /* original 0x9f3d0 */

/* Original 0x7b3f0..0x7b439. Queue classes with dispatch tables and make
   their deleting-destructor definitions available for initialization. */
void __stdcall prepare_class_initialization(CppSymbol *tag)
{
    if (tag->detail.class_info->alignment_flags & 0xc0) {
        if (!classes_pending_table_output)
            classes_pending_table_output = new_inline_vector(1);
        classes_pending_table_output = add_inline_pointer(classes_pending_table_output, tag);
    }
    require_deleting_destructor(tag);
}

/* Original 0x38bb0..0x38bc3. The deleting-destructor entry retains its
   own identity while sharing overload-definition processing by name. */
void __stdcall require_deleting_destructor(CppSymbol *tag)
{
    require_class_member_definitions(tag, name_delete_dtor);
}

/* Original 0x38b20..0x38ba6. Resolve a special-member overload set and
   defer each generated definition while retaining the original traversal. */
void __stdcall require_class_member_definitions(CppSymbol *tag, char *name)
{
    CppSymbol *member = find_class_special_member(tag, name), *current;
    if (member && ((member->kind & 0x1f) == 16 || (member->kind & 0x1f) == 17))
        member = ((OverloadSet *)member)->functions;
    while (member && ((member->kind & 0x1f) == 6 || (member->kind & 0x1f) == 7)) {
        current = member;
        member = member->next;
        if (current->storage[1] & 2) defer_generated_member(current);
    }
}

/* Original 0x364c0..0x3653c. Consume one matching initializer while
   restoring any symbol records detached with that expression. */
IrToken *__stdcall take_member_initializer(IrToken **initializers, char *name)
{
    IrToken **link, *item, *expression, *callee, *arguments;
    ExpressionSymbolList *symbols;
    CppSymbol *target;
    for (link = initializers; *link; link = &item->tail.right) {
        item = *link;
        symbols = 0;
        expression = item->value.pointer;
        if (expression && expression->kind == 0xc8) {
            symbols = (ExpressionSymbolList *)expression->tail.right;
            expression = expression->value.pointer;
        }
        callee = expression->value.pointer;
        target = callee->value.pointer;
        if (target->name == name) {
            *link = item->tail.right;
            arguments = expression->tail.right;
            if (!options.prototypes_only && symbols) restore_expression_symbols(symbols);
            return arguments;
        }
    }
    return 0;
}

/* Original 0x36540..0x365d2. Address a member relative to the adjusted
   this expression, preserving optional bit-field position/width metadata. */
int __stdcall initialize_member_expression(unsigned displacement, CType *type,
    IrToken *value, CppSymbol *member)
{
    IrToken *object, *offset, *target, *position, *width, *range;
    CType *member_type;
    object = adjusted_this_expression(0);
    member_type = replace_type_pointer_kind(type, (unsigned short)(object->type->related->attributes & 7));
    offset = make_integer_constant(displacement);
    target = new_ir_expression(0x5c, object, offset, member_type);
    if (member && (member->kind & 0x1f) == 2) {
        width = make_integer_constant((member->auxiliary.value >> 8) & 0xff);
        position = make_integer_constant(member->auxiliary.value & 0xff);
        range = new_left_typed_expression(0x3f, position, width);
        target = new_left_typed_expression(0x3e, target, range);
    }
    if (value->kind == 0x6a && !value->tail.right) value = value->value.pointer;
    return initialize_lvalue(target, value);
}

/* Original 0x347b0..0x347ee. An adjusted member function subtracts its
   recorded this displacement and marks the resulting address expression. */
IrToken *__stdcall adjusted_this_expression(CppSymbol *parameter)
{
    IrToken *object = this_pointer_expression(parameter), *offset;
    if (!object) return 0;
    if (function_symbol->field28) {
        offset = make_integer_constant(function_symbol->field28);
        object = new_left_typed_expression(0x4f, object, offset);
        object->flags |= 0x20;
    }
    return object;
}

/* Original 0x63680..0x636b6. Replace only the low pointer-kind bits in
   a copied type description, then release that description-stack entry. */
CType *__stdcall replace_type_pointer_kind(CType *type, unsigned short kind)
{
    TypeDescription *description = copy_type_description(type);
    CType *result;
    description->attributes &= 0xfff8;
    description->attributes |= kind & 7;
    result = intern_type(description);
    --type_description_depth;
    return result;
}
