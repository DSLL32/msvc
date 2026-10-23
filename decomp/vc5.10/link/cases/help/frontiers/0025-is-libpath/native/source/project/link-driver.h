#ifndef LINK_DRIVER_H
#define LINK_DRIVER_H
#include <windows.h>
#include <stdio.h>
#include <stddef.h>
#include <recon.h>

/* The context is a PE header plus linker options. Unread fields retain their
 * discovered extent until their users are reconstructed. */
struct LinkOptions {
    BYTE reserved124[8], emit_symbols, reserved12d[3];
    BYTE retain_relocations, reserved131, fixed_image, reserved133;
    BYTE discard_unreferenced, reserved135[2], write_map, reserved138[10];
    BYTE option142, reserved143[2], profile, rom, order;
    BYTE effective_reference_opt, reference_opt_specified;
    BYTE windowsce_emulation, windowsce_noconvert;
    DWORD reserved14c, debug_level, debug_formats;
    BYTE reserved158[56];
};

/* Genuine target thiscall: ECX carries this, and one stack DWORD is popped.
 * VC5 C cannot declare it. This POD has no implicit construction, destruction,
 * virtual dispatch, RTTI or exception machinery. Native calls also explicitly
 * use thiscall, rather than the Linux default C++ member convention. */
struct LinkContext {
    char database_signature[32];
    WORD database_type, database_flags;
    LinkContext *self;
    DWORD reserved028[2];
    IMAGE_FILE_HEADER file_header;
    IMAGE_OPTIONAL_HEADER optional_header;
    LinkOptions options;
    DWORD specified_switches, reserved194[8];
    BYTE reserved1b4[48];
    int is_vxd;
    BYTE reserved1e8[176];
    LinkContext *RECON_THISCALL Initialize(int vxd);
    int RECON_THISCALL FullLink(BYTE *pack_requested);
};
typedef char LinkOptionsSize[(sizeof(LinkOptions)==108)?1:-1];
typedef char LinkContextSize[(sizeof(LinkContext)==664)?1:-1];
typedef char LinkContextOptionalAt44[(offsetof(LinkContext,optional_header)==0x44)?1:-1];
typedef char LinkContextOptionsAt124[(offsetof(LinkContext,options)==0x124)?1:-1];
typedef char LinkContextVxdAt1e4[(offsetof(LinkContext,is_vxd)==0x1e4)?1:-1];
#endif
