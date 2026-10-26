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
* Description:  Old ELF386 load headers, groups, imports and relocation metadata.
*
****************************************************************************/

#include <string.h>
#include "cmdline.h"
#include "dbgdwarf.h"
#include "strtab.h"
#include <recon.h>
#ifdef __WATCOMC__
#pragma intrinsic(memcpy,strlen)
#endif

static unsigned long NumElfImports;       /* original441fb8 */

unsigned long AssignELFImportIndex( symbol *sym )
{
    ++NumElfImports;
    sym->u.import = (dll_sym_info *)NumElfImports;
    return NumElfImports;
}

extern address StartInfo;
extern void WriteLoad( void *, unsigned short );

void FiniELFLoadFile( void )
{
    elf32_header header;
    elf32_program *programs;
    unsigned long phoff, phsize, filepos, vabase;

    memcpy( header.ident, "\177ELF", 4 );
    header.ident[4] = header.ident[5] = header.ident[6] = 1;
    memset( header.ident + 7, 0, 9 );
    header.type = 2;
    header.machine = 3;
    header.version = 1;
    header.entry = StartInfo.off;
    header.phoff = sizeof(header);
    header.shoff = header.flags = 0;
    header.ehsize = sizeof(header);
    header.phentsize = sizeof(elf32_program);
    header.phnum = NumGroups;
    if( FmtData.u.elf.exports != 0 || NumElfImports > 0 ) ++header.phnum;
    header.shentsize = sizeof(elf32_section);
    header.shnum = header.shstrndx = 0;
    WriteLoad( &header, sizeof(header) );
    /* This old writer keeps phoff=52 even when the table is aligned later. */
    phoff = PadToAlign( sizeof(header), FmtData.u.elf.alignment );
    phsize = header.phnum * sizeof(elf32_program);
    programs = ChkLAlloc( phsize );
    filepos = (phoff + phsize + FmtData.u.elf.alignment - 1)
                                         & ~(FmtData.u.elf.alignment - 1);
    QSeek( Root->outfile->handle, filepos, Root->outfile->fname );
    filepos = WriteELFData( programs, filepos, &vabase );
    WriteELFDynamic( programs + NumGroups, filepos, vabase );
    QSeek( Root->outfile->handle, phoff, Root->outfile->fname );
    WriteLoad( programs, phsize );
    _LnkFree( programs );
}

unsigned long WriteELFData( elf32_program *program, unsigned long filepos,
                                                        unsigned long *vabase )
{
    group_entry *group;
    unsigned long end;
    *vabase = 0;
    for( group = Groups; group != 0; group = group->next_group ) {
        program->type = 1;
        program->offset = filepos;
        program->vaddr = group->grp_addr.off;
        program->paddr = 0;
        program->filesz = group->size;
        program->memsz = group->totalsize;
        end = (group->grp_addr.off + group->totalsize
                           + FmtData.u.elf.alignment - 1)
                                         & ~(FmtData.u.elf.alignment - 1);
        if( end > *vabase ) *vabase = end;
        program->flags = 4;
        if( group->segflags & 1 ) program->flags |= 2;
        else program->flags |= 1;
        program->align = FmtData.u.elf.alignment;
        QWrite( CurrSect->outfile->handle, (void *)group->data,
                                  group->size, CurrSect->outfile->fname );
        filepos = PadToAlign( filepos + group->size, FmtData.u.elf.alignment );
        ++program;
    }
    return filepos;
}

extern unsigned char DumpRelocList( reloc_info * );

void WriteELFDynamic( elf32_program *program, unsigned long filepos,
                                                      unsigned long vabase )
{
    elf32_dynamic *dynamic, *entry;
    snamelist *module;
    stringtable strings;
    symbol **indices;
    unsigned dynsize;
    unsigned long size;
    char *strdata;

    if( FmtData.u.elf.exports == 0 && NumElfImports == 0 ) return;
    program->offset = filepos;
    dynsize = 9;
    for( module = FmtData.u.elf.modules; module != 0; module = module->next ) {
        ++dynsize;
    }
    dynsize *= sizeof(*dynamic);
    dynamic = ChkLAlloc( dynsize );
    entry = dynamic;
    InitStringTable( &strings );
    for( module = FmtData.u.elf.modules; module != 0; module = module->next ) {
        entry->tag = 1;
        entry->value = GetStringTableSize( &strings );
        AddStringTable( &strings, module->name, module->len + 1 );
        ++entry;
    }
    DumpRelocList( Root->reloclist );
    entry->tag = 17;
    entry->value = filepos;
    ++entry;
    entry->tag = 18;
    entry->value = Root->relocs * 8;
    filepos = PadToAlign( filepos + entry->value, FmtData.u.elf.alignment );
    ++entry;
    entry->tag = 19;
    entry->value = 8;
    ++entry;
    entry->tag = 4;
    entry->value = filepos;
    AssignELFExportIndices();
    indices = ChkLAlloc( (NumElfImports + 1) * sizeof(*indices) );
    size = WriteELFHash( indices );
    filepos = PadToAlign( filepos + size, FmtData.u.elf.alignment );
    ++entry;
    entry->tag = 11;
    entry->value = sizeof(elf32_symbol);
    ++entry;
    entry->tag = 6;
    entry->value = filepos;
    size = WriteELFSymbols( indices, &strings );
    filepos = PadToAlign( filepos + size, FmtData.u.elf.alignment );
    ++entry;
    _LnkFree( indices );
    entry->tag = 5;
    entry->value = filepos;
    ++entry;
    entry->tag = 10;
    entry->value = GetStringTableSize( &strings );
    strdata = GetStringTable( &strings );
    WriteLoad( strdata, entry->value );
    _LnkFree( strdata );
    filepos = PadToAlign( filepos + entry->value, FmtData.u.elf.alignment );
    ++entry;
    entry->tag = 0;
    entry->value = 0;
    WriteLoad( dynamic, dynsize );
    program->type = 2;
    program->paddr = 0;
    program->flags = 6;
    program->vaddr = vabase;
    program->filesz = program->memsz = filepos + dynsize - program->offset;
    program->align = FmtData.u.elf.alignment;
    _LnkFree( dynamic );
}

void AssignELFExportIndices( void )
{
    snamelist *entry;
    symbol *sym;
    for( entry = FmtData.u.elf.exports; entry != 0; entry = entry->next ) {
        sym = SymOp( 8, entry->name, entry->len );
        if( sym == 0 ) {
            LnkMsg( 0x322c, "s", entry->name );
        } else {
            ++NumElfImports;
            sym->u.import = (dll_sym_info *)NumElfImports;
        }
    }
}

extern symbol *HeadSym;

unsigned long WriteELFHash( symbol **indices )
{
    unsigned long count, *buckets, *chains, bucket, index;
    symbol *sym;

    count = ELFPrimeBuckets( NumElfImports >> 1 );
    buckets = ChkLAlloc( count * sizeof(*buckets) );
    chains = ChkLAlloc( (NumElfImports + 1) * sizeof(*chains) );
    memset( buckets, 0, count * sizeof(*buckets) );
    for( sym = HeadSym; sym != 0; sym = sym->link ) {
        if( sym->u.import != 0 && sym->u.import != (dll_sym_info *)-1 ) {
            index = (unsigned long)sym->u.import;
            indices[index] = sym;
            bucket = ELFHash( sym->name ) % count;
            chains[index] = buckets[bucket];
            buckets[bucket] = index;
        }
    }
    WriteLoad( &count, sizeof(count) );
    WriteLoad( &NumElfImports, sizeof(NumElfImports) );
    WriteLoad( buckets, count * sizeof(*buckets) );
    WriteLoad( chains + 1, NumElfImports * sizeof(*chains) );
    _LnkFree( buckets );
    _LnkFree( chains );
    return (count + NumElfImports) * 4 + 8;
}

const unsigned char ELFPrimes[] = {
        2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53,
        59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113,
        127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181,
        191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 0
    };

unsigned ELFPrimeBuckets( unsigned count )
{

    const unsigned char *p;
    for( p = ELFPrimes; *p <= count && p[1] != 0; ++p ) {
    }
    return *p;
}

unsigned long ELFHash( char *name )
{
    unsigned long hash, high;
    hash = 0;
    while( *name != 0 ) {
        hash = (hash << 4) + (unsigned char)*name++;
        high = hash & 0xf0000000UL;
        if( high != 0 ) hash ^= high >> 24;
        hash &= ~high;
    }
    return hash;
}

unsigned long WriteELFSymbols( symbol **indices, stringtable *strings )
{
    elf32_symbol entry;
    symbol *sym;
    unsigned long index;

    entry.size = 0;
    entry.info = 0x10;
    entry.other = 0;
    for( index = 1; index <= NumElfImports; ++index ) {
        sym = indices[index];
        entry.name = GetStringTableSize( strings );
        AddStringTable( strings, sym->name, strlen(sym->name) + 1 );
        entry.value = sym->addr.off;
        entry.shndx = (sym->info & 15) != 6;
        WriteLoad( &entry, sizeof(entry) );
    }
    return NumElfImports * sizeof(entry);
}
