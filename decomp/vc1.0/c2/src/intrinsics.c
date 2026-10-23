/* Intrinsic argument preparation and command interpretation. */
#include "intrinsics.h"
#include "expression-fields.h"
#include "work-prepare.h"
#include <recon.h>
extern int floating_frontend;
Node *current_intrinsic; /* original0007b9b4 */
/* ID173 is encountered in WinMain's aggregate-zero initialization. Original
   descriptor844a8..844b8: no scalar result-type override, recipe84c08 "vyE0",
   empty runtime-name string, flag10 selects a distinct argument-type record.
   No neighboring intrinsic descriptors or recipe strings have been copied. */
const IntrinsicDescription aggregate_fill_intrinsic = { 0, 0, "vyE0", "", 0x10 };
/* Original [000638a0,00063cec), including12 character-dispatch words at
   63cbc..63cec and the intervening three alignment bytes. The second register
   argument is unused in this build but its fastcall ABI remains unchanged. */
Node *__fastcall lower_intrinsic(Node *node, Node *parent)
{
    const IntrinsicDescription *description;
    const char *commands;
    Node *arguments[5], *argument_nodes[5];
    Node *argument, *value, *temporary, *replacement, *converted, *left, *right;
    unsigned intrinsic = node->left.node->left.value;
    unsigned short type, width;
    unsigned operation;
    int argument_count, index, reversed = 0, command;
    (void)parent;
    current_intrinsic = node;
    node->detail.control.code = 8;
    SELECT_INTRINSIC_DESCRIPTION(intrinsic, description);
    if (description->result_type || (description->flags & 0x10))
        prepare_intrinsic_arguments(node, intrinsic, arguments);
    argument_count = 0;
    for (argument = node->second.node; argument->opcode != 0x4c; argument = argument->second.node) {
        if (argument_count < 5) {
            arguments[argument_count] = argument->left.node;
            argument_nodes[argument_count] = argument;
        }
        ++argument_count;
    }
restart_commands:
    SELECT_INTRINSIC_DESCRIPTION(intrinsic, description);
    commands = description->commands;
    node->left.node = intern_leaf(0x33, node->left.node->attributes.words.flags,
        node->left.node->attributes.words.type, intrinsic);
    while ((command = (signed char)*commands++) != 0) {
        switch (command) {
        case 'A':
            index = (signed char)*commands++ - '0';
            value = arguments[index];
            for (;;) {
                operation = value->opcode;
                if (operation == 0x26 || operation == 0x30 || operation == 0x33) break;
                if (operation == 0x34) {
                    value = value->left.node;
                    if (value->attributes.words.type & 0xf00) continue;
                }
                type = arguments[index]->attributes.words.type;
                temporary = new_temporary_reference(type & 255, 0x200);
                replacement = build_tree("PPNt", temporary, arguments[index], 0x32, type);
                append_intrinsic_expression(replacement);
                replacement = build_tree("DNt", temporary, 0x30, type);
                arguments[index] = replacement;
                argument_nodes[index]->left.node = replacement;
                break;
            }
            break;
        case 'C':
            index = *commands == 'Y';
            if (index) ++commands;
            if (node->opcode == 0x40 && !floating_frontend) node->opcode = 0x3e;
            prepare_generated_intrinsic(node, index);
            break;
        case 'E':
            index = (signed char)*commands++ - '0';
            if (reversed) index = argument_count - index - 1;
            argument_nodes[index]->attributes.words.flags |= 0x8000;
            break;
        case 'I': ++intrinsic; goto restart_commands;
        case 'a':
            release_intrinsic_operands(node);
            operation = intrinsic_operator(intrinsic);
            left = arguments[reversed ? 1 : 0];
            right = arguments[reversed ? 0 : 1];
            replacement = build_tree("PPn", left, right, operation);
            converted = convert_node_type(replacement, node->attributes.words.type);
            *node = *converted;
            release_node(converted);
            node->attributes.words.flags |= 0x4000;
            break;
        case 'b':
            node->detail.control.code = 0;
            release_intrinsic_operands(node);
            node->opcode = intrinsic_operator(intrinsic);
            left = arguments[reversed ? 1 : 0];
            right = arguments[reversed ? 0 : 1];
            build_tree("PPL", left, right, node);
            break;
        case 'd': --intrinsic; goto restart_commands;
        case 'n':
            type = node->attributes.words.type;
            if (type & 0xf00) {
                width = intrinsic_argument_width(node, 0);
                node->attributes.words.type = (type & 0xff00) | width;
            }
            break;
        case 'o': node->attributes.words.type = 0x201; break;
        case 'u':
            release_intrinsic_operands(node);
            node->opcode = intrinsic_operator(intrinsic);
            build_tree("PL", arguments[0], node);
            break;
        case 'v':
reverse_arguments:
            node->second.node = reverse_node_list(node->second.node);
            reversed = 1;
            break;
        case 'w':
            if (!floating_frontend) goto reverse_arguments;
            break;
        case 'y':
            argument = node->second.node;
            value = argument->left.node;
            if (!can_repeat_tree(value)) {
                type = value->attributes.words.type;
                temporary = new_temporary_reference(type & 255, 0x200);
                argument->left.node = build_tree("PPNt", temporary, value, 0x32, type);
                replacement = build_tree("DNt", temporary, 0x30, type);
            } else replacement = copy_tree(value);
            current_intrinsic = build_tree("PPn", current_intrinsic, replacement, 0x44);
            break;
        /* Other character slots in the native dispatch are no-ops. */
        }
    }
    return current_intrinsic;
}

/* Original [00063750,00063801). Normalize each argument container/value to
   its expected intrinsic type. At most five expressions are returned to the
   caller; nonfixed trailing arguments reuse the last default type slot. */
void __fastcall prepare_intrinsic_arguments(Node *node, unsigned intrinsic, Node **arguments)
{
    const unsigned *types = intrinsic_argument_types(intrinsic);
    const IntrinsicDescription *description;
    Node *argument, *value;
    unsigned short original, expected;
    int index = 0;
    for (argument = node->second.node; argument->opcode != 0x4c; argument = argument->second.node) {
        if (index >= 5) {
            SELECT_INTRINSIC_DESCRIPTION(intrinsic, description);
            if (description->flags & 0x10)
                compiler_internal_error("@(#)intrinsc.c:1.148", 0x88);
            --types;
        }
        original = argument->left.node->attributes.words.type;
        expected = (unsigned short)*types;
        argument->attributes.words.type = expected;
        if (expected != original) {
            value = convert_node_type(argument->left.node, expected);
            build_tree("PL", value, argument);
            argument->left.node->attributes.words.flags |= 0x4000;
        }
        if (index < 5) arguments[index] = argument->left.node;
        ++index;
        ++types;
    }
}

/* Original838e0..838f4: five cached default argument-type words, initially
   zero. Fixed signatures are28-byte records (ID, reserved halfword, six types).
   Only the encountered aggregate-fill signature84a8c..84aa8 is introduced. */
static unsigned default_intrinsic_argument_types[5];
typedef struct IntrinsicArgumentRecord {
    short intrinsic;
    unsigned short reserved;
    unsigned types[6];
} IntrinsicArgumentRecord;
typedef char IntrinsicArgumentRecordSizeCheck[sizeof(IntrinsicArgumentRecord) == 28 ? 1 : -1];
static const IntrinsicArgumentRecord intrinsic_argument_records[] = {
    { INTRINSIC_AGGREGATE_FILL, 0, { 0x104, 0x104, 0x404, 0, 0, 0 } },
    { 0, 0, { 0, 0, 0, 0, 0, 0 } }
};
/* Original [000636c0,0006374c). Scalar-type intrinsics share a five-slot
   cache; flag10 intrinsics select their stable, distinct argument signature. */
const unsigned *__fastcall intrinsic_argument_types(unsigned intrinsic)
{
    const IntrinsicDescription *description;
    const IntrinsicArgumentRecord *record;
    unsigned type;
    int index;
    SELECT_INTRINSIC_DESCRIPTION(intrinsic, description);
    if (!(description->flags & 0x10)) {
        type = (unsigned)(int)(short)description->result_type;
        if (default_intrinsic_argument_types[0] != type)
            for (index = 0; index < 5; ++index) default_intrinsic_argument_types[index] = type;
        return default_intrinsic_argument_types;
    }
    record = intrinsic_argument_records;
    while (record->intrinsic && record->intrinsic < (int)intrinsic) ++record;
    if (record->intrinsic != (int)intrinsic)
        compiler_internal_error("@(#)intrinsc.c:1.148", 0x74);
    return record->types;
}
