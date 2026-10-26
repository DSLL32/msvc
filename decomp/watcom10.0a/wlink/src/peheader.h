/****************************************************************************
*
*                            Open Watcom Project
*
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#ifndef WLINK_PEHEADER_H
#define WLINK_PEHEADER_H
#pragma pack(1)
/* type of a [relative] virtual address */
typedef unsigned long     pe_va;

/* PE header table types */
enum {
        PE_TBL_EXPORT,
        PE_TBL_IMPORT,
        PE_TBL_RESOURCE,
        PE_TBL_EXCEPTION,
        PE_TBL_SECURITY,
        PE_TBL_FIXUP,
        PE_TBL_DEBUG,
        PE_TBL_DESCRIPTION,
        PE_TBL_MACHINE,
        PE_TBL_THREAD,
        PE_TBL_CALLBACKS,
        PE_TBL_NUMBER = 16
};

#define OLD_PE_TBL_NUMBER 9

typedef struct pe_hdr_table_entry {
        pe_va           rva;
        unsigned long     size;
} pe_hdr_table_entry;

/* PE header structure */
typedef struct {
    unsigned long         signature;
    unsigned short         cpu_type;
    unsigned short         num_objects;
    unsigned long         time_stamp;
    unsigned long         sym_table;
    unsigned long         num_syms;
    unsigned short         nt_hdr_size;    /* # of bytes after the flags field */
    unsigned short         flags;
    unsigned short         magic;          /* currently 0x10b */
    unsigned char          lnk_major;
    unsigned char          lnk_minor;
    unsigned long         code_size;
    unsigned long         init_data_size;
    unsigned long         uninit_data_size;
    pe_va               entry_rva;
    unsigned long         code_base;
    unsigned long         data_base;
    unsigned long         image_base;
    unsigned long         object_align;
    unsigned long         file_align;
    unsigned short         os_major;
    unsigned short         os_minor;
    unsigned short         user_major;
    unsigned short         user_minor;
    unsigned short         subsys_major;
    unsigned short         subsys_minor;
    unsigned long         rsvd1;
    unsigned long         image_size;
    unsigned long         header_size;  //size of dos hdr, nt hdr, obj table & pad
    unsigned long         file_checksum;
    unsigned short         subsystem;
    unsigned short         dll_flags;
    unsigned long         stack_reserve_size;
    unsigned long         stack_commit_size;
    unsigned long         heap_reserve_size;
    unsigned long         heap_commit_size;
    unsigned long         tls_idx_addr;
    unsigned long         num_tables;
    pe_hdr_table_entry  table[PE_TBL_NUMBER];
} pe_header;

/* PE object table structure */
#define PE_OBJ_NAME_LEN 8
typedef struct {
    unsigned char          name[PE_OBJ_NAME_LEN];
    unsigned long         virtual_size;
    pe_va               rva;
    unsigned long         physical_size;
    unsigned long         physical_offset;
    unsigned long         relocs_rva;
    unsigned long         linnum_rva;
    unsigned short         num_relocs;
    unsigned short         num_linnums;
    unsigned long         flags;
} pe_object;

typedef struct pe_import_directory {
    pe_va import_lookup_table_rva;
    unsigned long time_stamp;
    unsigned short major, minor;
    pe_va name_rva;
    pe_va import_address_table_rva;
} pe_import_directory;
typedef struct pe_export_directory {
    unsigned long flags, time_stamp;
    unsigned short major, minor;
    pe_va name_rva;
    unsigned long ordinal_base, num_eat_entries, num_name_ptrs;
    pe_va address_table_rva, name_ptr_table_rva, ordinal_table_rva;
} pe_export_directory;
#pragma pack()
typedef char old_pe_header_is_248[(sizeof(pe_header) == 248) ? 1 : -1];
typedef char old_pe_object_is_40[(sizeof(pe_object) == 40) ? 1 : -1];
#include <recon.h>
#endif
