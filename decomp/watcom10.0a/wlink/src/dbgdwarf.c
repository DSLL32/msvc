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


/*
 *  DBGDWARF -- routines for producing DWARF debugging information
 *
*/

#include <string.h>
#include <malloc.h>
#include "cmdline.h"
#include "dbgdwarf.h"
#include "strtab.h"
#include <recon.h>
#ifdef __WATCOMC__
#pragma intrinsic(strlen,memcpy)
#endif

/* The1994 module owns three direct-pointer section descriptors. */
typedef struct dwarfsect {
    unsigned char *addr;
    unsigned long size;
} dwarfsect;
typedef struct dwarfmodinfo {
    dwarfsect pubsym, dasi, arange;
} dwarfmodinfo;
typedef char old_dwarfmodinfo_is_24[(sizeof(dwarfmodinfo) == 24) ? 1 : -1];

void DwarfInitModule( mod_entry *mod )
{
    mod->d.d = PermAlloc( sizeof(dwarfmodinfo) );
    memset( mod->d.d, 0, sizeof(dwarfmodinfo) );
}

void DwarfAddAddrInfo( seg_leader *seg, unsigned long start )
{
    if( !(CurrMod->modinfo & 0x0800) && !(seg->info & 0x7000)
        && !(seg->info & 0x40) && seg->saved_size - start > 0 ) {
        ((dwarfmodinfo *)CurrMod->d.d)->arange.size += 10;
        seg->info |= 0x40;
    }
}

extern unsigned DWLineGen( long, long, char * );
void DwarfAddLines( seg_leader *seg, unsigned char is32, unsigned char isdata )
{
    unsigned char *line;
    unsigned short previous, number;
    unsigned long previous_offset, offset, size;
    long line_delta, address_delta;
    char buffer[16];

    if( CurrMod->modinfo & 0x0800 ) return;
    size = 12;
    previous = 1;
    previous_offset = 0;
    line = ObjBuff;
    while( line < EOObjRec ) {
        number = *(unsigned short *)line;
        if( is32 ) {
            offset = *(unsigned long *)(line + 2);
            line += 6;
        } else {
            offset = *(unsigned short *)(line + 2);
            line += 4;
        }
        line_delta = (long)number - previous;
        address_delta = offset - previous_offset;
        previous += line_delta;
        previous_offset += address_delta;
        size += DWLineGen( line_delta, address_delta, buffer );
    }
    if( isdata ) ((segdata *)seg)->line_size += size;
    else ((dwarfmodinfo *)CurrMod->d.d)->dasi.size += size;
}

void DwarfAddGlobal( symbol *sym, unsigned len )
{
    sym = sym;
    if( !(CurrMod->modinfo & 0x0800) )
        ((dwarfmodinfo *)CurrMod->d.d)->pubsym.size += len + 11;
}

void DwarfP1ModuleScanned( void )
{
    ResetAddrInfo();
}

typedef struct dwarfsectioninfo {
    unsigned char *addr;
    unsigned long size, start;
    char *name;
} dwarfsectioninfo;
enum { DWSECT_INFO, DWSECT_ABBREV, DWSECT_LINE, DWSECT_ARANGE };
static dwarfsectioninfo SectionTable[] = {
    { 0, 0, 0, ".debug_info" },
    { 0, 0, 0, ".debug_abbrev" },
    { 0, 0, 0, ".debug_line" },
    { 0, 0, 0, ".debug_aranges" }
};

void DwarfGenModule( void )
{
    dwarfmodinfo *info;

    if( CurrMod->modinfo & 0x0800 ) return;
    SectionTable[DWSECT_ABBREV].size = 42;
    info = CurrMod->d.d;
    if( info->dasi.size > 0 ) {
        info->dasi.addr = (unsigned char *)SectionTable[DWSECT_LINE].size;
        info->dasi.size += strlen( CurrMod->name ) + 30;
        SectionTable[DWSECT_LINE].size += info->dasi.size;
        info->pubsym.size += 4;
    }
    info->pubsym.addr = (unsigned char *)SectionTable[DWSECT_INFO].size;
    info->pubsym.size += strlen( CurrMod->name ) + 14;
    SectionTable[DWSECT_INFO].size += info->pubsym.size;
    if( info->arange.size > 0 ) {
        info->arange.addr = (unsigned char *)SectionTable[DWSECT_ARANGE].size;
        info->arange.size += 22;
        SectionTable[DWSECT_ARANGE].size += info->arange.size;
    }
}

static class_entry *DBIClass;  /* original4503a0 */
void DwarfDefClass( class_entry *class, unsigned long size )
{
    seg_leader *seg;
    unsigned index;

    size = size;
    DBIClass = class;
    for( seg = class->segs; seg != 0; seg = seg->next_seg ) {
        seg->data = (unsigned long)DBIAlloc( seg->saved_size );
        if( SectionTable[DWSECT_INFO].size > 0 ) {
            for( index = 0; index < 4; ++index ) {
                if( stricmp( seg->segname->name, SectionTable[index].name ) == 0 ) {
                    seg->info = (seg->info & ~0x7000) | ((index + 1) << 12);
                    SectionTable[index].start = seg->saved_size;
                    break;
                }
            }
        }
    }
}

void DwarfAddrSectStart( section *sect )
{
    unsigned index;

    if( sect != Root || SectionTable[DWSECT_ABBREV].size == 0 ) return;
    for( index = 0; index < 4; ++index ) {
        if( index != DWSECT_ABBREV )
            SectionTable[index].addr = (unsigned char *)DBIAlloc( SectionTable[index].size );
    }
}

#pragma pack(1)
typedef struct dwarf_arange_header {
    unsigned long length;
    unsigned short version;
    unsigned long debug_offset;
    unsigned char offset_size, segment_size;
} dwarf_arange_header;
typedef struct dwarf_compunit_header {
    unsigned long length;
    unsigned short version;
    unsigned long abbrev_offset;
    unsigned char address_size;
} dwarf_compunit_header;
typedef struct dwarf_line_header {
    unsigned long length;
    unsigned short version;
    unsigned long prologue_length;
    unsigned char min_instruction_length, default_statement;
    signed char line_base;
    unsigned char line_range, opcode_base, opcode_lengths[9];
} dwarf_line_header;
#pragma pack()

void DwarfAddModule( mod_entry *mod, section *sect )
{
    dwarfmodinfo *info;
    dwarf_arange_header arange;
    dwarf_compunit_header unit;
    dwarf_line_header line;
    unsigned namelen;
    unsigned char abbrev;
    unsigned long statement;
    char *buffer;

    sect = sect;
    if( mod->modinfo & 0x0800 ) return;
    namelen = strlen( mod->name );
    info = mod->d.d;
    if( info->arange.size > 0 ) {
        info->arange.addr += (unsigned long)SectionTable[DWSECT_ARANGE].addr;
        arange.length = info->arange.size - 4;
        arange.version = 2;
        arange.debug_offset = (unsigned long)info->pubsym.addr + SectionTable[DWSECT_INFO].start;
        arange.offset_size = 4;
        arange.segment_size = 2;
        memcpy( info->arange.addr, &arange, sizeof(arange) );
        info->arange.addr += sizeof(arange);
    }
    info->pubsym.addr += (unsigned long)SectionTable[DWSECT_INFO].addr;
    unit.length = info->pubsym.size - 4;
    unit.version = 2;
    unit.abbrev_offset = SectionTable[DWSECT_ABBREV].start;
    unit.address_size = 4;
    memcpy( info->pubsym.addr, &unit, sizeof(unit) );
    info->pubsym.addr += sizeof(unit);
    abbrev = info->dasi.size > 0 ? 1 : 2;
    memcpy( info->pubsym.addr, &abbrev, 1 );
    ++info->pubsym.addr;
    if( info->dasi.size > 0 ) {
        statement = (unsigned long)info->dasi.addr + SectionTable[DWSECT_LINE].start;
        memcpy( info->pubsym.addr, &statement, sizeof(statement) );
        info->pubsym.addr += sizeof(statement);
    }
    memcpy( info->pubsym.addr, mod->name, namelen + 1 );
    info->pubsym.addr += namelen + 1;
    if( info->dasi.size > 0 ) {
        info->dasi.addr += (unsigned long)SectionTable[DWSECT_LINE].addr;
        line.length = info->dasi.size - 4;
        line.version = 2;
        line.prologue_length = namelen + 16;
        line.min_instruction_length = 1;
        line.default_statement = 0;
        line.line_base = -1;
        line.line_range = 4;
        line.opcode_base = 10;
        line.opcode_lengths[0] = 0;
        line.opcode_lengths[1] = 1;
        line.opcode_lengths[2] = 1;
        line.opcode_lengths[3] = 1;
        line.opcode_lengths[4] = 1;
        line.opcode_lengths[5] = 0;
        line.opcode_lengths[6] = 0;
        line.opcode_lengths[7] = 0;
        line.opcode_lengths[8] = 0;
        memcpy( info->dasi.addr, &line, sizeof(line) );
        info->dasi.addr += sizeof(line);
        buffer = alloca( namelen + 6 );
        buffer[0] = 0;
        memcpy( buffer + 1, mod->name, namelen + 1 );
        buffer[namelen + 2] = 0;
        buffer[namelen + 3] = 0;
        buffer[namelen + 4] = 0;
        buffer[namelen + 5] = 0;
        memcpy( info->dasi.addr, buffer, namelen + 6 );
        info->dasi.addr += namelen + 6;
    }
}

#pragma pack(1)
typedef struct dwarf_symbol_die {
    unsigned char abbreviation;
    unsigned long offset;
    unsigned char expression_size, operation;
    unsigned short segment;
    unsigned char external;
} dwarf_symbol_die;
#pragma pack()
void DwarfGenGlobal( symbol *sym, section *sect )
{
    dwarf_symbol_die die;
    dwarfmodinfo *info;
    unsigned length;

    sect = sect;
    if( CurrMod->modinfo & 0x0800 ) return;
    die.abbreviation = sym->p.seg == 0 || (sym->p.leader->info & 0x0200) ? 3 : 4;
    die.offset = sym->addr.off;
    die.expression_size = 3;
    die.operation = 10;
    die.segment = sym->addr.seg;
    die.external = !(sym->info & 0x0100);
    info = CurrMod->d.d;
    memcpy( info->pubsym.addr, &die, sizeof(die) );
    info->pubsym.addr += sizeof(die);
    length = strlen( sym->name ) + 1;
    memcpy( info->pubsym.addr, sym->name, length );
    info->pubsym.addr += length;
}

void DwarfGenAddrInfo( seg_leader *seg, unsigned long start, unsigned long padding )
{
    unsigned long size;

    if( (CurrMod->modinfo & 0x0800) || (seg->info & 0x7000) ) return;
    size = seg->size - start;
    if( size + padding > 0 ) {
        if( seg->info & 0x40 ) seg->u.p2.prev_offset += size + padding;
        else if( size > 0 ) {
            seg->u.p2.prev_offset = size;
            seg->info |= 0x40;
        }
    }
}

void DwarfGenLines( seg_leader *seg, unsigned long adjust, unsigned char is32 )
{
    dwarfmodinfo *info;
    unsigned char *line, *dest;
    unsigned short previous, number;
    unsigned long previous_offset, offset;
    long line_delta, address_delta;
    unsigned size;
    char buffer[16];

    if( CurrMod->modinfo & 0x0800 ) return;
    info = CurrMod->d.d;
    dest = info->dasi.addr;
    previous = 1;
    previous_offset = 0;
    buffer[0] = 0;
    buffer[1] = 5;
    buffer[2] = 2;
    *(unsigned long *)(buffer + 3) = adjust;
    *(unsigned short *)(buffer + 7) = seg->seg_addr.seg;
    memcpy( dest, buffer, 9 );
    dest += 9;
    for( line = ObjBuff; line < EOObjRec; ) {
        number = *(unsigned short *)line;
        if( is32 ) {
            offset = *(unsigned long *)(line + 2);
            line += 6;
        } else {
            offset = *(unsigned short *)(line + 2);
            line += 4;
        }
        line_delta = (long)number - previous;
        address_delta = offset - previous_offset;
        previous += line_delta;
        previous_offset += address_delta;
        size = DWLineGen( line_delta, address_delta, buffer );
        memcpy( dest, buffer, size );
        dest += size;
    }
    buffer[0] = 0;
    buffer[1] = 1;
    buffer[2] = 1;
    memcpy( dest, buffer, 3 );
    info->dasi.addr = dest + 3;
}

#pragma pack(1)
typedef struct dwarf_arange_tuple {
    unsigned long offset;
    unsigned short segment;
    unsigned long size;
} dwarf_arange_tuple;
#pragma pack()
extern void DwarfGenSegInfo( seg_leader * );
extern void ProcModSegments( void (*)(seg_leader *) );
void DwarfP2Module( void )
{
    dwarfmodinfo *info;
    dwarf_arange_tuple tuple;
    unsigned char null_die;

    if( CurrMod->modinfo & 0x0800 ) return;
    ProcModSegments( DwarfGenSegInfo );
    info = CurrMod->d.d;
    if( info->arange.size > 0 ) {
        memset( &tuple, 0, sizeof(tuple) );
        memcpy( info->arange.addr, &tuple, sizeof(tuple) );
    }
    null_die = 0;
    memcpy( info->pubsym.addr, &null_die, 1 );
}

extern void WriteLoad( void *, unsigned );
extern unsigned long WriteELFSections( unsigned long, elf32_section * );
void DwarfWriteDBI( void )
{
    elf32_header header;
    elf32_section *sections;
    unsigned size, index;
    unsigned long savepos, virt;
    seg_leader *seg;
    struct {
        unsigned long signature, vendor, type, size;
    } trailer;

    memcpy( header.ident, "\177ELF", 4 );
    header.ident[4] = header.ident[5] = header.ident[6] = 1;
    memset( header.ident + 7, 0, 9 );
    header.type = 2;
    header.machine = 3;
    header.version = 1;
    header.entry = header.phoff = header.flags = 0;
    header.shoff = sizeof(header);
    header.ehsize = sizeof(header);
    header.phentsize = 32;
    header.phnum = 0;
    header.shentsize = sizeof(*sections);
    header.shnum = 2;
    if( DBIClass != NULL ) {
        for( seg = DBIClass->segs; seg != NULL; seg = seg->next_seg ) {
            if( seg->data != 0 ) ++header.shnum;
        }
    }
    for( index = 0; index < 4; ++index ) {
        if( SectionTable[index].size != 0 && SectionTable[index].start == 0 ) {
            ++header.shnum;
        }
    }
    if( LinkFlags & 0x80 ) ++header.shnum;
    header.shstrndx = header.shnum - 1;
    WriteLoad( &header, sizeof(header) );
    size = header.shnum * sizeof(*sections);
    sections = ChkLAlloc( size );
    savepos = QLSeek( Root->outfile->handle, size, 1, Root->outfile->fname );
    virt = WriteELFSections( sizeof(header) + size, sections );
    trailer.signature = 0x534954;
    trailer.vendor = trailer.type = 0;
    trailer.size = virt + sizeof(trailer);
    WriteLoad( &trailer, sizeof(trailer) );
    QSeek( Root->outfile->handle, savepos - size, Root->outfile->fname );
    WriteLoad( sections, (unsigned short)size );
    _LnkFree( sections );
}

extern void FillHeader( elf32_section *, char *, stringtable *, unsigned long );
/* Abbreviations consumed by the old segmented DWARF writer. */
enum {
    DW_TAG_compile_unit = 0x11, DW_TAG_label = 0x0a, DW_TAG_variable = 0x34,
    DW_AT_stmt_list = 0x10, DW_AT_name = 3, DW_AT_low_pc = 0x11,
    DW_AT_segment = 0x46, DW_AT_external = 0x3f,
    DW_FORM_ref_addr = 0x10, DW_FORM_string = 8, DW_FORM_addr = 1,
    DW_FORM_block1 = 0x0a, DW_FORM_flag = 0x0c
};
const unsigned char StandardAbbrevs[] = {
    1, DW_TAG_compile_unit, 1,
    DW_AT_stmt_list, DW_FORM_ref_addr, DW_AT_name, DW_FORM_string, 0, 0,
    2, DW_TAG_compile_unit, 1, DW_AT_name, DW_FORM_string, 0, 0,
    3, DW_TAG_label, 0, DW_AT_low_pc, DW_FORM_addr,
    DW_AT_segment, DW_FORM_block1, DW_AT_external, DW_FORM_flag,
    DW_AT_name, DW_FORM_string, 0, 0,
    4, DW_TAG_variable, 0, DW_AT_low_pc, DW_FORM_addr,
    DW_AT_segment, DW_FORM_block1, DW_AT_external, DW_FORM_flag,
    DW_AT_name, DW_FORM_string, 0, 0
};
unsigned long WriteELFSections( unsigned long curr, elf32_section *hdr )
{
    stringtable strtab;
    seg_leader *seg;
    unsigned index;
    unsigned long size;
    char *strings;

    memset( hdr, 0, sizeof(*hdr) );
    InitStringTable( &strtab );
    CharStringTable( &strtab, 0 );
    ++hdr;
    if( DBIClass != NULL ) {
        for( seg = DBIClass->segs; seg != NULL; seg = seg->next_seg ) {
            size = 0;
            index = ((seg->info & 0x7000) >> 12) - 1;
            if( index < 4 ) size = SectionTable[index].size;
            if( seg->data != 0 ) {
                FillHeader( hdr, seg->segname->name, &strtab, curr );
                hdr->size = seg->saved_size + size;
                QWrite( CurrSect->outfile->handle, (void *)seg->data,
                        seg->saved_size, CurrSect->outfile->fname );
                if( size != 0 ) {
                    if( index == DWSECT_ABBREV ) {
                        WriteLoad( (void *)StandardAbbrevs, (unsigned short)size );
                    } else {
                        QWrite( CurrSect->outfile->handle, SectionTable[index].addr,
                                size, CurrSect->outfile->fname );
                    }
                }
                curr += hdr->size;
                ++hdr;
            }
        }
    }
    for( index = 0; index < 4; ++index ) {
        if( SectionTable[index].size != 0 && SectionTable[index].start == 0 ) {
            FillHeader( hdr, SectionTable[index].name, &strtab, curr );
            hdr->size = SectionTable[index].size;
            if( index == DWSECT_ABBREV ) {
                WriteLoad( (void *)StandardAbbrevs, (unsigned short)hdr->size );
            } else {
                QWrite( CurrSect->outfile->handle, SectionTable[index].addr,
                        hdr->size, CurrSect->outfile->fname );
            }
            curr += hdr->size;
            ++hdr;
        }
    }
    FillHeader( hdr, ".shstrtab", &strtab, curr );
    hdr->type = 3;
    hdr->size = GetStringTableSize( &strtab );
    strings = GetStringTable( &strtab );
    WriteLoad( strings, (unsigned short)hdr->size );
    _LnkFree( strings );
    return curr + hdr->size;
}

void FillHeader( elf32_section *header, char *name, stringtable *table, unsigned long curr )
{
    header->name = GetStringTableSize( table );
    AddStringTable( table, name, strlen(name) + 1 );
    header->type = 1;
    header->flags = 0;
    header->addr = 0;
    header->offset = curr;
    header->link = 0;
    header->info = 0;
    header->align = 0;
    header->entsize = 0;
}
