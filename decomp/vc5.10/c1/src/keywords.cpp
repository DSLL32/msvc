#include <string.h>
#include "compiler-state.h"
#include "keywords.h"
#pragma intrinsic(memcpy, memcmp)
#include <recon.h>

/* Encountered keyword spellings and their byte token codes. Original rows
 * at RVA601c8 have stride8, ending with a NULL spelling. */
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
    { "bool", 243 },
    { "true", 243 },
    { "false", 243 },
    { NULL, 0 },
};

/* Only the name column of the original sixteen-byte declspec records is
 * consumed here. Their other fields belong to later declaration parsing. */
static const char *declspec_names[] = {
    "dllimport",
    "dllexport",
    "inline",
    "naked",
    "thread",
    "allocate",
    "selectany",
    NULL,
};

extern "C" {
void __cdecl intern_identifier(const char *);
void __cdecl unreserve_keyword(const char *);
void __cdecl set_keyword_category2(const char *);
void __cdecl set_keyword_category3(const char *);
void __cdecl set_keyword_category1(const char *);
void __cdecl set_keyword_category5(const char *);
void __cdecl set_keyword_category4(const char *);
void __cdecl set_keyword_category6(const char *);
void __cdecl report_language_diagnostic(unsigned, unsigned, ...);
}

extern "C" void __cdecl initialize_language_keywords(void)
{
    const KeywordDescription *entry;
    const char **attribute;
    unsigned code;
    c1_data.options.language_extensions =
        c1_data.options.language_extensions || c1_data.options.option_zbe;
    c1_data.internal_zb_value /= 8;
    if (c1_data.internal_zb_value) {
        if (c1_data.internal_zb_value < 4 || c1_data.internal_zb_value > 8 ||
            (c1_data.internal_zb_value != 4 && c1_data.internal_zb_value != 8)) {
            report_language_diagnostic(1, 696, (unsigned)c1_data.internal_zb_value << 3, 32);
            c1_data.internal_zb_value = 4;
        }
    } else {
        c1_data.internal_zb_value = 4;
    }
    for (entry = keywords; entry->name; ++entry) {
        intern_identifier(entry->name);
        c1_data.current_identifier->flags |= 0x10000;
        c1_data.current_identifier->token = entry->token;
    }
    unreserve_keyword("asm");
    unreserve_keyword("catch");
    unreserve_keyword("class");
    unreserve_keyword("const_cast");
    unreserve_keyword("delete");
    unreserve_keyword("dynamic_cast");
    unreserve_keyword("explicit");
    unreserve_keyword("friend");
    unreserve_keyword("inline");
    unreserve_keyword("mutable");
    unreserve_keyword("namespace");
    unreserve_keyword("new");
    unreserve_keyword("operator");
    unreserve_keyword("private");
    unreserve_keyword("protected");
    unreserve_keyword("public");
    unreserve_keyword("reinterpret_cast");
    unreserve_keyword("static_cast");
    unreserve_keyword("template");
    unreserve_keyword("this");
    unreserve_keyword("throw");
    unreserve_keyword("try");
    unreserve_keyword("typeid");
    unreserve_keyword("typename");
    unreserve_keyword("using");
    unreserve_keyword("virtual");
    unreserve_keyword("__interrupt");
    unreserve_keyword("__loadds");
    unreserve_keyword("__saveregs");
    unreserve_keyword("__segment");
    unreserve_keyword("__segname");
    unreserve_keyword("__self");
    if (!c1_data.options.language_extensions && !c1_bss.options.option_zf)
        unreserve_keyword("cdecl");
    if (!c1_data.options.language_extensions && !c1_bss.options.option_zf)
        unreserve_keyword("far");
    if (!c1_data.options.language_extensions && !c1_bss.options.option_zf)
        unreserve_keyword("fortran");
    if (!c1_data.options.language_extensions && !c1_bss.options.option_zf)
        unreserve_keyword("huge");
    if (!c1_data.options.language_extensions && !c1_bss.options.option_zf)
        unreserve_keyword("near");
    if (!c1_data.options.language_extensions && !c1_bss.options.option_zf)
        unreserve_keyword("pascal");
    unreserve_keyword("wchar_t");
    unreserve_keyword("and");
    unreserve_keyword("and_eq");
    unreserve_keyword("bitand");
    unreserve_keyword("or");
    unreserve_keyword("or_eq");
    unreserve_keyword("bitor");
    unreserve_keyword("xor");
    unreserve_keyword("xor_eq");
    unreserve_keyword("compl");
    unreserve_keyword("not");
    unreserve_keyword("not_eq");
    unreserve_keyword("bool");
    unreserve_keyword("true");
    unreserve_keyword("false");
    set_keyword_category2("__single_inheritance");
    set_keyword_category2("__multiple_inheritance");
    set_keyword_category2("__virtual_inheritance");
    set_keyword_category2("__novtordisp");
    set_keyword_category2("__uuidof");
    set_keyword_category3("__resume");
    set_keyword_category3("__nounwind");
    set_keyword_category1("__syscall");
    set_keyword_category1("__near");
    set_keyword_category1("__far");
    set_keyword_category1("__far16");
    set_keyword_category1("__huge");
    set_keyword_category1("__fortran");
    set_keyword_category5("__export");
    set_keyword_category3("__thiscall");
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
    if (c1_data.internal_zb_value < 4) {
        set_keyword_category4("__int8");
        if (c1_data.internal_zb_value < 4) {
            set_keyword_category4("__int16");
            if (c1_data.internal_zb_value < 4) set_keyword_category4("__int32");
        }
    }
    if (c1_data.internal_zb_value < 8) set_keyword_category4("__int64");
    if (c1_data.internal_zb_value < 16) set_keyword_category4("__int128");
    set_keyword_category6("bool");
    set_keyword_category6("true");
    set_keyword_category6("false");
    if (c1_data.options.language_extensions || c1_data.options.preprocess_only) {
        for (entry = keywords; entry->name; ++entry) {
            BYTE token;
            if (strncmp(entry->name, "__", 2)) continue;
            intern_identifier(entry->name);
            if (!(c1_data.current_identifier->flags & 0x10000)) continue;
            token = c1_data.current_identifier->token;
            if (token != 243) {
                intern_identifier(entry->name + 1);
                c1_data.current_identifier->flags |= 0x10000;
                c1_data.current_identifier->token = token;
            } else {
                token = c1_data.current_identifier->category;
                if (token != 1 && token != 5) continue;
                intern_identifier(entry->name + 1);
                c1_data.current_identifier->flags |= 0x10000;
                c1_data.current_identifier->token = 243;
                c1_data.current_identifier->category = token;
            }
        }
    }
    for (attribute = declspec_names, code = 0; *attribute; ++attribute, ++code) {
        intern_identifier(*attribute);
        c1_data.current_identifier->flags |= 0x800000;
        if (!(c1_data.current_identifier->flags & 0x10000))
            c1_data.current_identifier->token = (BYTE)code;
    }
}

extern "C" void __cdecl intern_identifier(const char *name)
{
    const unsigned char *text = (const unsigned char *)name;
    unsigned hash = 0, length, folded;
    CompilerNameEntry **bucket, **link, *entry;
    CompilerPool *pool = &c1_bss.pools[6];
    CompilerArena *arena = &c1_bss.arenas[0];
    while (*text) {
        hash = (hash >> 4) + hash * 4 + *text;
        ++text;
    }
    length = (unsigned)(text - (const unsigned char *)name);
    folded = ((hash >> 16) ^ hash) & 0xffff;
    bucket = &c1_data.name_buckets[hash & 0x7ff];
    link = bucket;
    for (entry = *link; entry; link = &entry->next, entry = *link) {
        if ((entry->flags & 0xffff) == folded && entry->name[length] == 0 &&
            memcmp(name, entry->name, length) == 0) {
            *link = entry->next;
            entry->next = *bucket;
            *bucket = entry;
            c1_data.current_identifier = entry;
            return;
        }
    }
    /* The compact view omits original DWORD12 and its allocation tail.
     * Preserve the actual20-byte pool stride, not sizeof the projection. */
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
    c1_data.current_identifier = entry;
    entry->flags = (entry->flags & 0xffff0000) | folded;
}
extern "C" void __cdecl unreserve_keyword(const char *name)
{
    intern_identifier(name);
    c1_data.current_identifier->flags &= ~0x10000U;
}
extern "C" void __cdecl set_keyword_category2(const char *name)
{
    intern_identifier(name);
    if (c1_data.current_identifier->flags & 0x10000) {
        c1_data.current_identifier->token = 243;
        c1_data.current_identifier->category = 2;
    }
}
extern "C" void __cdecl set_keyword_category3(const char *name)
{
    intern_identifier(name);
    if (c1_data.current_identifier->flags & 0x10000) {
        c1_data.current_identifier->token = 243;
        c1_data.current_identifier->category = 3;
    }
}
extern "C" void __cdecl set_keyword_category1(const char *name)
{
    intern_identifier(name);
    if (c1_data.current_identifier->flags & 0x10000) {
        c1_data.current_identifier->token = 243;
        c1_data.current_identifier->category = 1;
    }
}
extern "C" void __cdecl set_keyword_category5(const char *name)
{
    intern_identifier(name);
    if (c1_data.current_identifier->flags & 0x10000) {
        c1_data.current_identifier->token = 243;
        c1_data.current_identifier->category = 5;
    }
}
extern "C" void __cdecl set_keyword_category4(const char *name)
{
    intern_identifier(name);
    if (c1_data.current_identifier->flags & 0x10000) {
        c1_data.current_identifier->token = 243;
        c1_data.current_identifier->category = 4;
    }
}
extern "C" void __cdecl set_keyword_category6(const char *name)
{
    intern_identifier(name);
    if (c1_data.current_identifier->flags & 0x10000) {
        c1_data.current_identifier->token = 243;
        c1_data.current_identifier->category = 6;
    }
}
extern "C" void __cdecl report_language_diagnostic(unsigned level, unsigned number, ...) { __asm { int 3 } }
