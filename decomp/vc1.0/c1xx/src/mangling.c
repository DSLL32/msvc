/* Shared workspace and entry to C++ symbol decoration. */
#include "mangling.h"
#include "lexer.h"
#include "operators.h"
#include "keywords.h"
#include "cpp-context.h"
#include "ir.h"
#include <recon.h>

char decorated_name_workspace[DECORATED_NAME_WORKSPACE_BYTES]; /* original 0x97c38 */
int decorated_type_count; /* original 0x97c30: ten-entry type cache count */
int decorated_name_count; /* original 0x98040: ten-entry identifier cache count */
int mangle_name_only; /* original 0x98044 */
int allow_plain_symbol_names = 1; /* original 0xa0a6c */
unsigned mangle_error_sequence; /* original 0xa0a70 */
unsigned qualified_tag_depth; /* original 0xa0a78: active scope-name recursion */
typedef char DecoratedWorkspaceSizeMustBe992[
    (sizeof(decorated_name_workspace) == 992) ? 1 : -1];

char *__stdcall encode_literal_name_byte(char *cursor, signed char character);
unsigned __stdcall update_checksum(unsigned char *bytes, unsigned count, unsigned checksum);

/* Original 0x44350..0x44451. Literal names carry their byte count,
   checksum and a bounded content prefix when pooling or inline rules permit. */
char *__stdcall encode_string_literal_name(char *cursor, unsigned char *text,
    unsigned bytes, int wide)
{
    unsigned count, word;
    if (!options.option_Gf) {
        if (!function_symbol || !(function_symbol->storage[1] & 1)) return 0;
        if ((function_symbol->storage[0] & 7) == 3 &&
            !(function_symbol->declaration_flags & 0x400)) return 0;
    }
    *cursor++ = '?'; *cursor++ = '?'; *cursor++ = '_';
    *cursor++ = 'C'; *cursor++ = '@'; *cursor++ = '_';
    *cursor++ = wide ? '1' : '0';
    cursor = mangle_number(cursor, bytes);
    cursor = mangle_number(cursor, update_checksum(text, bytes, 0));
    if (wide) {
        /* The original uses the byte count as a WORD iteration count.
           Keep that loop. This unvisited wide path has no span/equality proof;
           a reached wide case must establish the actual defined owner span. */
        count = bytes > 20 ? 20 : bytes;
        while (count--) {
            word = *(unsigned short *)text;
            text += 2;
            cursor = encode_literal_name_byte(cursor, (signed char)(word >> 8));
            cursor = encode_literal_name_byte(cursor, (signed char)word);
        }
    } else {
        count = bytes > 32 ? 32 : bytes;
        while (count--)
            cursor = encode_literal_name_byte(cursor, (signed char)*text++);
    }
    *cursor++ = '@';
    return cursor;
}

char *__stdcall encode_string_literal_name(char *cursor, unsigned char *text,
    unsigned bytes, int wide);

/* Original 0x42aa0..0x42b0e. String names use the same decoration
   workspace and reset the same contextual reference caches as symbol names. */
char *__stdcall mangle_string_literal(ParserValue *literal, int wide)
{
    char *end;
    unsigned char *text;
    mangle_name_only = 0;
    decorated_type_count = 0;
    decorated_name_count = 0;
    text = (unsigned char *)literal->string.text;
    end = encode_string_literal_name(decorated_name_workspace,
        text, literal->string.length, wide);
    if (!end) return 0;
    if (end - decorated_name_workspace > DECORATED_NAME_LIMIT) {
        fatal_error(66);
        end = decorated_name_workspace + DECORATED_NAME_LIMIT;
    }
    *end = 0;
    return decorated_name_workspace;
}

/* Original 0x442f0..0x44341. Recursive scope names include ::
   after every inner component and a NUL after the final tag name. */
char *__stdcall format_qualified_tag_name(char *cursor, CppSymbol *tag)
{
    CppSymbol *parent;
    char *name;
    ++qualified_tag_depth;
    parent = next_member_context(tag);
    if (parent) cursor = format_qualified_tag_name(cursor, parent);
    name = tag->name;
    while ((*cursor = *name++) != 0) ++cursor;
    if (--qualified_tag_depth) {
        *cursor++ = ':';
        *cursor++ = ':';
    }
    return cursor;
}

/* Original 0x442b0..0x442e5, stdcall 8. Tag type encodings end with
   a separate @ after the identifier and any enclosing scopes. */
char *__stdcall mangle_tag_name(char *cursor, CppSymbol *tag)
{
    CppSymbol *parent;
    cursor = mangle_identifier(cursor, tag->name, 0);
    parent = next_member_context(tag);
    if (parent) cursor = mangle_symbol_scope(cursor, parent);
    *cursor++ = '@';
    return cursor;
}

/* Original 0x42b10..0x42bdb. Return the shared decorated string and,
   when requested, write it including its terminating NUL to the stream. */
char *__stdcall write_decorated_name(FILE *stream, CppSymbol *symbol,
    CppSymbol *context, int flags)
{
    char *cursor;
    int length;
    mangle_name_only = 0;
    decorated_type_count = 0;
    decorated_name_count = 0;
    cursor = decorated_name_workspace;
    if (flags) {
        *cursor++ = '?';
        *cursor++ = '@';
    }
    if ((symbol->kind & 0x1f) == 4)
        cursor = format_qualified_tag_name(cursor, symbol);
    else cursor = mangle_symbol_name(cursor, symbol, context);
    length = cursor - decorated_name_workspace;
    if (length > DECORATED_NAME_LIMIT) {
        if (length > 3 * DECORATED_NAME_LIMIT) fatal_error(66);
        else compiler_warning(1, 503, format_symbol_name(symbol));
        cursor = decorated_name_workspace + DECORATED_NAME_LIMIT;
    }
    *cursor = 0;
    if (stream) {
        fputs(decorated_name_workspace, stream);
        fputc(0, stream);
    }
    return decorated_name_workspace;
}

/* Original 0x42be0..0x42dd8. This entry owns the plain-name allowance
   while nested scope/type decoration is in progress, restoring it on exit. */
char *__stdcall mangle_symbol_name(char *cursor, CppSymbol *symbol, CppSymbol *context)
{
    int saved_plain_names = allow_plain_symbol_names;
    unsigned encoding, value, kind, convention;
    char *name;
    allow_plain_symbol_names = 0;
    encoding = classify_symbol_decoration(symbol);
    if (!error_count && encoding) {
        if (!context) context = symbol;
        *cursor++ = '?';
        if (encoding == 0x24) {
            context->declaration_flags &= ~0x300;
            context->storage[0] = (context->storage[0] & ~4) | 3;
            name = context->name + 1;
            while (*name && *name != '@') *cursor++ = *name++;
            *cursor++ = '@';
        } else cursor = mangle_symbol_identifier(cursor, symbol, encoding);
        cursor = mangle_symbol_scope(cursor, context);
        *cursor++ = '@';
        if (!mangle_name_only) {
            if (encoding == 0x24) {
                cursor = mangle_symbol_encoding(cursor, context, symbol, 0x24);
                context->declaration_flags = (context->declaration_flags & ~0x100) | 0x200;
                context->storage[0] = (context->storage[0] & ~6) | 1;
            } else cursor = mangle_symbol_encoding(cursor, symbol, 0, encoding);
        } else *cursor++ = '9';
    } else {
        name = symbol->name;
        if (!encoding || !error_count) {
            if (!saved_plain_names) {
                *cursor++ = '?';
                cursor = mangle_identifier(cursor, name, 0);
                *cursor++ = '@';
                *cursor++ = '9';
                mangle_name_only = 1;
            } else {
                /* The plain branch returns one byte after the copied NUL. */
                do { *cursor++ = *name; } while (*name++);
            }
            kind = symbol->kind & 0x1f;
            if (kind != 6 && kind != 7) goto finished;
            convention = symbol->type->attributes & 0x70;
            if (convention != 0x60 && convention != 0x40) goto finished;
            if (!(symbol->declaration_flags & 0x200) && symbol->name != name_main &&
                symbol->name != name_winmain && symbol->name != name_dllmain) goto finished;
            if (!(symbol->attributes & 0x4000)) cache_function_parameter_bytes(symbol);
            cursor[-1] = '@';
            value = symbol->field2c;
        } else {
            *cursor++ = ' ';
            do { *cursor++ = *name; } while (*name++);
            cursor[-1] = ' ';
            value = mangle_error_sequence++;
        }
        cursor += format_integer(value, cursor, 10);
    }
finished:
    allow_plain_symbol_names = saved_plain_names;
    return cursor;
}

/* This compiler does not accept nested-member offsetof as an array bound.
   Four ordered byte fields in a four-byte struct have no intervening padding;
   combined with cpp+48, this proves token+49, flags+50 and qualifiers+51. */
typedef char MangledCppOffsetMustBe48[(offsetof(CppSymbol, cpp) == 48) ? 1 : -1];
typedef char MangledCppByteLayoutMustMatch[
    (sizeof(((CppSymbolState *)0)->parts) == 4 &&
     sizeof(((CppSymbolState *)0)->parts.low_flags) == 1 &&
     sizeof(((CppSymbolState *)0)->parts.token) == 1 &&
     sizeof(((CppSymbolState *)0)->parts.member_flags) == 1 &&
     sizeof(((CppSymbolState *)0)->parts.member_qualifiers) == 1) ? 1 : -1];

/* Original 0x42de0..0x4320f; operator dispatch 0x43210..0x433a4.
   The low byte describes storage/member form; the high byte optionally
   selects an operator or compiler-generated name. Bit 7 adds its '_' form. */
unsigned __stdcall classify_symbol_decoration(CppSymbol *symbol)
{
    TypeShape *shape;
    char *name;
    unsigned encoding = 0, kind, storage, access, member;
    unsigned char special = 0;
    unsigned short declaration_flags;
    if (symbol->declaration_flags & 0x200) return 0;
    shape = symbol->type->related;
    if (shape && (shape->flags & 0x77) == 4) {
        /* Retain the native inlined prefix test within this function test. */
        if (shape && !(shape->flags & 0x77)) shape = shape->related;
        if (!shape->data.word && symbol->name != name_static_destructor) return 0;
    }
    name = symbol->name;
    kind = symbol->kind & 0x1f;
    storage = symbol->storage[0] & 7;
    access = symbol->flags & 3;
    if ((name == name_main || name == name_winmain || name == name_dllmain) &&
        kind != 3 && (kind == 6 || kind == 7) && storage != 3) return 0;
    if (storage == 7) return 0;
    if (kind != 6 && kind != 7) {
        if (name == name_vftable) {
            special = 0x80 | '7'; encoding = 0x36;
        } else if (name == name_vbtable) {
            special = 0x80 | '8'; encoding = 0x37;
        } else if (name == name_vdispmap) {
            special = 0x80 | 'K'; encoding = 0x3a;
        } else if (kind == 3) {
            if (storage == 3) {
                encoding = 0x30;
                if (access == 2) encoding = 0x31;
                else if (access == 3) encoding = 0x32;
            }
        } else {
            if (name != name_static_guard && (kind != 9 || (storage > 3 && storage < 7)))
                return 0;
            if (storage == 3) {
                if (!symbol->detail.scope_number) return 0;
                if (name == name_static_guard) {
                    special = 0x80 | 'B'; encoding = 0x35;
                } else encoding = 0x34;
            } else encoding = 0x33;
        }
        goto packed;
    }
    member = symbol->cpp.parts.member_qualifiers & 6;
    if (member == 2 || member == 4) encoding = member == 4 ? 0x1a : 6;
    declaration_flags = symbol->declaration_flags;
    if (declaration_flags & 0x1000) special = '0';
    else if (declaration_flags & 0x2000) special = '1';
    else if (declaration_flags & 0x4000) special = 'B';
    else if (member == 6) {
        encoding = 0x39;
        special = 0x80 | '9';
    } else if (name == name_default_ctor_closure) special = 0x80 | 'F';
    else if (name == name_vector_ctor) special = 0x80 | 'H';
    else if (name == name_vector_ctor_vb) special = 0x80 | 'J';
    else if (name == name_vector_dtor) special = 0x80 | 'I';
    else if (name == name_vbase_dtor) special = 0x80 | 'D';
    else if (name == name_delete_dtor) special = 0x80 | 'G';
    else if (name == name_vector_delete_dtor) special = 0x80 | 'E';
    else if (!symbol->cpp.parts.token || symbol->cpp.parts.token == 1) {
        if (name == name_static_destructor) encoding = 0x24;
    } else {
        switch (symbol->cpp.parts.token) {
        case OP_BIT_AND: special = 'I'; break;
        case OP_LOGICAL_AND: special = 'V'; break;
        case OP_AND_ASSIGN: special = 0x80 | '4'; break;
        case OP_ASSIGN: special = '4'; break;
        case 0x40: special = 'Q'; break; /* comma */
        case OP_POST_DECREMENT: case OP_PRE_DECREMENT: special = 'F'; break;
        case OP_DIVIDE: special = 'K'; break;
        case OP_DIVIDE_ASSIGN: special = 0x80 | '0'; break;
        case OP_EQUAL: special = '8'; break;
        case OP_NOT: special = '7'; break;
        case OP_DEREFERENCE: case OP_MULTIPLY: special = 'D'; break;
        case OP_GREATER: special = 'O'; break;
        case OP_GREATER_EQUAL: special = 'P'; break;
        case OP_POST_INCREMENT: case OP_PRE_INCREMENT: special = 'E'; break;
        case OP_SUBSCRIPT: special = 'A'; break;
        case OP_SHIFT_LEFT_ASSIGN: special = 0x80 | '3'; break;
        case OP_SHIFT_LEFT: special = '6'; break;
        case OP_LESS: special = 'M'; break;
        case OP_LESS_EQUAL: special = 'N'; break;
        case OP_SUBTRACT: case OP_NEGATE: special = 'G'; break;
        case OP_SUBTRACT_ASSIGN: special = 'Z'; break;
        case OP_REMAINDER: special = 'L'; break;
        case OP_REMAINDER_ASSIGN: special = 0x80 | '1'; break;
        case OP_MULTIPLY_ASSIGN: special = 'X'; break;
        case OP_NOT_EQUAL: special = '9'; break;
        case OP_BIT_OR: special = 'U'; break;
        case OP_OR_ASSIGN: special = 0x80 | '5'; break;
        case OP_LOGICAL_OR: special = 'W'; break;
        case OP_ADD: case OP_UNARY_PLUS: special = 'H'; break;
        case OP_ADD_ASSIGN: special = 'Y'; break;
        case OP_ARROW: special = 'C'; break;
        case OP_SHIFT_RIGHT_ASSIGN: special = 0x80 | '2'; break;
        case OP_SHIFT_RIGHT: special = '5'; break;
        case OP_COMPLEMENT: special = 'S'; break;
        case OP_BIT_XOR: special = 'T'; break;
        case OP_XOR_ASSIGN: special = 0x80 | '6'; break;
        case OP_CALL: special = 'R'; break;
        case TOK_NEW: special = '2'; break;
        case TOK_DELETE: special = '3'; break;
        case OP_MEMBER_POINTER: special = 'J'; break;
        default: special = 0x80 | '@'; break;
        }
    }
    if (encoding == 0x1a) {
        if (access == 2) encoding += 2;
        else if (access == 3) encoding += 4;
    } else if (encoding == 6 || (!encoding && (declaration_flags & 0x400))) {
        if (!encoding) {
            if (symbol->storage[1] & 2) encoding = 4;
            else encoding = (symbol->cpp.parts.member_flags & 1) ? 0 : 2;
        }
        if (access == 2) encoding |= 8;
        else if (access == 3) encoding |= 0x10;
    } else if (!encoding) encoding = 0x18;
    if (encoding != 0x24) {
        if (encoding != 0x39) encoding += 0x41;
        shape = symbol->type->related;
        if (!(shape->flags & 0x77) && (encoding == 0x39 || !(shape->flags & 0x400)))
            encoding |= 0x80;
    }
packed:
    return encoding | ((unsigned)special << 8);
}

/* Original 0x433b0..0x433eb. Ordinary names use the name-reference
   cache; encoded operators write their ordinary or underscore extension code. */
char *__stdcall mangle_symbol_identifier(char *cursor, CppSymbol *symbol, unsigned encoding)
{
    unsigned char operation = (unsigned char)(encoding >> 8);
    if (operation) {
        *cursor++ = '?';
        if (encoding & 0x8000) *cursor++ = '_';
        *cursor++ = operation & 0x7f;
        return cursor;
    }
    return mangle_identifier(cursor, symbol->name, 0);
}

/* Original 0x43490..0x43549. Class names recurse through enclosing
   contexts; block statics encode their scope number and containing function. */
char *__stdcall mangle_symbol_scope(char *cursor, CppSymbol *symbol)
{
    CppSymbol *parent;
    if ((symbol->kind & 31) == 4) {
        cursor = mangle_identifier(cursor, symbol->name, 0);
        parent = next_member_context(symbol);
        if (parent) return mangle_symbol_scope(cursor, parent);
    } else if (symbol->declaration_flags & 0x400)
        return mangle_symbol_scope(cursor, cpp_context_stack->items[symbol->auxiliary.declaring_class]);
    else if ((symbol->storage[0] & 7) == 3 && symbol->detail.scope_number) {
        *cursor++ = '?';
        cursor = mangle_number(cursor, symbol->detail.scope_number);
        *cursor++ = '?';
        return mangle_symbol_name(cursor, function_symbol, 0);
    }
    return cursor;
}

char *decorated_names[10]; /* original 0x98018..0x9803f */
typedef char DecoratedNameCacheSizeMustBe40[(sizeof(decorated_names) == 40) ? 1 : -1];

/* Original 0x433f0..0x4348c. Name references are interned-pointer
   identities, not string comparisons. New names end in '@'; later uses
   abbreviate one of the first ten names with a single decimal digit. */
char *__stdcall mangle_identifier(char *cursor, char *name, int convert_leading_dot)
{
    int index;
    for (index = 0; index < decorated_name_count; ++index) {
        if (decorated_names[index] == name) {
            *cursor++ = (char)('0' + index);
            return cursor;
        }
    }
    if (decorated_name_count < 10) decorated_names[decorated_name_count++] = name;
    if (convert_leading_dot) {
        *cursor = *name++;
        if (*cursor == '.') *cursor = '@';
        ++cursor;
    }
    while ((*cursor = *name++) != 0) ++cursor;
    *cursor++ = '@';
    return cursor;
}

/* Original code 0x43550..0x43712, dispatch table 0x43714..0x437ff.
   Emit the storage/member form, then the appropriate function, object,
   guard, virtual-table or compiler-generated displacement encoding. The
   context argument retains its original ABI slot but is not read here. */
char *__stdcall mangle_symbol_encoding(char *cursor, CppSymbol *symbol,
    CppSymbol *context, unsigned encoding)
{
    unsigned mode = encoding & 0x7f;
    unsigned char extended;
    TypeShape *shape;
    if (encoding & 0x80) *cursor++ = '_';
    if (mode == 0x24) { *cursor++ = '$'; *cursor++ = 'A'; }
    else if (mode == 0x39) { *cursor++ = '$'; *cursor++ = 'B'; }
    else if (mode == 0x3a) { *cursor++ = '$'; *cursor++ = 'C'; }
    else {
        extended = ((unsigned char)encoding + 0x3f) & 0x7f;
        if (extended >= 0x1a && extended < 0x20) {
            *cursor++ = '$';
            *cursor++ = extended + 0x16;
        } else *cursor++ = encoding & 0x7f;
    }
    if (encoding == 0x24) mode = 0x24;
    else {
        if (mode >= 0x41) encoding -= 0x41;
        if (encoding & 0x80)
            cursor = mangle_based_symbol(cursor, 5, symbol->type->related->data.based_symbol);
        mode = encoding & 0x7f;
        if (mode < 0x18) mode &= ~0x18U;
        if (mode >= 0x30 && mode < 0x33) mode &= ~3U;
        if (mode >= 0x1a && mode < 0x20) mode = 0x1a;
        if (mode < 0x30) mode &= ~1U;
    }
    switch (mode) {
    case 0x1a:
        cursor = mangle_number(cursor, symbol->detail.value);
    case 6:
        cursor = mangle_number(cursor, symbol->field28);
    case 0: case 4:
        shape = symbol->type->related;
        if (shape && !(shape->flags & 0x77)) shape = shape->related;
        cursor = mangle_type_qualifiers(cursor, shape->data.parameters->associated_type, 1);
    case 2: case 0x18:
        return mangle_function_type(cursor, symbol);
    case 0x24: case 0x30: case 0x33: case 0x34:
        cursor = mangle_type(cursor, strip_based_shape(symbol->type), 0);
        return mangle_type_qualifiers(cursor, strip_based_shape(symbol->type), 0);
    case 0x35:
        return mangle_number(cursor, symbol->detail.scope_number);
    case 0x36: case 0x37:
        cursor = mangle_type_qualifiers(cursor, strip_based_shape(symbol->type), 0);
        cursor = mangle_table_scope_path(cursor, symbol);
        *cursor++ = '@';
        return cursor;
    case 0x39:
        cursor = mangle_number(cursor, symbol->field28);
        return mangle_vcall_model(cursor, symbol);
    case 0x3a:
        cursor = mangle_symbol_scope(cursor, (CppSymbol *)symbol->auxiliary.value);
        *cursor++ = '@';
        return cursor;
    }
    return cursor;
}

/* Original 0x44280..0x44299. The generated vcall symbol stores
   its three-bit model in the existing member-qualifier byte. */
char *__stdcall mangle_vcall_model(char *cursor, CppSymbol *symbol)
{
    *cursor++ = (char)('A' + ((symbol->cpp.parts.member_qualifiers & 0x38) >> 3));
    return cursor;
}

/* Original 0x43880..0x439a7. Combine distance, cv and member-pointer
   qualifiers, then append the declaring class or based-pointer information. */
char *__stdcall mangle_type_qualifiers(char *cursor, CType *type, int object)
{
    TypeShape *shape = type->related, *inner;
    int direct_function = shape && (shape->flags & 0x77) == 4 && !shape->related;
    unsigned short attributes;
    unsigned code;
    if (direct_function || !shape || (shape->flags & 0x77) == 2) attributes = type->attributes;
    else attributes = shape->attributes;
    if (!(attributes & 15)) attributes |= (unsigned short)data_pointer_kind;
    code = ((shape && (shape->flags & 0x10)) || (attributes & 7) == 1 || !(attributes & 12)) ? 0 : 12;
    if (!direct_function) {
        if (object) attributes = type->attributes;
        else if (shape) {
            if ((shape->flags & 0x77) == 4) attributes = 0;
            else {
                inner = shape->related;
                if (inner) attributes = (inner->flags & 0x77) == 4 ? 0 : inner->attributes;
                else attributes = type->attributes;
            }
        }
    }
    if (attributes & 0x1000) code |= 1;
    if (attributes & 0x2000) code |= 2;
    if (shape && (shape->flags & 0x30)) code |= 0x10;
    *cursor++ = (char)(code < 0x19 ? code + 'A' : code + 0x17);
    if (code & 0x10) {
        cursor = mangle_symbol_scope(cursor, shape->data.tag_symbol);
        *cursor++ = '@';
    }
    if ((code & 12) == 12) return mangle_pointer_base(cursor, type);
    return cursor;
}

/* Original 0x43a40..0x43a87. Constructors and destructors omit the
   return type; all functions retain convention, parameters and terminator. */
char *__stdcall mangle_function_type(char *cursor, CppSymbol *function)
{
    cursor = mangle_calling_convention(cursor, function->type->related);
    if (!(function->declaration_flags & 0x3000))
        cursor = mangle_type(cursor, function->type, 0);
    else *cursor++ = '@';
    cursor = mangle_parameter_types(cursor, function->type);
    /* The original passes the type even though the terminator ignores it. */
    return mangle_function_terminator(cursor, function->type);
}

/* Original 0x43ac0..0x43b2c. Encode convention and far/interrupt modifier. */
char *__stdcall mangle_calling_convention(char *cursor, TypeShape *shape)
{
    unsigned short attributes;
    unsigned code;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    attributes = shape->attributes;
    switch (attributes & 0x70) {
    case 0x10: code = 0; break;
    case 0x20: code = 2; break;
    case 0x40: code = 8; break;
    case 0x60: code = 6; break;
    case 0x70: code = 4; break;
    default: code = 11; break;
    }
    if (code == 11) {
        if (attributes & 0x100) code = 10;
    } else if (attributes & 0x200) code |= 1;
    *cursor++ = (char)(code + 'A');
    return cursor;
}

/* Original 0x97c08..0x97c2f: adjacent to decorated_type_count. Entries
   retain canonical type identity, rather than comparing encoded strings. */
CType *decorated_types[10];
int mangle_parameter_depth; /* original 0xa0a74, dword inc/dec */
typedef char DecoratedTypeCacheSizeMustBe40[(sizeof(decorated_types) == 40) ? 1 : -1];

/* Original 0x43f90..0x44091. Encode a parameter list, sharing the first
   ten multi-character type encodings through single decimal references. */
char *__stdcall mangle_parameter_types(char *cursor, CType *type)
{
    TypeShape *shape = type->related;
    DeclarationParameterInfo *parameters;
    CType **next, *parameter;
    int remaining, index, variadic = 0;
    char *end;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    ++mangle_parameter_depth;
    parameters = shape->data.parameters->declarations;
    remaining = parameters->count;
    next = parameters->types;
    if (remaining == 1 && *next && !(*next)->related && ((*next)->flags & 0x80))
        *cursor = 'X';
    else {
        while (remaining--) {
            if (variadic) break;
            parameter = *next;
            if (!parameter) variadic = 1;
            else {
                for (index = 0; index < decorated_type_count; ++index)
                    if (decorated_types[index] == parameter) break;
                if (index < decorated_type_count && decorated_types[index] == parameter) {
                    *cursor++ = (char)(index + '0');
                } else {
                    end = mangle_parameter_type(cursor, parameter);
                    if (decorated_type_count < 10 && end - cursor > 1)
                        decorated_types[decorated_type_count++] = parameter;
                    cursor = end;
                }
                ++next;
            }
        }
        *cursor = variadic ? 'Z' : '@';
    }
    --mangle_parameter_depth;
    return cursor + 1;
}

/* Original 0x43c40..0x43c8d. References have an A/B cv prefix before
   their target; other parameter shapes use the ordinary type encoder. */
char *__stdcall mangle_parameter_type(char *cursor, CType *type)
{
    TypeShape *shape = type->related;
    if (shape && ((shape->flags & 0x77) == 0x40 || (shape->flags & 0x77) == 0x44)) {
        *cursor++ = (shape->attributes & 0x2000) ? 'B' : 'A';
        return mangle_referenced_type(cursor, type);
    }
    return mangle_type_shape(cursor, type);
}

/* Original 0x43c90..0x43ce1, stdcall 8. References to objects encode
   qualifiers before their referent; function references use signature form. */
char *__stdcall mangle_referenced_type(char *cursor, CType *type)
{
    if ((type->related->flags & 0x77) == 0x40) {
        cursor = mangle_type_qualifiers(cursor, type, 0);
        type = drop_first_type_shape(type);
        return mangle_pointee_shape(cursor, type);
    }
    cursor = mangle_function_pointer_prefix(cursor, type);
    type = drop_first_type_shape(type);
    return mangle_function_pointer_signature(cursor, type);
}

/* Original 0x43cf0..0x43d23, stdcall 8. Array referents add a Y
   marker before dimensions; other referents use the ordinary type shape. */
char *__stdcall mangle_pointee_shape(char *cursor, CType *type)
{
    if (type->related && (type->related->flags & 0x77) == 2) {
        *cursor++ = 'Y';
        return mangle_array_type(cursor, type);
    }
    return mangle_type_shape(cursor, type);
}

/* Original 0x43d30..0x43eef. Basic types use single letters; pointer
   shapes append their target and tagged types append their scoped name. */
char *__stdcall mangle_type_shape(char *cursor, CType *type)
{
    TypeShape *shape = type->related;
    unsigned short flags = type->flags, kind;
    unsigned char code = 0;
    if (shape) {
        kind = shape->flags & 0x77;
        if (kind != 1 && kind != 2 && kind != 0x10 && kind != 0x20 && kind != 3)
            return cursor;
        code = (shape->attributes & 0x1000) || kind == 2 ? 'Q' : 'P';
        if (shape->attributes & 0x2000) code |= 2;
        *cursor++ = code;
        return mangle_pointer_target(cursor, type);
    }
    if (flags & 0x9c0) {
        if (flags & 0x40) {
            if (flags & 0x20) code = 'H';
            else switch (flags & 15) {
            case 1: code = (flags & 0x8000) ? 'C' : 'D'; break;
            case 2: code = 'F'; break;
            case 4: code = 'J'; break;
            }
            if (flags & 0x4000) code |= 1;
        } else if (flags & 0x100) {
            switch (flags) {
            case 0x104: code = 'M'; break;
            case 0x108: code = 'N'; break;
            case 0x128: code = 'O'; break;
            }
        } else code = (flags & 0x800) ? 'L' : 'Z';
        *cursor++ = code;
        return cursor;
    }
    if ((flags & 0x408) == 0x408) *cursor++ = 'V';
    else if ((flags & 0x402) == 0x402) *cursor++ = 'U';
    else if ((flags & 0x404) == 0x404) *cursor++ = 'T';
    else if (flags & 0x200) {
        *cursor++ = 'W';
        /* The second argument is present in the original ABI. */
        cursor = mangle_enum_representation(cursor, type);
    } else return cursor;
    return mangle_tag_name(cursor, cpp_context_stack->items[type->field08]);
}

/* Original 0x442a0..0x442aa. RET 8 retains the unused type argument. */
char *__stdcall mangle_function_terminator(char *cursor, CType *unused_type)
{
    *cursor++ = 'Z';
    return cursor;
}

/* Original 0x43b60..0x43c36. Encode an argument or result type, including
   object cv qualifiers that are otherwise absent from primitive encodings. */
char *__stdcall mangle_type(char *cursor, CType *type, int parameter)
{
    TypeShape *shape;
    unsigned char qualifiers;
    int is_void = 0;
    type = strip_based_shape(type);
    shape = type->related;
    if ((parameter && !shape) ||
        (!parameter && shape && !shape->related && (shape->flags & 0x77) == 4)) {
        if (type->flags & 0x80) {
            is_void = 1;
            *cursor++ = 'X';
        } else if (!(type->flags & 0x140) && !(type->flags & 0x800)) {
            *cursor++ = '?';
            cursor = mangle_type_qualifiers(cursor, type, 0);
        } else {
            qualifiers = (type->attributes & 0x1000) != 0;
            if (type->attributes & 0x2000) qualifiers |= 2;
            if (qualifiers) {
                *cursor++ = '?';
                *cursor++ = qualifiers + 'A';
            }
        }
    }
    if (!is_void) {
        if (!parameter && type->related && (type->related->flags & 0x77) == 4)
            type = drop_first_type_shape(type);
        cursor = mangle_parameter_type(cursor, type);
    }
    return cursor;
}

/* Original 0x561e0..0x5620b. Name a generated external symbol in the
   supplied owner context, then retain the decorated spelling in the interner. */
char *__stdcall intern_generated_symbol_name(CppSymbol *symbol, CppSymbol *owner)
{
    symbol->declaration_flags &= ~0x300;
    symbol->storage[0] = (symbol->storage[0] & ~5) | 2;
    return intern_identifier(write_decorated_name(0, symbol, owner, 0));
}

Vector *__stdcall virtual_table_owners(VirtualTable *table);

/* Original 0x44220..0x44279. Shared table paths append every owner scope
   and its separator. A declaration without an associated table adds no path. */
char *__stdcall mangle_table_scope_path(char *cursor, CppSymbol *symbol)
{
    Vector *owners;
    short index;
    CppSymbol *owner;
    if (!symbol->auxiliary.virtual_table) return cursor;
    owners = virtual_table_owners(symbol->auxiliary.virtual_table);
    index = 0;
    while (index < owners->count) {
        owner = owners->items[index++];
        cursor = mangle_symbol_scope(cursor, owner);
        *cursor++ = '@';
    }
    return cursor;
}

/* Original 0x43a90..0x43abd, stdcall8. A function pointer's
   stripped descriptor has the same convention, return and parameter encodings
   as an ordinary function, followed by its common signature terminator. */
char *__stdcall mangle_function_pointer_signature(char *cursor, CType *type)
{
    cursor = mangle_calling_convention(cursor, type->related);
    cursor = mangle_type(cursor, type, 0);
    cursor = mangle_parameter_types(cursor, type);
    return mangle_function_terminator(cursor, type);
}

/* Original 0x439b0..0x43a3d, stdcall8. Function and member-function
   pointers carry the distance/based form before their signature. A member
   pointer also names its declaring class with a terminating scope separator. */
char *__stdcall mangle_function_pointer_prefix(char *cursor, CType *type)
{
    TypeShape *shape = type->related;
    unsigned short kind = shape->flags & 0x77;
    unsigned char code = 0;
    if (kind != 3 && kind != 0x20 && kind != 0x44) return cursor;
    if (shape->attributes & 12) code = 4;
    if (shape->flags & 0x20) code |= 2;
    if (code < 4) *cursor++ = (char)(code + '6');
    else {
        *cursor++ = '_';
        *cursor++ = (char)(code + 'A');
    }
    if (shape->flags & 0x20) {
        cursor = mangle_symbol_scope(cursor, shape->data.tag_symbol);
        *cursor++ = '@';
    }
    if (code & 4) return mangle_pointer_base(cursor, type);
    return cursor;
}

/* Original 0x43ef0..0x43f51, stdcall8. Data pointers carry their
   qualifier prefix; function/member pointers carry a function prefix and
   the stripped function descriptor's full signature. */
char *__stdcall mangle_pointer_target(char *cursor, CType *type)
{
    unsigned short kind = type->related->flags & 0x77;
    if (kind == 1 || kind == 2 || kind == 0x10) {
        cursor = mangle_type_qualifiers(cursor, type, 0);
        return mangle_pointee_type(cursor, drop_first_type_shape(type));
    }
    cursor = mangle_function_pointer_prefix(cursor, type);
    return mangle_function_pointer_signature(cursor, drop_first_type_shape(type));
}

/* Original 0x43f60..0x43f87, stdcall8. An unshaped void pointee has
   its own encoding; other pointees retain array and ordinary shape handling. */
char *__stdcall mangle_pointee_type(char *cursor, CType *type)
{
    if (!type->related && (type->flags & 0x80)) {
        *cursor++ = 'X';
        return cursor;
    }
    return mangle_pointee_shape(cursor, type);
}

/* Original 0x43800..0x4387d, stdcall8. Small positive numbers use
   decimal backreference digits; other values use A..P hexadecimal nibbles. */
char *__stdcall mangle_number(char *cursor, unsigned value)
{
    unsigned mask;
    int shift;
    if (!value) {
        *cursor++ = 'A';
        *cursor++ = '@';
        return cursor;
    }
    if (value <= 10) {
        *cursor++ = (char)(value + '0' - 1);
        return cursor;
    }
    mask = 0xf0000000;
    shift = 28;
    while (!(value & mask) && shift) {
        shift -= 4;
        mask >>= 4;
    }
    while (shift) {
        *cursor++ = (char)(((value & mask) >> shift) + 'A');
        shift -= 4;
        mask >>= 4;
    }
    *cursor++ = (char)((value & 15) + 'A');
    *cursor++ = '@';
    return cursor;
}

/* Original 0x1e370..0x1e39a, stdcall4. Conversion names are interned
   from a separate 1024-byte automatic encoding buffer. */
char *__stdcall encode_type_name(CType *type)
{
    char name[1024];
    mangle_type_identifier(name, type);
    return intern_identifier(name);
}

/* Original 0x43b30..0x43b5f, stdcall8. Type-only identifiers begin with
   a period and use fresh name/type backreference caches. */
char *__stdcall mangle_type_identifier(char *cursor, CType *type)
{
    decorated_type_count = 0;
    decorated_name_count = 0;
    *cursor++ = '.';
    cursor = mangle_type(cursor, type, 1);
    *cursor++ = '\0';
    return cursor;
}

/* Original 0x55fd0..0x55fec. Intern the decorated name of a static
   class member using its class as the explicitly supplied owner context. */
char *__stdcall mangle_static_member_name(CppSymbol *owner, CppSymbol *member)
{
    return intern_identifier(write_decorated_name(0, member, owner, 0));
}

/* Original 0x56490..0x564a8, stdcall4. Intern the decorated name of
   local static storage without a separate enclosing-symbol override. */
char *__stdcall make_local_static_name(CppSymbol *symbol)
{
    return intern_identifier(write_decorated_name(0, symbol, 0, 0));
}

unsigned __stdcall declaration_parameter_bytes(FunctionParameters *parameters);

/* Original0x44600..0x4462b, stdcall4. Cache the explicit signature's
   rounded stack size before plain stdcall-name decoration appends it. */
void __stdcall cache_function_parameter_bytes(CppSymbol *function)
{
    TypeShape *shape = function->type->related;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    function->field2c = declaration_parameter_bytes(shape->data.parameters);
    function->attributes |= 0x4000;
}
