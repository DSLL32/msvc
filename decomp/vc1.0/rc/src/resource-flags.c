/* Original 0x01009c50..0x01009d1c, switch table through 0x01009d3c. */
#include "tokens.h"

BOOL __stdcall parse_memory_flag(struct ResourceInfo *resource)
{
    switch (current_token.length) {
    case 65520: resource->memory_flags &= 0xefef; break; /* FIXED */
    case 65521: resource->memory_flags |= 0x10; break;   /* MOVEABLE */
    case 65522: resource->memory_flags |= 0x1030; break; /* DISCARDABLE */
    case 65523: resource->memory_flags &= ~0x40; break;  /* LOADONCALL */
    case 65524: resource->memory_flags |= 0x40; break;   /* PRELOAD */
    case 65525: resource->memory_flags |= 0x20; break;   /* PURE/SHARED */
    case 65526: resource->memory_flags &= 0xefdf; break; /* IMPURE/NONSHARED */
    case 65527:
        next_token(0);
        if (current_token.kind != TOKEN_EQUALS)
            parser_error("Missing '=' in EXSTYLE=<flags>", "");
        resource->extended_style = parse_style(0);
        break;
    default:
        return FALSE;
    }
    next_token(0);
    return TRUE;
}
