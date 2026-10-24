#ifndef OMF_OUTPUT_H
#define OMF_OUTPUT_H
#include "coff-output.h"
#include "output.h"
#include <stddef.h>
#include <recon.h>
typedef struct OmfFixupThreads {
    unsigned group_frame, bss_target, data_target, const_target, code_target;
} OmfFixupThreads;
/* Constructor6b990 and allocator6bd10 use24-byte fixup entries. The first
   word and natural alignment padding are untouched here, not initialized. */
typedef struct OmfFixup {
    unsigned reserved00;
    unsigned char type;
    unsigned short location;
    unsigned flags;
    unsigned char frame_code, target_code;
    unsigned short frame_datum, target_datum;
    unsigned addend;
} OmfFixup;
typedef char OmfFixupSizeCheck[sizeof(OmfFixup) == 24 ? 1 : -1];
typedef char OmfFixupLayoutCheck[offsetof(OmfFixup, type) == 4 &&
    offsetof(OmfFixup, location) == 6 && offsetof(OmfFixup, flags) == 8 &&
    offsetof(OmfFixup, frame_code) == 12 && offsetof(OmfFixup, target_code) == 13 &&
    offsetof(OmfFixup, frame_datum) == 14 && offsetof(OmfFixup, target_datum) == 16 &&
    offsetof(OmfFixup, addend) == 20 ? 1 : -1];
enum {
    OMF_FIXUP_SEGMENT_RELATIVE = 1, OMF_FIXUP_FRAME_THREAD = 2,
    OMF_FIXUP_TARGET_THREAD = 4, OMF_FIXUP_NO_ADDEND = 8
};
/* Four reusable wire-record buffers. The twenty-byte header is followed by
   serialized record/header/checksum space; original slots are1004 bytes. */
typedef struct OmfRecord {
    unsigned type;
    void *context;
    unsigned context_offset, count;
    unsigned char *cursor;
    unsigned char bytes[984];
} OmfRecord;
typedef char OmfRecordSizeCheck[sizeof(OmfRecord) == 1004 ? 1 : -1];
extern OmfRecord omf_records[4], *omf_current_record;
extern OmfGroup *omf_default_group;
extern OmfFixupThreads omf_fixup_threads;
extern unsigned omf_empty_name, omf_next_group, omf_next_segment;
extern unsigned omf_next_frame_thread, omf_next_target_thread;
extern OmfFixup omf_fixups[250];
extern int omf_fixup_count;
void omf_write_fixups(void);
void __fastcall omf_write_fixup(OmfFixup *fixup);
void __fastcall omf_write_line(unsigned short line, unsigned offset, Segment *segment);
void __fastcall omf_write_comdat_line(unsigned short line, unsigned offset, Segment *segment);
void omf_initialize_records(void);
OmfRecord *omf_flush_records(void);
void __fastcall omf_write_module_header(char *source);
unsigned __fastcall omf_name_index(char *name);
void __fastcall omf_write_segment_definition(Segment *segment);
void __fastcall omf_write_group_definition(Segment *segment, OmfGroup *group);
unsigned __fastcall omf_define_fixup_thread(unsigned char flags, unsigned char method,
    unsigned target, unsigned thread);
#endif
