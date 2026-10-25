#include <string.h>
#include "frontend.h"
#include "keywords.h"
#pragma intrinsic(memcpy, memcmp)
#include <recon.h>

struct KeywordDescription { const char *name; BYTE token; };
static const KeywordDescription keywords[] = {
    { "auto", 8 },
    { "break", 98 },
    { "case", 99 },
    { "char", 16 },
    { "const", 47 },
    { "continue", 102 },
    { "default", 103 },
    { "do", 105 },
    { "double", 19 },
    { "else", 107 },
    { "enum", 27 },
    { "extern", 10 },
    { "float", 18 },
    { "for", 111 },
    { "goto", 113 },
    { "if", 114 },
    { "int", 17 },
    { "long", 21 },
    { "register", 11 },
    { "return", 125 },
    { "short", 20 },
    { "signed", 24 },
    { "sizeof", 129 },
    { "static", 9 },
    { "struct", 28 },
    { "switch", 132 },
    { "typedef", 12 },
    { "union", 29 },
    { "unsigned", 25 },
    { "void", 22 },
    { "volatile", 48 },
    { "while", 135 },
    { "__inline", 14 },
    { "__cdecl", 37 },
    { "__based", 44 },
    { "__stdcall", 130 },
    { "__declspec", 192 },
    { "asm", 175 },
    { "catch", 176 },
    { "class", 26 },
    { "const_cast", 227 },
    { "delete", 141 },
    { "dynamic_cast", 228 },
    { "explicit", 230 },
    { "friend", 13 },
    { "inline", 14 },
    { "mutable", 229 },
    { "namespace", 225 },
    { "new", 140 },
    { "operator", 151 },
    { "private", 137 },
    { "protected", 138 },
    { "public", 139 },
    { "reinterpret_cast", 220 },
    { "static_cast", 206 },
    { "template", 177 },
    { "this", 144 },
    { "throw", 178 },
    { "try", 179 },
    { "typeid", 121 },
    { "typename", 236 },
    { "using", 226 },
    { "virtual", 15 },
    { "__single_inheritance", 184 },
    { "__multiple_inheritance", 185 },
    { "__virtual_inheritance", 186 },
    { "__novtordisp", 174 },
    { "__uuidof", 239 },
    { "__resume", 243 },
    { "__nounwind", 243 },
    { "__syscall", 243 },
    { "__near", 243 },
    { "__far", 243 },
    { "__far16", 243 },
    { "__huge", 243 },
    { "__fortran", 243 },
    { "__export", 243 },
    { "__interrupt", 243 },
    { "__loadds", 243 },
    { "__saveregs", 243 },
    { "__segment", 243 },
    { "__segname", 243 },
    { "__self", 243 },
    { "__thiscall", 191 },
    { "cdecl", 37 },
    { "far", 243 },
    { "fortran", 243 },
    { "huge", 243 },
    { "near", 243 },
    { "pascal", 221 },
    { "__pascal", 222 },
    { "__try", 167 },
    { "__except", 168 },
    { "__finally", 169 },
    { "__leave", 181 },
    { "__asm", 42 },
    { "__fastcall", 38 },
    { "__unaligned", 193 },
    { "__builtin_alignof", 180 },
    { "__sysapi", 190 },
    { "__builtin_isfloat", 218 },
    { "__wchar_t", 217 },
    { "__restrict", 39 },
    { "__ptr64", 237 },
    { "__int8", 212 },
    { "__int16", 213 },
    { "__int32", 214 },
    { "__int64", 215 },
    { "__int128", 216 },
    { "wchar_t", 217 },
    { "and", 50 },
    { "and_eq", 51 },
    { "bitand", 49 },
    { "or", 79 },
    { "or_eq", 78 },
    { "bitor", 77 },
    { "xor", 91 },
    { "xor_eq", 92 },
    { "compl", 89 },
    { "not", 60 },
    { "not_eq", 76 },
    { "bool", 231 },
    { "true", 232 },
    { "false", 233 },
    { NULL, 0 }
};

static const char *declspec_names[] = {
    "dllimport",
    "dllexport",
    "naked",
    "thread",
    "allocate",
    "nothrow",
    "selectany",
    "property",
    "uuid",
    "implementation_key",
    "novtable",
    NULL
};

extern "C" {
void __cdecl intern_identifier(const char *);
void __cdecl unreserve_keyword(const char *);
void __cdecl set_keyword_category3(const char *);
void __cdecl set_keyword_category1(const char *);
void __cdecl set_keyword_category5(const char *);
void __cdecl set_keyword_category4(const char *);
void __cdecl report_language_diagnostic(unsigned, unsigned, ...);
}

extern "C" void __cdecl initialize_language_keywords(void)
{
    const KeywordDescription *entry;
    const char **attribute;
    unsigned code;
    frontend_data.options.language_extensions =
        frontend_data.options.language_extensions || frontend_data.options.option_zbe;
    frontend_data.option_zb /= 8;
    if (frontend_data.option_zb) {
        if (frontend_data.option_zb < 4 || frontend_data.option_zb > 8 ||
            (frontend_data.option_zb != 4 && frontend_data.option_zb != 8)) {
            report_language_diagnostic(1, 696, (unsigned)frontend_data.option_zb << 3, 32);
            frontend_data.option_zb = 4;
        }
    } else {
        frontend_data.option_zb = 4;
    }
    for (entry = keywords; entry->name; ++entry) {
        intern_identifier(entry->name);
        frontend_data.current_identifier->flags |= 0x10000;
        ((CompilerIdentifier *)frontend_data.current_identifier)->token = entry->token;
    }
    unreserve_keyword("__interrupt");
    unreserve_keyword("__loadds");
    unreserve_keyword("__saveregs");
    unreserve_keyword("__segment");
    unreserve_keyword("__segname");
    unreserve_keyword("__self");
    if (!frontend_data.options.language_extensions && !frontend_bss.options.option_zf)
        unreserve_keyword("cdecl");
    if (!frontend_data.options.language_extensions && !frontend_bss.options.option_zf)
        unreserve_keyword("far");
    if (!frontend_data.options.language_extensions && !frontend_bss.options.option_zf)
        unreserve_keyword("fortran");
    if (!frontend_data.options.language_extensions && !frontend_bss.options.option_zf)
        unreserve_keyword("huge");
    if (!frontend_data.options.language_extensions && !frontend_bss.options.option_zf)
        unreserve_keyword("near");
    if (!frontend_data.options.language_extensions && !frontend_bss.options.option_zf)
        unreserve_keyword("pascal");
    unreserve_keyword("wchar_t");
    if (frontend_data.options.language_extensions) unreserve_keyword("and");
    if (frontend_data.options.language_extensions) unreserve_keyword("and_eq");
    if (frontend_data.options.language_extensions) unreserve_keyword("bitand");
    if (frontend_data.options.language_extensions) unreserve_keyword("or");
    if (frontend_data.options.language_extensions) unreserve_keyword("or_eq");
    if (frontend_data.options.language_extensions) unreserve_keyword("bitor");
    if (frontend_data.options.language_extensions) unreserve_keyword("xor");
    if (frontend_data.options.language_extensions) unreserve_keyword("xor_eq");
    if (frontend_data.options.language_extensions) unreserve_keyword("compl");
    if (frontend_data.options.language_extensions) unreserve_keyword("not");
    if (frontend_data.options.language_extensions) unreserve_keyword("not_eq");
    if (frontend_data.options.no_bool || frontend_data.options.language_extensions)
        unreserve_keyword("bool");
    if (frontend_data.options.no_bool) {
        unreserve_keyword("true");
        if (frontend_data.options.no_bool) unreserve_keyword("false");
    }
    set_keyword_category3("__resume");
    set_keyword_category3("__nounwind");
    set_keyword_category1("__syscall");
    set_keyword_category1("__near");
    set_keyword_category1("__far");
    set_keyword_category1("__far16");
    set_keyword_category1("__huge");
    set_keyword_category1("__fortran");
    set_keyword_category5("__export");
    if (!frontend_bss.options.option_binl) set_keyword_category3("__thiscall");
    set_keyword_category1("far");
    set_keyword_category1("fortran");
    set_keyword_category1("huge");
    set_keyword_category1("near");
    set_keyword_category1("pascal");
    set_keyword_category1("__pascal");
    set_keyword_category4("__unaligned");
    set_keyword_category4("__builtin_alignof");
    set_keyword_category4("__sysapi");
    set_keyword_category4("__builtin_isfloat");
    set_keyword_category4("__wchar_t");
    set_keyword_category4("__restrict");
    set_keyword_category4("__ptr64");
    if (frontend_data.option_zb < 4) {
        set_keyword_category4("__int8");
        if (frontend_data.option_zb < 4) {
            set_keyword_category4("__int16");
            if (frontend_data.option_zb < 4) set_keyword_category4("__int32");
        }
    }
    if (frontend_data.option_zb < 8) set_keyword_category4("__int64");
    if (frontend_data.option_zb < 16) set_keyword_category4("__int128");
    if (frontend_data.options.language_extensions || frontend_data.options.preprocess_only) {
        for (entry = keywords; entry->name; ++entry) {
            BYTE token;
            if (strncmp(entry->name, "__", 2)) continue;
            intern_identifier(entry->name);
            if (!(frontend_data.current_identifier->flags & 0x10000)) continue;
            token = ((CompilerIdentifier *)frontend_data.current_identifier)->token;
            if (token != 243) {
                intern_identifier(entry->name + 1);
                frontend_data.current_identifier->flags |= 0x10000;
                ((CompilerIdentifier *)frontend_data.current_identifier)->token = token;
            } else {
                token = ((CompilerIdentifier *)frontend_data.current_identifier)->category;
                if (token != 1 && token != 5) continue;
                intern_identifier(entry->name + 1);
                frontend_data.current_identifier->flags |= 0x10000;
                ((CompilerIdentifier *)frontend_data.current_identifier)->token = 243;
                ((CompilerIdentifier *)frontend_data.current_identifier)->category = token;
            }
        }
    }
    for (attribute = declspec_names, code = 0; *attribute; ++attribute, ++code) {
        intern_identifier(*attribute);
        frontend_data.current_identifier->flags |= 0x800000;
        if (!(frontend_data.current_identifier->flags & 0x10000))
            ((CompilerIdentifier *)frontend_data.current_identifier)->token = (BYTE)code;
    }
}

extern "C" void __cdecl intern_identifier(const char *name)
{
    const unsigned char *text = (const unsigned char *)name;
    unsigned hash = 0, length, folded;
    CompilerNameEntry **bucket, **link, *entry;
    CompilerPool *pool = &frontend_bss.pools[6]; /* original c0560 */
    CompilerArena *arena = &frontend_bss.arenas[0];
    while (*text) {
        hash = (hash >> 4) + hash * 4 + *text;
        ++text;
    }
    length = (unsigned)(text - (const unsigned char *)name);
    folded = ((hash >> 16) ^ hash) & 0xffff;
    bucket = &frontend_data.keyword_table->buckets[hash & 0x7ff];
    link = bucket;
    for (entry = *link; entry; link = &entry->next, entry = *link) {
        if ((entry->flags & 0xffff) == folded && entry->name[length] == 0 &&
            memcmp(name, entry->name, length) == 0) {
            *link = entry->next;
            entry->next = *bucket;
            *bucket = entry;
            frontend_data.current_identifier = entry;
            return;
        }
    }
    if (pool->remaining < 20)
        pool->Refill(20);
    entry = (CompilerNameEntry *)pool->cursor;
    pool->cursor += 20;
    pool->remaining -= 20;
    entry->next = *bucket;
    *bucket = entry;
    entry->name = (char *)arena->cursor;
    arena->cursor += (length + 4) & ~3U;
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    memcpy(entry->name, name, length);
    entry->name[length] = 0;
    frontend_data.current_identifier = entry;
    entry->flags = (entry->flags & 0xffff0000) | folded;
}
extern "C" void __cdecl unreserve_keyword(const char *name)
{
    intern_identifier(name);
    frontend_data.current_identifier->flags &= ~0x10000U;
}
extern "C" void __cdecl set_keyword_category3(const char *name)
{
    intern_identifier(name);
    if (frontend_data.current_identifier->flags & 0x10000) {
        ((CompilerIdentifier *)frontend_data.current_identifier)->token = 243;
        ((CompilerIdentifier *)frontend_data.current_identifier)->category = 3;
    }
}
extern "C" void __cdecl set_keyword_category1(const char *name)
{
    intern_identifier(name);
    if (frontend_data.current_identifier->flags & 0x10000) {
        ((CompilerIdentifier *)frontend_data.current_identifier)->token = 243;
        ((CompilerIdentifier *)frontend_data.current_identifier)->category = 1;
    }
}
extern "C" void __cdecl set_keyword_category5(const char *name)
{
    intern_identifier(name);
    if (frontend_data.current_identifier->flags & 0x10000) {
        ((CompilerIdentifier *)frontend_data.current_identifier)->token = 243;
        ((CompilerIdentifier *)frontend_data.current_identifier)->category = 5;
    }
}
extern "C" void __cdecl set_keyword_category4(const char *name)
{
    intern_identifier(name);
    if (frontend_data.current_identifier->flags & 0x10000) {
        ((CompilerIdentifier *)frontend_data.current_identifier)->token = 243;
        ((CompilerIdentifier *)frontend_data.current_identifier)->category = 4;
    }
}
extern "C" void __cdecl report_language_diagnostic(unsigned level, unsigned number, ...) { __asm { int 3 } }

/* Returning lookup for generic 16-byte names. It shares only the common
 * header with 20-byte keyword records and never reads their extension. */
extern "C" CompilerNameEntry *__cdecl find_or_create_identifier(const char *name)
{
    const unsigned char *text = (const unsigned char *)name;
    unsigned hash = 0, length, folded;
    CompilerNameEntry **bucket, **link, *entry;
    CompilerPool *pool = &frontend_bss.pools[6]; /* own original c0560 */
    CompilerArena *arena = &frontend_bss.arenas[0];
    while (*text) {
        hash = (hash >> 4) + hash * 4 + *text;
        ++text;
    }
    length = (unsigned)(text - (const unsigned char *)name);
    folded = ((hash >> 16) ^ hash) & 0xffff;
    bucket = &frontend_data.keyword_table->buckets[hash & 0x7ff];
    link = bucket;
    for (entry = *link; entry; link = &entry->next, entry = *link) {
        if ((entry->flags & 0xffff) == folded && entry->name[length] == 0 &&
            memcmp(name, entry->name, length) == 0) {
            *link = entry->next;
            entry->next = *bucket;
            *bucket = entry;
            frontend_data.current_identifier = entry;
            return entry;
        }
    }
    if (pool->remaining < 16) pool->Refill(16);
    entry = (CompilerNameEntry *)pool->cursor;
    pool->cursor += 16;
    pool->remaining -= 16;
    entry->next = *bucket;
    *bucket = entry;
    entry->name = (char *)arena->cursor;
    arena->cursor += (length + 4) & ~3U;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    memcpy(entry->name, name, length);
    entry->name[length] = 0;
    frontend_data.current_identifier = entry;
    entry->flags = (entry->flags & 0xffff0000) | folded;
    return entry;
}
