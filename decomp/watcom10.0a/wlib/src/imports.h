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
/* Selected wire-format declarations from initial OpenWatcom commit
   6b9cb4438959d845a2b4dd4bf19f7908ecd608e9, bld/watcom/h/
   {exeos2,exeflat,exenov,exepe}.h. Period scalar spellings replace
   unsigned_N/pe_va; only the encountered header/table types are retained. */
#ifndef WLIB_IMPORTS_H
#define WLIB_IMPORTS_H
#include "toolstate.h"
#include <recon.h>
#pragma pack(1)
typedef struct os2_exe_header {
    unsigned short         signature;      /* signature to mark valid EXE file */
    unsigned short         version;        /* version of linker                */
    unsigned short         entry_off;      /* offset from header of entry table*/
    unsigned short         entry_size;     /* size of entry table              */
    unsigned long         chk_sum;        /* checksum of entire file          */
    unsigned short         info;           /* various flags (see below)        */
    unsigned short         adsegnum;       /* automatic data segment number    */
    unsigned short         heap;           /* initial heap allocation - bytes  */
    unsigned short         stack;          /* initial stack allocation - bytes */
    unsigned short         IP;             /* value for IP                     */
    unsigned short         entrynum;       /* segment number of entry segment  */
    unsigned short         SP;             /* value for SP                     */
    unsigned short         stacknum;       /* segment number of stack segment  */
    unsigned short         segments;       /* total number of segments         */
    unsigned short         modrefs;        /* # entries in module ref table    */
    unsigned short         nonres_size;    /* size of nonresident name table   */
    unsigned short         segment_off;    /* header offset of   segment table */
    unsigned short         resource_off;   /* " " "             resource table */
    unsigned short         resident_off;   /* " " "        resident name table */
    unsigned short         module_off;     /* " " "     module reference table */
    unsigned short         import_off;     /* " " "       imported names table */
    unsigned long         nonres_off;     /* offset from start of file of
                                                    nonresident names table */
    unsigned short         movable;        /* number of movable entry points   */
    unsigned short         align;          /* segment alignment shift count    */
    unsigned short         resource;       /* # of entries in resource table   */
    unsigned char          target;         /* target operating system */
    unsigned char          otherflags;     /* more flags */
    unsigned short         gangstart;      /* these 4 added for windows */
    unsigned short         ganglength;
    unsigned short         swaparea;
    unsigned short         expver;
} os2_exe_header;

typedef struct os2_flat_header {
    unsigned short     signature;
    unsigned char      byte_order;     /* the byte ordering of the .exe */
    unsigned char      word_order;     /* the word ordering of the .exe */
    unsigned long     level;          /* the exe format level */
    unsigned short     cpu_type;       /* the cpu type */
    unsigned short     os_type;        /* the operating system type */
    unsigned long     version;        /* .exe version */
    unsigned long     flags;          /* .exe flags */
    unsigned long     num_pages;      /* # of pages in .exe */
    unsigned long     start_obj;      /* starting object number */
    unsigned long     eip;            /* starting value of eip */
    unsigned long     stack_obj;      /* object # for stack pointer */
    unsigned long     esp;            /* starting value of esp */
    unsigned long     page_size;      /* .exe page size */
    union {
        unsigned long     last_page;      /* size of last page - LE */
        unsigned long     page_shift;     /* left shift for page offsets - LX */
    }               l;
    unsigned long     fixup_size;     /* fixup section size */
    unsigned long     fixup_cksum;    /* fixup section checksum */
    unsigned long     loader_size;    /* loader section size */
    unsigned long     loader_cksum;   /* loader section checksum */
    unsigned long     objtab_off;     /* object table offset */
    unsigned long     num_objects;    /* number of objects in .exe */
    unsigned long     objmap_off;     /* object page map offset */
    unsigned long     idmap_off;      /* iterated data map offset */
    unsigned long     rsrc_off;       /* offset of resource table */
    unsigned long     num_rsrcs;      /* number of resource entries */
    unsigned long     resname_off;    /* offset of resident names table */
    unsigned long     entry_off;      /* offset of entry table */
    unsigned long     moddir_off;     /* offset of module directives table */
    unsigned long     num_moddirs;    /* number of module directives */
    unsigned long     fixpage_off;    /* offset of fixup page table */
    unsigned long     fixrec_off;     /* offset of fixup record table */
    unsigned long     impmod_off;     /* offset of import module name table */
    unsigned long     num_impmods;    /* # of entries in import mod name tbl */
    unsigned long     impproc_off;    /* offset of import procedure name table */
    unsigned long     cksum_off;      /* offset of per-page checksum table */
    unsigned long     page_off;       /* offset of enumerated data pages */
    unsigned long     num_preload;    /* number of preload pages */
    unsigned long     nonres_off;     /* offset of non-resident names table */
    unsigned long     nonres_size;    /* size of non-resident names table */
    unsigned long     nonres_cksum;   /* non-resident name table checksum */
    unsigned long     autodata_obj;   /* object # of autodata segment */
    unsigned long     debug_off;      /* offset of the debugging information */
    unsigned long     debug_len;      /* length of the debugging info */
    unsigned long     num_inst_preload;   /* # of instance pages in preload sect*/
    unsigned long     num_inst_demand;    /*# instance pages in demand load sect*/
    unsigned long     heapsize;       /* size of heap - for 16-bit apps */
    unsigned long     stacksize;      /* size of stack */
    unsigned char      reserved[ 20 ];   /* pad to 196 bytes. */
} os2_flat_header;

typedef struct nlm_header {
    char            signature[ 24 ];
    unsigned long     version;
    char            moduleName[14];     /* 1st character is a length byte.*/
    unsigned long     codeImageOffset;
    unsigned long     codeImageSize;
    unsigned long     dataImageOffset;
    unsigned long     dataImageSize;
    unsigned long     uninitializedDataSize;
    unsigned long     customDataOffset;
    unsigned long     customDataSize;
    unsigned long     moduleDependencyOffset;
    unsigned long     numberOfModuleDependencies;
    unsigned long     relocationFixupOffset;
    unsigned long     numberOfRelocationFixups;
    unsigned long     externalReferencesOffset;
    unsigned long     numberOfExternalReferences;
    unsigned long     publicsOffset;
    unsigned long     numberOfPublics;
    unsigned long     debugInfoOffset;
    unsigned long     numberOfDebugRecords;
    unsigned long     codeStartOffset;
    unsigned long     exitProcedureOffset;
    unsigned long     checkUnloadProcedureOffset;
    unsigned long     moduleType;
    unsigned long     flags;
    unsigned char      descriptionLength;
} nlm_header;

typedef struct {
        unsigned long           rva;
        unsigned long     size;
} pe_hdr_table_entry;

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
    unsigned long               entry_rva;
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
    pe_hdr_table_entry  table[16];
} pe_header;

typedef struct {
    unsigned char          name[8];
    unsigned long         virtual_size;
    unsigned long               rva;
    unsigned long         physical_size;
    unsigned long         physical_offset;
    unsigned long         relocs_rva;
    unsigned long         linnum_rva;
    unsigned short         num_relocs;
    unsigned short         num_linnums;
    unsigned long         flags;
} pe_object;
typedef struct {
    unsigned long         flags;
    unsigned long         time_stamp;
    unsigned short         major;
    unsigned short         minor;
    unsigned long               name_rva;
    unsigned long         ordinal_base;
    unsigned long         num_eat_entries;
    unsigned long         num_name_ptrs;
    unsigned long               address_table_rva;
    unsigned long               name_ptr_table_rva;
    unsigned long               ordinal_table_rva;
} pe_export_directory;
#pragma pack()
typedef char WlibNeHeaderSize[(sizeof(os2_exe_header) == 64) ? 1 : -1];
typedef char WlibFlatHeaderSize[(sizeof(os2_flat_header) == 196) ? 1 : -1];
typedef char WlibNlmHeaderSize[(sizeof(nlm_header) == 131) ? 1 : -1];
typedef char WlibPeHeaderSize[(sizeof(pe_header) == 248) ? 1 : -1];
typedef char WlibPeObjectSize[(sizeof(pe_object) == 40) ? 1 : -1];
extern int ImportInputHandle;
extern unsigned char *ImportReadBuffer, *ImportRecordBuffer;
extern char *ImportModuleName;
extern unsigned ImportModuleNameLength;
extern char ImportTemporaryName[13];
int ReadImportData( void *buffer, unsigned length );
unsigned char ReadImportByte( void );
int SeekImportInput( unsigned long offset );
int CloseImportInput( void );
int ReadImportModuleName( unsigned wide_ordinal );
int ConvertNlmExports( int handle, unsigned long count );
int ConvertNameTable( int handle, unsigned by_name );
int ConvertPeExports( int handle, unsigned long *offset, unsigned char by_name );
int ReadImportNameAtOffset( unsigned long offset );
void WriteImportSymbol( const char *name, unsigned length, int ordinal, int handle, unsigned char by_name );
int ConvertPeNameBlock( unsigned long start, unsigned long end, unsigned long ordinals,
                        unsigned count, unsigned ordinal_base, int handle, unsigned char by_name );
#endif
