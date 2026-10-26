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
* Description:  Phar Lap simple and extended executable writers.
*
****************************************************************************/

#ifndef WLINK_EXEPHAR_H
#define WLINK_EXEPHAR_H
#pragma pack(1)
typedef struct extended_header {
    unsigned short     signature;
    unsigned short     format_level;
    unsigned short     header_size;    // in bytes
    unsigned long     file_size;      // in bytes;
    unsigned short     checksum;
    unsigned long     rtp_offset;     // byte offset of run-time param block.
    unsigned long     rtp_size;       // byte size of run-time param block.
    unsigned long     reloc_offset;   // byte offset of relocation table.
    unsigned long     reloc_size;     // byte size of relocation table.
    unsigned long     sit_offset;     // byte offset of segment info table.
    unsigned long     sit_size;       // byte size of segment info table.
    unsigned short     sit_entry_size; // byte size of a seg info table entry
    unsigned long     load_offset;    // byte offset of load image in file.
    unsigned long     load_size;      // byte size of load image.
    unsigned long     sym_offset;     // byte offset of symbol table in file
    unsigned long     sym_size;       // byte size of symbol table.
    unsigned long     gdt_offset;     // byte offset of GDT in load image
    unsigned long     gdt_size;       // byte size of GDT in load image.
    unsigned long     ldt_offset;     // byte offset of LDT in load image.
    unsigned long     ldt_size;       // byte size of LDT in load image.
    unsigned long     idt_offset;     // byte offset of IDT in load image.
    unsigned long     idt_size;       // byte size of idt in load image.
    unsigned long     tss_offset;     // byte offset of tss in load image.
    unsigned long     tss_size;       // byte size of tss in load image.
    unsigned long     min_extra;      // minimum # of extra bytes to allocate.
    unsigned long     max_extra;      // maximum # of extra bytes to allocate.
    unsigned long     offset;         // offset of program image.
    unsigned long     ESP;
    unsigned short     SS;
    unsigned long     EIP;
    unsigned short     CS;
    unsigned short     LDT;
    unsigned short     TSS;
    unsigned short     flags;          // load image flags.
    unsigned long     mem_req;        // byte load image memory requirements
    unsigned long     reserved1;      // must be zero
    unsigned long     stack_size;     // initial stack size
} extended_header;


typedef struct seg_info_table {
    unsigned short selector, flags;
    unsigned long base, extra;
} seg_info_table;
typedef struct {
    unsigned short backlink;
    unsigned short _zero1;
    unsigned long esp0;
    unsigned short ss0;
    unsigned short _zero2;
    unsigned long esp1;
    unsigned short ss1;
    unsigned short _zero3;
    unsigned long esp2;
    unsigned short ss2;
    unsigned short _zero4;
    unsigned long cr3;
    unsigned long eip;
    unsigned long eflags;
    unsigned long eax;
    unsigned long ecx;
    unsigned long edx;
    unsigned long ebx;
    unsigned long esp;
    unsigned long ebp;
    unsigned long esi;
    unsigned long edi;
    unsigned short es;
    unsigned short _zero5;
    unsigned short cs;
    unsigned short _zero6;
    unsigned short ss;
    unsigned short _zero7;
    unsigned short ds;
    unsigned short _zero8;
    unsigned short fs;
    unsigned short _zero9;
    unsigned short gs;
    unsigned short _zero10;
    unsigned short ldt;
    unsigned short _zero11;
    unsigned short t;
    unsigned short iobase;
} TSS;

typedef struct {
    unsigned short limitlow;
    unsigned short baselow;
    unsigned char  basemid;
    unsigned char  bits1;
    unsigned char  bits2;
    unsigned char  basehigh;
} descriptor;


#pragma pack()
typedef char phar_extended_is_128[(sizeof(extended_header) == 128) ? 1 : -1];
typedef char phar_tss_is_104[(sizeof(TSS) == 104) ? 1 : -1];
typedef char phar_descriptor_is_8[(sizeof(descriptor) == 8) ? 1 : -1];
typedef char phar_sit_is_12[(sizeof(seg_info_table) == 12) ? 1 : -1];
#include <recon.h>
#endif
