#ifndef LINK_VXD_H
#define LINK_VXD_H
#include <windows.h>
#include <stddef.h>
#include <recon.h>

/* The linear-executable header at original 4673f0. Only fields whose
 * producers have been recovered are consumed by the current native path. */
typedef struct VxdImageHeader {
    WORD signature;
    BYTE byte_order,word_order;
    DWORD format_level;
    WORD cpu,os;
    DWORD module_version,flags,module_pages;
    DWORD entry_object,entry_offset,stack_object,stack_offset;
    DWORD page_size,last_page_size,fixup_size,fixup_checksum;
    DWORD loader_size,loader_checksum,object_table,object_count,page_map;
    DWORD iterated_pages,resource_table,resource_count,resident_names,entry_table;
    DWORD directives,directive_count,fixup_page_table,fixup_records;
    DWORD import_modules,import_module_count,import_procedures,page_checksums;
    DWORD data_pages,preload_pages,nonresident_names,nonresident_size;
    DWORD nonresident_checksum,automatic_data_object,debug_offset,debug_size;
    DWORD preload_instance_pages,demand_instance_pages,heap_size;
    BYTE reservedac[12];
    DWORD windows_resources,windows_resource_size;
    WORD device_id,ddk_version;
} VxdImageHeader;
typedef struct VxdObjectRecord {
    DWORD virtual_size,base,flags,page_map_index,page_count;
    char name[4];
} VxdObjectRecord;
typedef char VxdHeaderSize[(sizeof(VxdImageHeader)==196)?1:-1];
typedef char VxdHeaderLastPageAt2c[(offsetof(VxdImageHeader,last_page_size)==0x2c)?1:-1];
typedef char VxdHeaderPreloadAt84[(offsetof(VxdImageHeader,preload_pages)==0x84)?1:-1];
typedef char VxdObjectSize[(sizeof(VxdObjectRecord)==24)?1:-1];
typedef struct VxdFixup {
    struct VxdFixup *next;
    BYTE type,target_object;
    WORD reserved06;
    LONG target_offset;
    WORD capacity;
    BYTE count,reserved0f;
    SHORT *sources;
} VxdFixup;
typedef struct VxdPageFixups {
    VxdFixup *first;
    DWORD offset;
} VxdPageFixups;
typedef char VxdFixupSize[(sizeof(VxdFixup)==20)?1:-1];
typedef char VxdPageFixupSize[(sizeof(VxdPageFixups)==8)?1:-1];
#endif
