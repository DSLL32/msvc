#include <string.h>
#include "compiler-state.h"
#include <recon-tool.h>
#include <recon.h>
/* Native-only typed fixture references to the genuine thiscall methods. */
extern void RECON_THISCALL alias_insert(CompilerIncludeAliases *, CompilerIncludeAlias *)
    __asm__("_ZN22CompilerIncludeAliases6InsertEP20CompilerIncludeAlias");
extern const char *RECON_THISCALL alias_find(CompilerIncludeAliases *, const char *, int *)
    __asm__("_ZN22CompilerIncludeAliases4FindEPKcPi");
extern char *__cdecl duplicate_argument(const char *, int);
static int duplicate_extent(void)
{
    unsigned storage[16];
    BYTE *bytes = (BYTE *)storage;
    memset(storage, 0xa5, sizeof(storage));
    c1_bss.arenas[0].cursor = bytes;
    c1_bss.arenas[0].committed_limit = bytes + sizeof(storage);
    if (duplicate_argument("header", 0) != (char *)bytes ||
        strcmp((char *)bytes, "header") || c1_bss.arenas[0].cursor != bytes + 8 ||
        bytes[7] != 0xa5 || bytes[8] != 0xa5) return 0;
    if (duplicate_argument("", 0) != (char *)bytes + 8 ||
        c1_bss.arenas[0].cursor != bytes + 12 || bytes[8] || bytes[9] != 0xa5 ||
        bytes[11] != 0xa5 || bytes[12] != 0xa5 || bytes[63] != 0xa5) return 0;
    return 1;
}
static int alias_links(void)
{
    CompilerIncludeAliases aliases;
    CompilerIncludeAlias old_alias = { NULL, "legacy.h", "old.h", 1, 1 };
    CompilerIncludeAlias new_alias = { (CompilerIncludeAlias *)1, "legacy.h", "actual.h", 1, 0 };
    CompilerIncludeAlias angle_alias = { (CompilerIncludeAlias *)2, "legacy.h", "angle.h", 2, 2 };
    aliases.head = NULL;
    alias_insert(&aliases, &old_alias);
    alias_insert(&aliases, &new_alias);
    alias_insert(&aliases, &angle_alias);
    if (aliases.head != &angle_alias || angle_alias.next != &new_alias ||
        new_alias.next != &old_alias || old_alias.next || new_alias.kind != 1 ||
        new_alias.replacement_kind != 0) return 0;
    int kind = 1;
    if (alias_find(&aliases, "LEGACY.H", &kind) != new_alias.replacement || kind != 0) return 0;
    kind = 2;
    if (alias_find(&aliases, "legacy.h", &kind) != angle_alias.replacement || kind != 2) return 0;
    kind = 3;
    return !alias_find(&aliases, "legacy.h", &kind) && kind == 3 &&
        !strcmp(new_alias.name, "legacy.h") && !strcmp(old_alias.replacement, "old.h");
}
const ReconTest header_tests[2] = {
    { "arena string duplication preserves padding and advances on NUL", 1, duplicate_extent },
    { "include aliases retain links and select newest matching delimiter kind", 1, alias_links }
};
