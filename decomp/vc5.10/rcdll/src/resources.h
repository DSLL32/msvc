#ifndef RCDLL_RESOURCES_H
#define RCDLL_RESOURCES_H
#include <stddef.h>
#include "resource-input.h"
#include <recon.h>
/* Contextual layouts from the dispatcher's actual field accesses and the
 * 0x428/20-byte allocation requests. Unidentified members retain that name. */
typedef struct RcResource {
    DWORD version,characteristics,unknown8,data_offset,size,unknown20,unknown24;
    WCHAR *name;
    DWORD unknown32;
    WORD language,flags,id,unknown42;
    WCHAR prefix[510];
} RcResource;
typedef struct RcResourceType {
    struct RcResourceType *next;
    DWORD unknown4;
    WCHAR *name;
    WORD id,unknown14,unknown16,unknown18;
} RcResourceType;
typedef char RcResourceLayout[
    sizeof(RcResource)==0x428 && offsetof(RcResource,name)==28 &&
    offsetof(RcResource,language)==36 && offsetof(RcResource,prefix)==44 &&
    sizeof(RcResourceType)==20 && offsetof(RcResourceType,id)==12 ? 1 : -1];
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
extern WCHAR rc_resource_name[257];
extern RcResourceType *rc_resource_types;
extern void *rc_string_tables;
extern WORD rc_font_resource_count,rc_next_resource_id;
extern DWORD rc_resource_version,rc_resource_characteristics;
extern int rc_explicit_font_directory;
extern void *rc_dialog_template;
extern DWORD rc_hwb_resource_offset;
extern int rc_resource_buffer_size,rc_resource_buffer_used;
extern BYTE *rc_resource_buffer;
extern FILE *rc_resource_data_file;
void __stdcall rc_resource_data_fatal(unsigned);
int __cdecl rc_read_resource_name(void);
void __stdcall rc_parse_menu(int extended,RcResource *resource);
void __stdcall rc_resource_error(unsigned code);
void __stdcall rc_resource_error_name(unsigned code,const WCHAR *name);
long __cdecl rc_read_resource_file(void);
RcResourceType * __stdcall rc_find_resource_type(const WCHAR *name,unsigned id);
int __stdcall rc_read_resource_attributes(RcResource *resource);
int __stdcall rc_parse_resource_style(void *style,unsigned flags);
void __cdecl rc_initialize_resource_types(void);
void __cdecl rc_flush_font_directory(void);
WORD __cdecl rc_read_resource_language(void);
RcResource * __stdcall rc_parse_string_table(RcResource *resource);
void __stdcall rc_write_string_table(void *table);
void __stdcall rc_parse_accelerators(RcResource *resource);
void __cdecl rc_initialize_resource_buffer(void);
void __cdecl rc_reset_resource_buffer(void);
void __cdecl rc_release_resource_buffer(void);
long __stdcall rc_convert_icon_file(long size);
long __cdecl rc_convert_bitmap_file(void);
void __stdcall rc_parse_raw_resource(RcResource *resource);
int __stdcall rc_parse_font_resource(RcResource *resource);
void __stdcall rc_write_resource(RcResourceType *type,RcResource *resource);
void __stdcall rc_convert_group_resource(RcResourceType *type,RcResource *resource,unsigned kind);
int __stdcall rc_is_riff_resource(long unused_size);
long __stdcall rc_copy_riff_resource(long size);
DWORD __stdcall rc_mac_long(DWORD value);
unsigned __stdcall rc_mac_word(WORD value);
void __stdcall rc_mac_group_icon(RcResourceType *type,RcResource *resource);
void __stdcall rc_mac_group_cursor(RcResourceType *type,RcResource *resource);
void __cdecl rc_write_mac_resource_file(void);
void __stdcall rc_parse_toolbar_resource(RcResource *resource);
void __stdcall rc_parse_dialog(RcResource *resource,void *dialog,int extended);
int __stdcall rc_parse_menu_template(int nested,RcResource *resource);
void __stdcall rc_read_resource_preamble(RcResource *,unsigned);
WORD __stdcall rc_parse_menu_item(int popup);
int __stdcall rc_copy_menu_caption(WCHAR *);
WORD __cdecl rc_scan_menu_commas(void);
int __stdcall rc_parse_menu_attribute(unsigned,RcMenuItem *);
WORD __stdcall rc_write_menu_item(RcMenuItem *);
void __stdcall rc_write_apstudio_name(const WCHAR *);
void __stdcall rc_mark_last_menu_item(WORD);
int __cdecl rc_parse_version_resource(void);
void __stdcall rc_parse_dialog_include(RcResource *unused_resource);
void __stdcall rc_write_resource_source(RcResource *resource,RcResourceType *type,const WCHAR *filename);
void __stdcall rc_write_resource_metadata(RcResource *resource,RcResourceType *type,int phase);
WCHAR *__stdcall rc_duplicate_resource_name(const WCHAR *);
WCHAR *__cdecl rc_collect_resource_name(void);
unsigned __stdcall rc_decimal_resource_id(const WCHAR *);
void __stdcall rc_read_resource_marker(int,DWORD);
#endif
