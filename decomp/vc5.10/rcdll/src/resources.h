#ifndef RCDLL_RESOURCES_H
#define RCDLL_RESOURCES_H
#include <stddef.h>
#include "resource-input.h"
#include <recon.h>
/* Contextual layouts from the dispatcher's actual field accesses and the
 * 0x428/20-byte allocation requests. Unidentified members retain that name. */
typedef struct RcResource {
    DWORD version,characteristics,unknown8,data_offset,size;
    struct RcResource *next;
    DWORD unknown24;
    WCHAR *name;
    DWORD unknown32;
    WORD language,flags,id,unknown42;
    WCHAR prefix[510];
} RcResource;
typedef struct RcResourceType {
    struct RcResourceType *next;
    RcResource *resources;
    WCHAR *name;
    WORD id,unknown14,unknown16,count;
} RcResourceType;
typedef char RcResourceLayout[
    sizeof(RcResource)==0x428 && offsetof(RcResource,name)==28 &&
    offsetof(RcResource,next)==20 && offsetof(RcResource,language)==36 && offsetof(RcResource,prefix)==44 &&
    sizeof(RcResourceType)==20 && offsetof(RcResourceType,id)==12 &&
    offsetof(RcResourceType,resources)==4 && offsetof(RcResourceType,count)==18 ? 1 : -1];
/* The menu parser owns this 520-byte local frame. The final word's role
 * and the original C object's boundary are unknown; only actual accesses
 * establish these members. The caption copier accepts 256 units plus NUL. */
typedef struct RcMenuItem {
    WORD id;
    WCHAR caption[257];
    WORD flags,unknown518;
} RcMenuItem;
typedef char RcMenuItemLayout[
    sizeof(RcMenuItem)==520 && offsetof(RcMenuItem,caption)==2 &&
    offsetof(RcMenuItem,flags)==516 && offsetof(RcMenuItem,unknown518)==518 ? 1 : -1];
/* Exact 0x826-byte allocation and actual packed field accesses. The WORD at
 * 18 has no established role. Strings occupy the copier's bounded 257 units. */
#pragma pack(push,2)
typedef struct RcDialog {
    WORD x,y,width,height;
    BYTE menu_is_ordinal,class_is_ordinal;
    DWORD style,exstyle;
    WORD unknown18;
    WCHAR caption[257],menu[257],class_name[257],font[257];
    WORD point_size,weight;
    BYTE italic,charset;
    DWORD help_id;
} RcDialog;
#pragma pack(pop)
typedef char RcDialogLayout[
    sizeof(RcDialog)==0x826 && offsetof(RcDialog,style)==10 &&
    offsetof(RcDialog,exstyle)==14 && offsetof(RcDialog,caption)==20 &&
    offsetof(RcDialog,menu)==0x216 && offsetof(RcDialog,class_name)==0x418 &&
    offsetof(RcDialog,font)==0x61a && offsetof(RcDialog,point_size)==0x81c &&
    offsetof(RcDialog,weight)==0x81e && offsetof(RcDialog,italic)==0x820 &&
    offsetof(RcDialog,charset)==0x821 && offsetof(RcDialog,help_id)==0x822 ? 1 : -1];
/* Consumed control fields occupy 1054 bytes of the parser's 1056-byte frame.
 * The final two frame bytes are outside this contextual object. */
#pragma pack(push,2)
typedef struct RcControl {
    WORD x,y,width,height,text_is_ordinal;
    WCHAR text[257];
    DWORD id;
    WCHAR class_name[257];
    DWORD style,exstyle,help_id;
} RcControl;
#pragma pack(pop)
typedef char RcControlLayout[
    sizeof(RcControl)==1054 && offsetof(RcControl,text_is_ordinal)==8 &&
    offsetof(RcControl,text)==10 && offsetof(RcControl,id)==524 &&
    offsetof(RcControl,class_name)==528 && offsetof(RcControl,style)==1042 &&
    offsetof(RcControl,exstyle)==1046 && offsetof(RcControl,help_id)==1050 ? 1 : -1];
extern int rc_dialog_id_count,rc_dialog_id_capacity;
extern DWORD *rc_dialog_ids;
void __stdcall rc_parse_generic_control(RcControl *,int);
WCHAR *__stdcall rc_format_wide_resource_number(unsigned,WCHAR *,unsigned);
void __stdcall rc_read_control_text(RcControl *);
void __stdcall rc_read_control_id(RcControl *,int);
extern DWORD rc_control_data_offset;
void __cdecl rc_pad_resource_buffer(void);
void __cdecl rc_increment_dialog_count(void);
void __stdcall rc_write_control(RcControl *,int);
void __stdcall rc_patch_control_data_size(WORD,int);
extern WCHAR rc_resource_name[257];
extern RcResourceType *rc_resource_types;
/* A string block is allocated with the exact 0x92-byte request. */
#pragma pack(push,2)
typedef struct RcStringBlock {
    struct RcStringBlock *next;
    DWORD version,characteristics;
    WORD block,flags,language;
    WCHAR *strings[16],*prefixes[16];
} RcStringBlock;
#pragma pack(pop)
typedef char RcStringBlockLayout[
    sizeof(RcStringBlock)==146 && offsetof(RcStringBlock,block)==12 &&
    offsetof(RcStringBlock,flags)==14 && offsetof(RcStringBlock,language)==16 &&
    offsetof(RcStringBlock,strings)==18 && offsetof(RcStringBlock,prefixes)==82 ? 1 : -1];
extern RcStringBlock *rc_string_blocks;
extern RcResource *rc_last_string_resource;
void *__stdcall rc_allocate_copy(unsigned,const void *);
void __stdcall rc_report_resource_error(unsigned);

extern WORD rc_font_resource_count,rc_resource_count;
extern DWORD rc_resource_version,rc_resource_characteristics;
extern int rc_explicit_font_directory;
extern RcDialog *rc_dialog_template;
extern DWORD rc_hwb_resource_offset;
extern int rc_resource_buffer_size,rc_resource_buffer_used;
extern BYTE *rc_resource_buffer;
extern FILE *rc_resource_data_file;
void __stdcall rc_resource_data_fatal(unsigned);
int __cdecl rc_read_resource_name(void);
void __stdcall rc_parse_menu(int extended,RcResource *resource);
void __stdcall rc_resource_error(unsigned code);
void __stdcall rc_resource_error_name(unsigned code,const void *name);
long __cdecl rc_read_resource_file(void);
void __stdcall rc_search_resource_path(const char *,const char *,char *);
FILE *__stdcall rc_resource_file(FILE *);
RcResourceType * __stdcall rc_find_resource_type(const WCHAR *name,unsigned id);
int __stdcall rc_read_resource_attributes(RcResource *resource);
int __stdcall rc_parse_resource_style(void *style,unsigned flags);
int __stdcall rc_parse_resource_style_expression(DWORD *,int);
int __stdcall rc_apply_resource_style_term(DWORD *,WORD);
void __cdecl rc_initialize_resource_types(void);
void __cdecl rc_flush_font_directory(void);
WORD __cdecl rc_read_resource_language(void);
RcResource * __stdcall rc_parse_string_table(RcResource *resource);
void __stdcall rc_write_string_table(RcResource *unused_resource);
int __stdcall rc_parse_accelerators(RcResource *resource);
void __cdecl rc_begin_accelerator(void);
extern DWORD rc_accelerator_offset;
void __cdecl rc_mark_last_accelerator(void);
void __cdecl rc_initialize_resource_buffer(void);
void __cdecl rc_reset_resource_buffer(void);
void __cdecl rc_release_resource_buffer(void);
long __stdcall rc_convert_icon_file(long size);
long __cdecl rc_convert_bitmap_file(void);
WORD __stdcall rc_parse_raw_resource(RcResource *resource);
int __stdcall rc_parse_font_resource(RcResource *resource);
void __stdcall rc_write_resource(RcResourceType *type,RcResource *resource);
void __stdcall rc_append_resource(RcResourceType *,RcResource *,const void *,int,int);
void __stdcall rc_write_resource_record(RcResourceType *,RcResource *,const void *,int,int);
DWORD __cdecl rc_resource_data_size(void);
unsigned __stdcall rc_write_resource_identifier(WORD,const WCHAR *);
int __stdcall rc_write_resource_payload(FILE *,const void *,int,int);
void __stdcall rc_convert_group_resource(RcResourceType *type,RcResource *resource,unsigned kind);
void __stdcall rc_append_resource_bytes(const void *,WORD);
void __stdcall rc_resource_file_fatal(unsigned,const WCHAR *);
extern WORD rc_next_image_id;
/* File directory and emitted group-entry fields consumed by the converter.
 * Packing follows its 6/16/12-byte transfers, independently of local-frame gaps. */
#pragma pack(push,2)
typedef struct RcImageDirectory { WORD reserved,kind,count; } RcImageDirectory;
typedef struct RcImageFileEntry {
    BYTE width,height,color_count,reserved;
    WORD planes_or_x,bits_or_y;
    DWORD size,offset;
} RcImageFileEntry;
typedef struct RcBitmapInfo {
    DWORD size;
    LONG width,height;
    WORD planes,bit_count;
    DWORD compression,image_size;
    LONG x_pixels,y_pixels;
    DWORD colors_used,colors_important;
} RcBitmapInfo;
typedef struct RcBitmapFileHeader {
    WORD type;
    DWORD size;
    WORD reserved1,reserved2;
    DWORD pixel_offset;
} RcBitmapFileHeader;
typedef struct RcBitmapCore {
    DWORD size;
    WORD width,height,planes,bit_count;
} RcBitmapCore;
typedef union RcBitmapHeader {
    RcBitmapInfo info;
    RcBitmapCore core;
} RcBitmapHeader;
typedef struct RcImageGroupEntry {
    union {
        struct { BYTE width,height,color_count,reserved; } icon;
        struct { WORD width,height; } cursor;
    } dimensions;
    WORD planes,bit_count;
    DWORD size;
} RcImageGroupEntry;
#pragma pack(pop)
typedef char RcImageLayouts[
    sizeof(RcImageDirectory)==6 && sizeof(RcImageFileEntry)==16 &&
    offsetof(RcImageFileEntry,planes_or_x)==4 && offsetof(RcImageFileEntry,size)==8 &&
    offsetof(RcImageFileEntry,offset)==12 && sizeof(RcBitmapInfo)==40 &&
    offsetof(RcBitmapInfo,width)==4 && offsetof(RcBitmapInfo,height)==8 &&
    offsetof(RcBitmapInfo,planes)==12 && offsetof(RcBitmapInfo,bit_count)==14 &&
    sizeof(RcBitmapFileHeader)==14 && offsetof(RcBitmapFileHeader,pixel_offset)==10 &&
    sizeof(RcBitmapCore)==12 && sizeof(RcBitmapHeader)==40 &&
    sizeof(RcImageGroupEntry)==12 && offsetof(RcImageGroupEntry,planes)==4 &&
    offsetof(RcImageGroupEntry,bit_count)==6 && offsetof(RcImageGroupEntry,size)==8 ? 1 : -1];
int __stdcall rc_is_riff_resource(long unused_size);
long __stdcall rc_copy_riff_resource(long size);
DWORD __stdcall rc_mac_long(DWORD value);
unsigned __stdcall rc_mac_word(WORD value);
void __stdcall rc_mac_group_icon(RcResourceType *type,RcResource *resource);
void __stdcall rc_mac_group_cursor(RcResourceType *type,RcResource *resource);
void __cdecl rc_write_mac_resource_file(void);
void __stdcall rc_parse_toolbar_resource(RcResource *resource);
int __stdcall rc_parse_dialog(RcResource *,RcDialog *,int);
WORD __cdecl rc_read_dialog_coordinate(void);
void __stdcall rc_read_dialog_rectangle(WORD *,WORD *,WORD *,WORD *);
void __stdcall rc_resource_notice_strings(unsigned,const WCHAR *,const WCHAR *,DWORD);
void __stdcall rc_parse_dialog_font(RcDialog *,int);
void __stdcall rc_parse_dialog_menu(RcDialog *);
void __stdcall rc_parse_dialog_caption(RcDialog *);
void __stdcall rc_parse_dialog_class(RcDialog *);
int __stdcall rc_parse_dialog_attributes(RcResource *,RcDialog *,int);
extern DWORD rc_dialog_count_offset;
void __cdecl rc_write_ordinal_marker(void);
void __stdcall rc_write_dialog_header(RcDialog *,int);
int __stdcall rc_parse_dialog_controls(int);
void __cdecl rc_convert_dialog_count(void);
int __stdcall rc_parse_menu_template(int nested,RcResource *resource);
void __stdcall rc_read_resource_preamble(RcResource *,unsigned);
WORD __stdcall rc_parse_menu_item(int popup);
int __stdcall rc_copy_menu_caption(WCHAR *);
extern int rc_menu_had_comma;
WORD __cdecl rc_scan_menu_commas(void);
int __stdcall rc_parse_menu_attribute(unsigned,RcMenuItem *);
WORD __stdcall rc_write_menu_item(RcMenuItem *);
void __stdcall rc_write_resource_string(const WCHAR *,int);
void __stdcall rc_write_mac_string(const WCHAR *,int,int);
void __stdcall rc_write_apstudio_name(const WCHAR *);
void __stdcall rc_mark_last_menu_item(WORD);
int __cdecl rc_parse_version_resource(void);
WORD __cdecl rc_write_fixed_version(void);
typedef struct RcVersionNumber { DWORD ms,ls; } RcVersionNumber;
typedef struct RcFixedVersion {
    DWORD signature,structure_version;
    RcVersionNumber file_version,product_version;
    DWORD flags_mask,flags,os,type,subtype,date_ms,date_ls;
} RcFixedVersion;
typedef char RcFixedVersionLayout[
    sizeof(RcFixedVersion)==52 && offsetof(RcFixedVersion,file_version)==8 &&
    offsetof(RcFixedVersion,product_version)==16 && offsetof(RcFixedVersion,flags_mask)==24 &&
    offsetof(RcFixedVersion,flags)==28 && offsetof(RcFixedVersion,os)==32 &&
    offsetof(RcFixedVersion,type)==36 && offsetof(RcFixedVersion,subtype)==40 &&
    offsetof(RcFixedVersion,date_ms)==44 && offsetof(RcFixedVersion,date_ls)==48 ? 1 : -1];
void __stdcall rc_read_version_quad(RcVersionNumber *);
void __stdcall rc_read_version_number(DWORD *);
WORD __stdcall rc_write_version_block(const WCHAR *,const void *,unsigned);
WORD __cdecl rc_parse_version_children(void);
WORD __stdcall rc_write_version_value(const WCHAR *);
WORD __stdcall rc_write_version_data(DWORD);
void __stdcall rc_concatenate_resource_string(const WCHAR *,int);
WORD __cdecl rc_resource_buffer_position(void);
WORD __stdcall rc_resource_buffer_word(DWORD);
void __stdcall rc_set_resource_buffer_word(DWORD,WORD);
void __stdcall rc_parse_dialog_include(RcResource *unused_resource);
void __stdcall rc_write_resource_source(RcResource *resource,RcResourceType *type,const WCHAR *filename);
void __stdcall rc_write_resource_metadata(RcResource *resource,RcResourceType *type,int phase);
WCHAR *__stdcall rc_duplicate_resource_name(const WCHAR *);
WCHAR *__cdecl rc_collect_resource_name(void);
unsigned __stdcall rc_decimal_resource_id(const WCHAR *);
void __stdcall rc_read_resource_marker(int,DWORD);
#endif
