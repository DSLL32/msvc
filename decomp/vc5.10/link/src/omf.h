#ifndef LINK_OMF_H
#define LINK_OMF_H
#include <windows.h>
#include <stdio.h>
#include <stddef.h>
#include <setjmp.h>
#include <recon.h>

extern jmp_buf omf_error_jump;
extern FILE *omf_stream;
extern char *omf_module_name;
extern char *omf_comments[256];
extern char *omf_names[2048];
extern DWORD omf_name_kinds[2048];
char *__stdcall read_omf_string(DWORD);
DWORD __stdcall append_omf_auxiliary(const void *);
extern IMAGE_SYMBOL *omf_symbols;
extern DWORD omf_symbol_count,omf_symbol_capacity;
extern char *omf_strings;
extern DWORD omf_string_length,omf_string_capacity;
DWORD __stdcall intern_omf_string(const char *);
/* Each object retains its encountered original width. Roles used only by
 * pending record handlers remain provisional until those consumers are read. */
extern WORD omf_external_count,omf_external_symbol_count,omf_group_count;
extern WORD omf_segdef_count,omf_name_count,omf_module_count;
extern WORD omf_data_record,omf_data_segment,omf_default_line_segment;
extern WORD omf_section_symbol_count,omf_comment_count;
extern SHORT omf_record_type,omf_record_length;
extern WORD omf_record_is32;
extern BYTE omf_module_name_changed,omf_record_checksum;
extern DWORD omf_compile_flags,omf_data_offset,omf_pass;
extern DWORD omf_line_count,omf_comment_bytes;
typedef struct OmfLine { DWORD offset; WORD line,unrecovered_6; } OmfLine;
typedef char OmfLineLayout[(sizeof(OmfLine)==8 && offsetof(OmfLine,line)==4)?1:-1];
extern OmfLine *omf_line_buffer;
extern DWORD omf_function_auxiliary_index;
typedef struct OmfLineNode {
    DWORD offset;
    WORD line,unrecovered_6;
    struct OmfLineNode *next;
} OmfLineNode;
typedef struct OmfLineList {
    OmfLineNode *head,*tail;
    WORD count,unrecovered_a;
} OmfLineList;
typedef char OmfLineListLayout[(sizeof(OmfLineNode)==12 &&
    offsetof(OmfLineNode,line)==4 && offsetof(OmfLineNode,next)==8 &&
    sizeof(OmfLineList)==12 && offsetof(OmfLineList,count)==8)?1:-1];
extern OmfLineList omf_line_lists[256];
extern DWORD omf_output_line_count;
void __stdcall append_omf_function_auxiliary(DWORD,DWORD,DWORD,DWORD,DWORD,DWORD);
typedef struct OmfNameEntry {
    char *name;
    DWORD value;
    WORD kind;
    SHORT section;
    WORD external_index,type_index;
    WORD weak_default_index,unrecovered_12;
    struct OmfNameEntry *next;
} OmfNameEntry;
typedef char OmfNameEntryLayout[(sizeof(OmfNameEntry)==24 &&
    offsetof(OmfNameEntry,kind)==8 && offsetof(OmfNameEntry,section)==10 &&
    offsetof(OmfNameEntry,external_index)==12 && offsetof(OmfNameEntry,type_index)==14 &&
    offsetof(OmfNameEntry,next)==20)?1:-1];
typedef struct OmfExternalNode {
    DWORD symbol,displacement,output_symbol;
    struct OmfExternalNode *next;
} OmfExternalNode;
typedef char OmfExternalNodeLayout[(sizeof(OmfExternalNode)==16 &&
    offsetof(OmfExternalNode,displacement)==4 && offsetof(OmfExternalNode,output_symbol)==8 &&
    offsetof(OmfExternalNode,next)==12)?1:-1];
extern struct OmfNameEntry **omf_symbol_hash;
extern struct OmfExternalNode **omf_offset_hash;
DWORD __stdcall hash_omf_name(const char *);
void __stdcall insert_omf_name(char *,DWORD,DWORD,DWORD,DWORD,DWORD);
extern DWORD omf_external_map[4096],omf_public_map[4096];
typedef struct OmfSegmentInfo {
    WORD name_index,output_section;
    DWORD alignment,characteristics;
} OmfSegmentInfo;
typedef char OmfSegmentInfoLayout[(sizeof(OmfSegmentInfo)==12 &&
    offsetof(OmfSegmentInfo,name_index)==0 && offsetof(OmfSegmentInfo,output_section)==2 &&
    offsetof(OmfSegmentInfo,alignment)==4 && offsetof(OmfSegmentInfo,characteristics)==8)?1:-1];
extern OmfSegmentInfo omf_segments[256]; /* logical segment IDs1..255 */
extern DWORD omf_segdef_symbols[256],omf_group_symbols[256];
extern DWORD omf_section_auxiliary_indices[256]; /* output section ordinal minus1 */
DWORD __cdecl read_omf_dword(void);
WORD __cdecl read_omf_index(void);
extern WORD omf_output_section_count;
extern IMAGE_SECTION_HEADER omf_output_sections[256];
extern BYTE *omf_section_data[256];
extern DWORD omf_section_capacities[256],omf_initialized_data_size;
DWORD __stdcall create_omf_section(const char *,DWORD,DWORD,DWORD,DWORD);
void __stdcall read_omf_module(const char *);
void __cdecl write_omf_coff(void);
void __cdecl initialize_omf_hashes(void);
DWORD __stdcall append_omf_symbol(const char *,DWORD,DWORD,DWORD,DWORD);
DWORD __cdecl read_omf_word(void);
DWORD __stdcall read_omf_bytes(void *,DWORD);
DWORD __cdecl read_omf_byte(void);
void __cdecl omf_error(const char *,...);
void __stdcall read_omf_extdefs(DWORD,DWORD);
typedef struct OmfFixupPosition {
    LONG file_offset;
    DWORD data_offset;
    WORD segment,record_type;
    struct OmfFixupPosition *next;
} OmfFixupPosition;
typedef char OmfFixupPositionLayout[(sizeof(OmfFixupPosition)==16 &&
    offsetof(OmfFixupPosition,data_offset)==4 && offsetof(OmfFixupPosition,segment)==8 &&
    offsetof(OmfFixupPosition,record_type)==10 && offsetof(OmfFixupPosition,next)==12)?1:-1];
extern OmfFixupPosition *omf_fixup_head,*omf_fixup_tail;
void __stdcall remember_omf_fixup_position(LONG);
void __cdecl read_omf_fixupp(void);
DWORD __stdcall read_omf_fixup_target(DWORD);
void __stdcall remember_omf_target_offset(DWORD,DWORD);
DWORD __stdcall lookup_omf_target_offset(DWORD,DWORD);
typedef struct OmfRelocationNode {
    DWORD offset,symbol;
    WORD type,unrecovered_a;
    struct OmfRelocationNode *next;
} OmfRelocationNode;
typedef struct OmfRelocationList {
    OmfRelocationNode *head,*tail;
    WORD count,unrecovered_a;
} OmfRelocationList;
typedef char OmfRelocationLayout[(sizeof(OmfRelocationNode)==16 &&
    offsetof(OmfRelocationNode,type)==8 && offsetof(OmfRelocationNode,next)==12 &&
    sizeof(OmfRelocationList)==12 && offsetof(OmfRelocationList,count)==8)?1:-1];
extern OmfRelocationList omf_relocations[256];
extern DWORD omf_relocation_count;
extern WORD omf_file_symbol_reference;
void __stdcall append_omf_relocation_node(OmfRelocationList *,DWORD,DWORD,DWORD);
void __stdcall append_omf_relocation(DWORD,DWORD,DWORD,DWORD,DWORD);
extern DWORD omf_target_threads[4];
extern BYTE omf_target_thread_is_segment[4];
void __stdcall read_omf_pubdefs(DWORD);
void __cdecl read_omf_ledata(void);
void __stdcall write_omf_section_data(DWORD,DWORD,const void *,DWORD);
void __cdecl read_omf_lidata(void);
void __stdcall read_omf_comdefs(DWORD);
void __stdcall read_omf_lpubdefs(DWORD);
void __stdcall read_omf_comment(const char *);
void __cdecl read_omf_segdef(void);
void __cdecl read_omf_linnum(void);
void __cdecl read_omf_grpdef(void);
void __cdecl read_omf_theadr(void);
void __cdecl read_omf_modend(void);
void __stdcall write_omf_indirect_section_data(DWORD,DWORD,char *const *,DWORD);
void __stdcall read_omf_lnames(DWORD);
void __cdecl read_omf_lheadr(void);
void __cdecl read_omf_comdat(void);
void __cdecl read_omf_alias(void);
void __cdecl read_omf_bakpat(void);
void __cdecl read_omf_linsym(void);
void __cdecl finish_omf_record(void);
void __cdecl finish_omf_external_symbols(void);
void __cdecl resolve_omf_segments(void);
void __cdecl write_omf_lines(void);
void __stdcall append_omf_line(DWORD,DWORD,DWORD);
void __cdecl replay_omf_fixups(void);
#endif
