#ifndef OMF_OUTPUT_H
#define OMF_OUTPUT_H
#include "coff-output.h"
#include "output.h"
#include <recon.h>
typedef struct OmfGroup {
    unsigned name_index, number;
} OmfGroup;
typedef struct OmfFixupThreads {
    unsigned group_frame, bss_target, data_target, const_target, code_target;
} OmfFixupThreads;
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
void omf_initialize_records(void);
void omf_flush_records(void);
void __fastcall omf_write_module_header(char *source);
unsigned __fastcall omf_name_index(char *name);
void __fastcall omf_write_segment_definition(Segment *segment);
void __fastcall omf_write_group_definition(Segment *segment, OmfGroup *group);
unsigned __fastcall omf_define_fixup_thread(unsigned char flags, unsigned char method,
    unsigned target, unsigned thread);
#endif
