/* Shared workspace and entry to C++ symbol decoration. */
#include "mangling.h"
#include "lexer.h"

char decorated_name_workspace[DECORATED_NAME_WORKSPACE_BYTES]; /* original 0x97c38 */
int decorated_type_count; /* original 0x97c30: ten-entry type cache count */
int decorated_name_count; /* original 0x98040: ten-entry identifier cache count */
int mangle_name_only; /* original 0x98044 */
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
