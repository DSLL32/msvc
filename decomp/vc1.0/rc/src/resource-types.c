/* Resource type interning and predefined spellings. */
#include "resource.h"
#include <wchar.h>
#include <recon.h>

static struct ResourceType *resource_types;

/* Original 0x01009bb0..0x01009c46. */
struct ResourceType * __stdcall find_resource_type(WCHAR *name, WORD ordinal)
{
    struct ResourceType *type = resource_types;
    if (!type)
        resource_types = type = resource_alloc(sizeof(*type));
    else {
        for (;;) {
            if (name && type->name && !wcscmp(name, type->name))
                return type;
            if (!name && ordinal && type->ordinal == ordinal)
                return type;
            if (!type->next)
                break;
            type = type->next;
        }
        type->next = resource_alloc(sizeof(*type));
        type = type->next;
    }
    type->name = duplicate_wide(name);
    type->ordinal = ordinal;
    type->resource_count = 0;
    type->resources = 0;
    return type;
}

/* Original 0x01009d40..0x01009ddd. */
void __cdecl initialize_resource_types(void)
{
    find_resource_type(L"CURSOR", 12);
    find_resource_type(L"ICON", 14);
    find_resource_type(L"BITMAP", 2);
    find_resource_type(L"MENU", 4);
    find_resource_type(L"DIALOG", 5);
    find_resource_type(L"STRINGTABLE", 6);
    find_resource_type(L"FONTDIR", 7);
    find_resource_type(L"FONT", 8);
    find_resource_type(L"ACCELERATORS", 9);
    find_resource_type(L"RCDATA", 10);
    find_resource_type(L"MESSAGETABLE", 11);
    find_resource_type(L"VERSIONINFO", 16);
    find_resource_type(L"DLGINCLUDE", 17);
}

/* Original 0x01007050..0x0100708c. */
WCHAR * __stdcall duplicate_wide(WCHAR *text)
{
    WCHAR *copy;
    if (!text)
        return 0;
    copy = resource_alloc((wcslen(text) + 1) * sizeof(WCHAR));
    wcscpy(copy, text);
    return copy;
}
