/* Repair parent/end/next scope links [0040dd60,0040deee). */
#include "pack.h"
#include <recon.h>

typedef struct ScopeLinks {
    unsigned parent, end, next;
} ScopeLinks;

int link_symbol_scopes(unsigned char *buffer, unsigned size)
{
    unsigned char *record, *end = buffer + size;
    unsigned char *root = buffer + 4, *first = root, *previous;
    ScopeLinks *links;
    unsigned scope = 0;
    unsigned short kind;
    int depth = 0;
    for (record = first; record < end; record += *(unsigned short *)record + 2) {
        kind = *(unsigned short *)(record + 2);
        if (kind == 6) {
            if (depth > 0) {
                links = (ScopeLinks *)(buffer + scope + 4);
                links->end = record - buffer;
                scope = links->parent;
                --depth;
            } else puts("Unbalanced S_END record encountered");
        } else if ((kind >= 0x104 && kind <= 0x108) ||
                   (kind >= 0x206 && kind <= 0x208) ||
                   kind == 0x100a || kind == 0x100b ||
                   kind == 0x1010 || kind == 0x1011) {
            ((ScopeLinks *)(record + 4))->parent = scope;
            scope = record - buffer;
            ++depth;
        }
    }
    while (first < end && *(unsigned short *)(first + 2) == 5)
        first += *(unsigned short *)first + 2;
    while (root < end && *(unsigned short *)(root + 2) == 5) {
        previous = root;
        for (record = first; record < end; record += *(unsigned short *)record + 2) {
            kind = *(unsigned short *)(record + 2);
            if ((kind >= 0x104 && kind <= 0x106) || kind == 0x206 ||
                kind == 0x100a || kind == 0x100b) {
                links = (ScopeLinks *)(record + 4);
                if (!links->end) return 0;
                if (*(unsigned short *)(previous + 2) == 5)
                    *(unsigned *)(previous + 4) = record - buffer;
                else ((ScopeLinks *)(previous + 4))->next = record - buffer;
                previous = record;
            }
        }
        root += *(unsigned short *)root + 2;
    }
    return 1;
}
