/* Shared workspace and entry to C++ symbol decoration. */
#include "mangling.h"
#include "lexer.h"
#include "operators.h"
#include "keywords.h"
#include <recon.h>

char decorated_name_workspace[DECORATED_NAME_WORKSPACE_BYTES]; /* original 0x97c38 */
int decorated_type_count; /* original 0x97c30: ten-entry type cache count */
int decorated_name_count; /* original 0x98040: ten-entry identifier cache count */
int mangle_name_only; /* original 0x98044 */
int allow_plain_symbol_names = 1; /* original 0xa0a6c */
unsigned mangle_error_sequence; /* original 0xa0a70 */
typedef char DecoratedWorkspaceSizeMustBe992[
    (sizeof(decorated_name_workspace) == 992) ? 1 : -1];

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
