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
extern WCHAR rc_resource_name[257];
extern WORD rc_font_resource_count,rc_next_resource_id;
extern DWORD rc_resource_version,rc_resource_characteristics;
extern int rc_explicit_font_directory;
extern void *rc_dialog_template;
extern DWORD rc_hwb_resource_offset;
extern int rc_resource_buffer_size,rc_resource_buffer_used;
extern BYTE *rc_resource_buffer;
int __cdecl rc_read_resource_name(void);
void __stdcall rc_parse_menu(int extended,RcResource *resource);
void __stdcall rc_resource_error(unsigned code);
long __cdecl rc_read_resource_file(void);
RcResourceType * __stdcall rc_find_resource_type(const WCHAR *name,unsigned id);
int __stdcall rc_read_resource_attributes(RcResource *resource);
void __cdecl rc_initialize_resource_types(void);
void __cdecl rc_flush_font_directory(void);
unsigned __cdecl rc_read_resource_language(void);
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
void __stdcall rc_parse_version_resource(RcResource *resource);
void __stdcall rc_parse_dialog(RcResource *resource,void *dialog,int extended);
void __stdcall rc_parse_menu_template(int extended,RcResource *resource);
int __cdecl rc_parse_dialog_include(void);
void __stdcall rc_parse_ansi_resource(RcResource *unused_resource);
void __stdcall rc_write_resource_source(RcResource *resource,RcResourceType *type,const WCHAR *filename);
void __stdcall rc_write_resource_metadata(RcResource *resource,RcResourceType *type,int phase);
WCHAR *__stdcall rc_duplicate_resource_name(const WCHAR *);
#endif
