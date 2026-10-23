/* Intrinsic argument preparation and command interpretation. */
#include "intrinsics.h"
#include "expression-fields.h"
#include "work-prepare.h"
#include <recon.h>
extern int floating_frontend;
enum { INTRINSIC_SQRT_FLOAT = 92, INTRINSIC_SQRT_DOUBLE = 93,
    INTRINSIC_SIN_FLOAT = 106, INTRINSIC_SIN_DOUBLE = 107,
    INTRINSIC_COS_FLOAT = 110, INTRINSIC_COS_DOUBLE = 111,
    INTRINSIC_ATAN2_FLOAT = 132, INTRINSIC_ATAN2_DOUBLE = 133,
    INTRINSIC_STRING_COPY = 164, INTRINSIC_STRING_COMPARE = 165, INTRINSIC_STRING_LENGTH = 167,
    INTRINSIC_AGGREGATE_COPY = 172, INTRINSIC_AGGREGATE_FILL = 173,
    INTRINSIC_POWER_DOUBLE = 174 };

Node *current_intrinsic; /* original0007b9b4 */
/* Original83f98..83fb8: square root reached after binary power lowering. */
const IntrinsicDescription root_float_intrinsic = { 0x1004, 0, "C", "sqrt", 0x120 };
const IntrinsicDescription root_double_intrinsic = { 0x1008, 0, "d", "", 0x120 };

/* Original84078..84098: the encountered double sine descriptor redirects
   by command 'd' to the preceding scalar descriptor. Commands839c0/839c4
   and runtime name84d38 are introduced only for these two reached records. */
const IntrinsicDescription sine_float_intrinsic = { 0x1004, 0, "C", "sin", 0x420 };
const IntrinsicDescription sine_double_intrinsic = { 0x1008, 0, "d", "", 0x20 };
/* Original840b8..840d8: cosine pair reached after sine in the x87 case. */
const IntrinsicDescription cosine_float_intrinsic = { 0x1004, 0, "C", "cos", 0x420 };
const IntrinsicDescription cosine_double_intrinsic = { 0x1008, 0, "d", "", 0x20 };
/* Original84218..84238: two-argument arctangent reached after cosine. */
const IntrinsicDescription arctangent2_float_intrinsic = { 0x1004, 0, "wC", "atan2", 0xa20 };
const IntrinsicDescription arctangent2_double_intrinsic = { 0x1008, 0, "d", "", 0x20 };
/* Original844b8..844c8: power reverses its arguments before lowering to
   a binary tree; the runtime alternatives retain their original FF separator. */
const IntrinsicDescription power_double_intrinsic = { 0x1008, 0, "vb", "fexp\377pow", 0xb20 };




/* ID173 is encountered in WinMain's aggregate-zero initialization. Original
   descriptor844a8..844b8: no scalar result-type override, recipe84c08 "vyE0",
   empty runtime-name string, flag10 selects a distinct argument-type record.
   No neighboring intrinsic descriptors or recipe strings have been copied. */
/* Original84498..844a8, recipe84c20: normalize argument width, reverse
   arguments and retain both address expressions for aggregate copy. */
const IntrinsicDescription aggregate_copy_intrinsic = { 0, 0, "nvyE0E1", "", 0x10 };
const IntrinsicDescription aggregate_fill_intrinsic = { 0, 0, "vyE0", "", 0x10 };
/* Original84418..84428: strcpy's scalar pointer argument recipe. */
const IntrinsicDescription string_copy_intrinsic = { 0x404, 0, "nvyE0E1", "", 0x40 };
/* Original84428..84438: strcmp retains both pointer argument trees. */
const IntrinsicDescription string_compare_intrinsic = { 0x404, 0, "vE0E1", "", 0x40 };
/* Original84448..84458: strlen's pointer argument normalization, reverse
   argument-list recipe and retained first argument. Empty runtime name. */
const IntrinsicDescription string_length_intrinsic = { 0x404, 0, "vE0", "", 0x40 };
/* An explicit data frontier, not a substitute implementation for another
   intrinsic. Inline INT3 is the intentional unreconstructed-data trap; keeping
   this dispatch inline introduces no nonoriginal function/call edge. */
#define SELECT_INTRINSIC_DESCRIPTION(id, description) do { \
    switch (id) { \
    case INTRINSIC_SQRT_FLOAT: (description) = &root_float_intrinsic; break; \
    case INTRINSIC_SQRT_DOUBLE: (description) = &root_double_intrinsic; break; \
    case INTRINSIC_SIN_FLOAT: (description) = &sine_float_intrinsic; break; \
    case INTRINSIC_SIN_DOUBLE: (description) = &sine_double_intrinsic; break; \
    case INTRINSIC_COS_FLOAT: (description) = &cosine_float_intrinsic; break; \
    case INTRINSIC_COS_DOUBLE: (description) = &cosine_double_intrinsic; break; \
    case INTRINSIC_ATAN2_FLOAT: (description) = &arctangent2_float_intrinsic; break; \
    case INTRINSIC_ATAN2_DOUBLE: (description) = &arctangent2_double_intrinsic; break; \
    case INTRINSIC_STRING_COMPARE: (description) = &string_compare_intrinsic; break; \
    case INTRINSIC_STRING_COPY: (description) = &string_copy_intrinsic; break; \
    case INTRINSIC_STRING_LENGTH: (description) = &string_length_intrinsic; break; \
    case INTRINSIC_AGGREGATE_COPY: (description) = &aggregate_copy_intrinsic; break; \
    case INTRINSIC_AGGREGATE_FILL: (description) = &aggregate_fill_intrinsic; break; \
    case INTRINSIC_POWER_DOUBLE: (description) = &power_double_intrinsic; break; \
    default: __asm { int 3 } \
        (description) = 0; break; \
    } \
} while (0)

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
   Encountered aggregate-copy/fill signatures84a70..84aa8 are introduced. */
static unsigned default_intrinsic_argument_types[5];
typedef struct IntrinsicArgumentRecord {
    short intrinsic;
    unsigned short reserved;
    unsigned types[6];
} IntrinsicArgumentRecord;
typedef char IntrinsicArgumentRecordSizeCheck[sizeof(IntrinsicArgumentRecord) == 28 ? 1 : -1];
static const IntrinsicArgumentRecord intrinsic_argument_records[] = {
    { INTRINSIC_AGGREGATE_COPY, 0, { 0x104, 0x404, 0x404, 0, 0, 0 } },
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

/* Original [00063810,0006383a). Argument containers already carry their
   normalized intrinsic type. Return the greatest low-byte width in the list. */
unsigned short __fastcall intrinsic_argument_width(Node *node, int skip_first)
{
    Node *argument;
    unsigned width = 0, bytes;
    if (skip_first) node = node->second.node;
    for (argument = node->second.node; argument->opcode != 0x4c;
         argument = argument->second.node) {
        bytes = argument->attributes.words.type & 255;
        if (bytes > width) width = bytes;
    }
    return (unsigned short)width;
}

extern int optimize_global, optimize_precision, optimize_speed;
SymbolHeader *__fastcall require_external_symbol(char *name);
/* Original [00063cf0,00063f20), including24 dispatch words63ec0..63f20.
   Keep supported floating operations inline; other entries name their generated
   runtime and describe its floating arguments/results to the call selector. */
void __fastcall prepare_generated_intrinsic(Node *node, int mode)
{
    unsigned intrinsic = node->left.node->left.value;
    const IntrinsicDescription *description;
    const unsigned char *runtime;
    char name[20], *out;
    unsigned char value, call_flags;
    unsigned short result = 0, arguments = 0;
    Node *argument;
    FunctionSymbol *symbol;
    if ((node->attributes.words.type & 0xff00) == 0x1000 &&
        ((optimize_global && !optimize_precision) || intrinsic == 0x47)) {
        switch (intrinsic) {
        case 0x47: case 0x5c: case 0x5d: case 0x64: case 0x65:
        case 0x68: case 0x69: case 0x6a: case 0x6b:
        case 0x6e: case 0x6f: case 0x72: case 0x73:
        case 0x82: case 0x83: case 0x84: case 0x85:
            return;
        case 0x60: case 0x61:
            if (optimize_speed) return;
            break;
        }
    }
    SELECT_INTRINSIC_DESCRIPTION(intrinsic, description);
    runtime = (const unsigned char *)description->runtime_name;
    name[0] = '_'; name[1] = '_';
    name[2] = floating_frontend ? 'F' : 'C';
    name[3] = 'I';
    out = name + 4;
    if (mode) {
        if (floating_frontend) {
            while (*runtime != 255) *out++ = (char)*runtime++;
            *out = 0;
        } else {
            while (*runtime != 255) ++runtime;
            ++runtime;
            do { value = *runtime++; *out++ = (char)value; } while (value);
        }
    } else {
        do { value = *runtime++; *out++ = (char)value; } while (value);
    }
    node->left.node = intern_leaf(0x26, 0, require_external_symbol(name), intrinsic);
    if (node->opcode != 0x7a) return;
    call_flags = (unsigned char)(description->flags >> 8);
    if (call_flags & 8) result = 1;
    if (call_flags & 0x10) result = 2;
    if (node->attributes.words.type & 0x3000) {
        symbol = (FunctionSymbol *)node->left.node->right.symbol;
        symbol->field24 = (symbol->field24 & ~15U) |
            (node_type_kind(node->attributes.words.type) & 15);
        for (argument = node->second.node; argument->opcode != 0x4c;
             argument = argument->second.node)
            if (argument->left.node->attributes.words.type & 0x1000) ++arguments;
    }
    symbol = (FunctionSymbol *)node->left.node->right.symbol;
    set_generated_intrinsic_call(symbol, (unsigned short)(call_flags & 7), result, arguments);
}

/* Original [00063870,00063892). Free only the original callee/argument
   container nodes; their value trees are retained by the lowered expression. */
void __fastcall release_intrinsic_operands(Node *node)
{
    Node *argument, *next;
    release_node(node->left.node);
    for (argument = node->second.node; argument; argument = next) {
        next = argument->second.node;
        release_node(argument);
    }
}

/* Original838c8..838e0: the complete12-row intrinsic-to-tree mapping,
   including its FF/zero terminator. The following argument-type cache is
   a separate object and is not part of this table. */
enum IntrinsicTreeOperation {
    INTRINSIC_TREE_POWER = 7, INTRINSIC_TREE_ABSOLUTE = 0x84,
    INTRINSIC_TREE_ROTATE_RIGHT = 0xac, INTRINSIC_TREE_ROTATE_LEFT = 0xad
};
typedef struct IntrinsicTreeOperator {
    unsigned char intrinsic, operation;
} IntrinsicTreeOperator;
typedef char IntrinsicTreeOperatorSizeCheck[sizeof(IntrinsicTreeOperator) == 2 ? 1 : -1];
static const IntrinsicTreeOperator intrinsic_tree_operators[] = {
    {174, INTRINSIC_TREE_POWER}, {209, INTRINSIC_TREE_POWER},
    {13, INTRINSIC_TREE_ABSOLUTE}, {14, INTRINSIC_TREE_ABSOLUTE},
    {15, INTRINSIC_TREE_ABSOLUTE}, {16, INTRINSIC_TREE_ABSOLUTE},
    {17, INTRINSIC_TREE_ABSOLUTE},
    {159, INTRINSIC_TREE_ROTATE_LEFT}, {160, INTRINSIC_TREE_ROTATE_RIGHT},
    {161, INTRINSIC_TREE_ROTATE_LEFT}, {162, INTRINSIC_TREE_ROTATE_RIGHT},
    {255, 0}
};
/* Original [00063840,0006386f). An unknown ID returns the terminator's
   zero opcode, preserving the original lookup's fallback. */
unsigned __fastcall intrinsic_operator(unsigned intrinsic)
{
    unsigned index = 0;
    while (intrinsic_tree_operators[index].intrinsic != 255 &&
           intrinsic_tree_operators[index].intrinsic != intrinsic) ++index;
    return intrinsic_tree_operators[index].operation;
}

/* Original [00027b30,00027b60). Fixed four-argument fastcall: the two
   trailing words occupy stack argument slots and the callee returns RET8. */
void __fastcall set_generated_intrinsic_call(FunctionSymbol *symbol,
    unsigned short maximum_depth, unsigned short cleanup_count,
    unsigned short argument_count)
{
    FunctionFloatStackInfo *information;
    information = (FunctionFloatStackInfo *)arena_alloc(0, sizeof(*information));
    information->maximum_depth = (short)maximum_depth;
    information->cleanup_count = (short)cleanup_count;
    information->argument_count = (short)argument_count;
    symbol->field2c = (unsigned)information;
}
