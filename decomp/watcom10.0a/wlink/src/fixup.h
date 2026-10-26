#ifndef WLINK_FIXUP_H
#define WLINK_FIXUP_H

#include "link.h"
#include <recon.h>

#pragma pack(1)
typedef struct fixup_ref {
    struct fixup_ref *next;
    union {
        symbol *sym;
        segnode *seg;
        group_entry *group;
        void *ptr;
        unsigned long value;
    } u;
    dll_sym_info *import;
    address addr;
    unsigned long flags;
} fixup_ref;

typedef struct fixup_record {
    unsigned char *loc;
    address addr;
    unsigned long value, type, flags;
} fixup_record;
#pragma pack()
typedef char old_fixup_ref_is_22[(sizeof(fixup_ref) == 22) ? 1 : -1];
typedef char old_fixup_record_is_22[(sizeof(fixup_record) == 22) ? 1 : -1];

#pragma pack(1)
typedef struct pending_reloc {
    struct pending_reloc *next;
    unsigned long length;
    unsigned long span, offset;
    unsigned char flags, reserved[3];
    unsigned char data[16];
} pending_reloc;
#pragma pack()
typedef char old_pending_reloc_is_36[(sizeof(pending_reloc) == 36) ? 1 : -1];


#endif
