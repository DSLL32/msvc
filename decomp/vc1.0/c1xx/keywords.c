/* Language and __declspec vocabularies reached by initialize_keyword_table. */
#include <string.h>
#include "keywords.h"

KeywordSpec keyword_specs[] = {
    { "auto", TOK_AUTO, {0,0,0} },
    { "break", TOK_BREAK, {0,0,0} },
    { "case", TOK_CASE, {0,0,0} },
    { "cdecl", TOK_CDECL, {0,0,0} },
    { "char", TOK_CHAR, {0,0,0} },
    { "class", TOK_CLASS, {0,0,0} },
    { "const", TOK_CONST, {0,0,0} },
    { "continue", TOK_CONTINUE, {0,0,0} },
    { "__declspec", TOK_DECLSPEC, {0,0,0} },
    { "default", TOK_DEFAULT, {0,0,0} },
    { "delete", TOK_DELETE, {0,0,0} },
    { "do", TOK_DO, {0,0,0} },
    { "double", TOK_DOUBLE, {0,0,0} },
    { "else", TOK_ELSE, {0,0,0} },
    { "enum", TOK_ENUM, {0,0,0} },
    { "extern", TOK_EXTERN, {0,0,0} },
    { "far", TOK_FAR, {0,0,0} },
    { "float", TOK_FLOAT, {0,0,0} },
    { "for", TOK_FOR, {0,0,0} },
    { "fortran", TOK_FORTRAN, {0,0,0} },
    { "friend", TOK_FRIEND, {0,0,0} },
    { "goto", TOK_GOTO, {0,0,0} },
    { "huge", TOK_HUGE, {0,0,0} },
    { "if", TOK_IF, {0,0,0} },
    { "inline", TOK_INLINE, {0,0,0} },
    { "__inline", TOK_INLINE, {0,0,0} },
    { "int", TOK_INT, {0,0,0} },
    { "long", TOK_LONG, {0,0,0} },
    { "near", TOK_NEAR, {0,0,0} },
    { "new", TOK_NEW, {0,0,0} },
    { "operator", TOK_OPERATOR, {0,0,0} },
    { "pascal", TOK_PASCAL, {0,0,0} },
    { "private", TOK_PRIVATE, {0,0,0} },
    { "protected", TOK_PROTECTED, {0,0,0} },
    { "public", TOK_PUBLIC, {0,0,0} },
    { "register", TOK_REGISTER, {0,0,0} },
    { "return", TOK_RETURN, {0,0,0} },
    { "short", TOK_SHORT, {0,0,0} },
    { "signed", TOK_SIGNED, {0,0,0} },
    { "sizeof", TOK_SIZEOF, {0,0,0} },
    { "static", TOK_STATIC, {0,0,0} },
    { "struct", TOK_STRUCT, {0,0,0} },
    { "switch", TOK_SWITCH, {0,0,0} },
    { "this", TOK_THIS, {0,0,0} },
    { "typedef", TOK_TYPEDEF, {0,0,0} },
    { "union", TOK_UNION, {0,0,0} },
    { "unsigned", TOK_UNSIGNED, {0,0,0} },
    { "virtual", TOK_VIRTUAL, {0,0,0} },
    { "void", TOK_VOID, {0,0,0} },
    { "volatile", TOK_VOLATILE, {0,0,0} },
    { "while", TOK_WHILE, {0,0,0} },
    { "__syscall", TOK_SYSCALL, {0,0,0} },
    { "__asm", TOK_MS_ASM, {0,0,0} },
    { "__near", TOK_NEAR, {0,0,0} },
    { "__far", TOK_FAR, {0,0,0} },
    { "__far16", TOK_FAR16, {0,0,0} },
    { "__huge", TOK_HUGE, {0,0,0} },
    { "__cdecl", TOK_CDECL, {0,0,0} },
    { "__fortran", TOK_FORTRAN, {0,0,0} },
    { "__pascal", TOK_PASCAL, {0,0,0} },
    { "__export", TOK_EXPORT, {0,0,0} },
    { "__based", TOK_BASED, {0,0,0} },
    { "__fastcall", TOK_FASTCALL, {0,0,0} },
    { "__except", TOK_EXCEPT, {0,0,0} },
    { "__finally", TOK_FINALLY, {0,0,0} },
    { "__stdcall", TOK_STDCALL, {0,0,0} },
    { "__try", TOK_SEH_TRY, {0,0,0} },
    { "__nctag", TOK_NCTAG, {0,0,0} },
    { "__novtordisp", TOK_NOVTORDISP, {0,0,0} },
    { "__thiscall", TOK_THISCALL, {0,0,0} },
    { "asm", TOK_ASM, {0,0,0} },
    { "catch", TOK_CATCH, {0,0,0} },
    { "template", TOK_TEMPLATE, {0,0,0} },
    { "throw", TOK_THROW, {0,0,0} },
    { "try", TOK_TRY, {0,0,0} },
    { "__resume", TOK_RESUME, {0,0,0} },
    { "__nounwind", TOK_NOUNWIND, {0,0,0} },
    { "__leave", TOK_LEAVE, {0,0,0} },
    { "__single_inheritance", TOK_SINGLE_INHERITANCE, {0,0,0} },
    { "__multiple_inheritance", TOK_MULTIPLE_INHERITANCE, {0,0,0} },
    { "__virtual_inheritance", TOK_VIRTUAL_INHERITANCE, {0,0,0} },
    { 0, 0, {0,0,0} }
};
DeclspecSpec declspec_specs[] = {
    { "dllimport", 6, 2, 0 },
    { "dllexport", 7, 3, 0 },
    { "naked", 4, 1, 0 },
    { "thread", 5, 1, 0 },
    { 0, 0, 0, 0 }
};

/* Original 0x1f380..0x1f439. */
void initialize_keyword_table(void)
{
    KeywordSpec *keyword;
    DeclspecSpec *attribute;
    unsigned index;
    for (keyword = keyword_specs; keyword->name; ++keyword) {
        intern_keyword(keyword->name);
        KEYWORD_FROM_NAME(interned_name)->token = keyword->token;
        SYMBOL_FROM_NAME(interned_name)->flags |= 1;
        if (!strncmp(keyword->name, "__", 2) && (options.configuration.z_a || options.configuration.z_E)) {
            intern_keyword(keyword->name + 1);
            KEYWORD_FROM_NAME(interned_name)->token = keyword->token;
            SYMBOL_FROM_NAME(interned_name)->flags |= 1;
        }
    }
    for (index = 0, attribute = declspec_specs; attribute->name; ++attribute, ++index) {
        intern_keyword(attribute->name);
        SYMBOL_FROM_NAME(interned_name)->flags |= 0x20;
        if (!(SYMBOL_FROM_NAME(interned_name)->flags & 1))
            KEYWORD_FROM_NAME(interned_name)->token = (unsigned char)index;
    }
}


/* Original 0x1f440..0x1f46b. */
void __stdcall disable_keyword(char *name)
{
    char *stored_name = intern_identifier(name);
    PreprocessorSymbol *symbol = SYMBOL_FROM_NAME(stored_name);
    if (symbol->flags & 1) {
        if (options.configuration.z_a)
            KEYWORD_FROM_NAME(stored_name)->token = TOK_DISABLED_KEYWORD;
        else
            symbol->flags &= ~1;
    }
}
