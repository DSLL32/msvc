#ifndef LINK_DRIVER_H
#define LINK_DRIVER_H
#include <windows.h>
#include <stdio.h>
#include <stddef.h>
#include "symbols.h"
#include <recon.h>

/* The context is a PE header plus linker options. Unread fields retain their
 * discovered extent until their users are reconstructed. */
struct LinkOptions {
    BYTE reserved124[8], emit_symbols, reserved12d, default12e, reserved12f;
    BYTE retain_relocations, reserved131, fixed_image, reserved133;
    BYTE discard_unreferenced, default135, default136, write_map, reserved138[10];
    BYTE option142, reserved143, default144, profile, rom, order;
    BYTE effective_reference_opt, reference_opt_specified;
    BYTE windowsce_emulation, windowsce_noconvert;
    DWORD reserved14c, debug_level, debug_formats;
    BYTE reserved158[16];
    const char *default_extension;
    DWORD reserved16c[5];
    BYTE default180, reserved181[15];
};

struct LinkAuxiliaryData { DWORD reserved[11]; };
struct LinkPointerList {
    void *first;
    void **last;
    BYTE active, reserved[3];
    DWORD count;
};
/* A contextual output-file payload, never executed by the reconstruction. */
struct DefaultDosImage {
    IMAGE_DOS_HEADER header;
    BYTE bootstrap[14];
    char message[50];
};
extern "C" const DefaultDosImage default_dos_image;

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
    void *objects;
    void **objects_tail;
    LinkPointerList pointer_list;
    DWORD reserved1cc;
    StringTable *symbols, *strings;
    const DefaultDosImage *dos_image;
    DWORD dos_image_size, reserved1e0;
    int is_vxd;
    BYTE reserved1e8[120];
    LinkAuxiliaryData *auxiliary;
    BYTE reserved264[36];
    DWORD auxiliary_flags;
    BYTE reserved28c[12];
    LinkAuxiliaryData *RECON_THISCALL Initialize(int vxd);
    void RECON_THISCALL InitializeVxd(void);
    DWORD RECON_THISCALL PeHeaderSize(DWORD *offset);
    DWORD RECON_THISCALL VxdHeaderSize(DWORD *offset);
    void RECON_THISCALL PeWriteSection(int file,void *section,void *header);
    void RECON_THISCALL VxdWriteSection(int file,void *section,void *header);
    void RECON_THISCALL PeWriteHeaders(int file);
    void RECON_THISCALL VxdWriteHeaders(int file);
    int RECON_THISCALL FullLink(BYTE *pack_requested);
};
typedef DWORD (RECON_THISCALL LinkContext::*HeaderSizeHandler)(DWORD *);
typedef void (RECON_THISCALL LinkContext::*SectionWriter)(int,void *,void *);
typedef void (RECON_THISCALL LinkContext::*HeaderWriter)(int);
extern HeaderSizeHandler image_header_size;
extern SectionWriter image_write_section;
extern HeaderWriter image_write_headers;
typedef char DefaultDosImageSize[(sizeof(DefaultDosImage)==128)?1:-1];
typedef char LinkAuxiliaryDataSize[(sizeof(LinkAuxiliaryData)==44)?1:-1];
typedef char LinkContextSymbolsAt1d0[(offsetof(LinkContext,symbols)==0x1d0)?1:-1];
typedef char LinkContextAuxiliaryAt260[(offsetof(LinkContext,auxiliary)==0x260)?1:-1];
typedef char LinkOptionsExtensionAt44[(offsetof(LinkOptions,default_extension)==0x44)?1:-1];
typedef char LinkOptionsSize[(sizeof(LinkOptions)==108)?1:-1];
typedef char LinkContextSize[(sizeof(LinkContext)==664)?1:-1];
typedef char LinkContextOptionalAt44[(offsetof(LinkContext,optional_header)==0x44)?1:-1];
typedef char LinkContextOptionsAt124[(offsetof(LinkContext,options)==0x124)?1:-1];
typedef char LinkContextVxdAt1e4[(offsetof(LinkContext,is_vxd)==0x1e4)?1:-1];
#endif
