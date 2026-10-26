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

#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "peheader.h"
#include "peimports.h"
#include "cmdline.h"
#include <recon.h>

#if defined(__WATCOMC__)
#pragma intrinsic(strlen,memcpy)
void reconstruction_trap( void );
#pragma aux reconstruction_trap = 0xcc;
#define TRAP() reconstruction_trap()
#else
#define TRAP() __asm__ __volatile__( "int3" )
#endif

static mod_entry *TransferMod;           /* original4502e0 */
static unsigned long TransferSize;       /* original4502e4 */
static list_of_names *TransferName;      /* original4502f4 */

void ChkPEData( void )
{
    symbol *sym;
    class_entry *class, *code;
    segdata sdata;
    unsigned long offset;

    ChkOS2Data();
    code = 0;
    for( class = Root->classlist; class != 0; class = class->next_class ) {
        if( class->flags & 0x10 ) code = class;
    }
    TransferMod = NewModEntry();
    TransferMod->modinfo = 0x8017;
    TransferMod->name = "TRANSFER CODE";
    DBIInitModule( TransferMod );
    CurrMod = TransferMod;
    TransferSize = 0;
    for( sym = HeadSym; sym != 0; sym = sym->link ) {
        if( (sym->info & 15) == 6 ) {
            TransferSize += 6;
            DBIAddGlobal( sym, strlen( sym->name ) );
        }
    }
    if( TransferSize != 0 ) {
        /* The old no-code/import combination reads an uninitialized code
         * pointer. Keep that unqualified path trapped in both builds. */
        if( code == 0 ) TRAP();
        code->flags |= 2;
        TransferName = ChkLAlloc( 20 );
        TransferName->next_name = SavedNames;
        SavedNames = TransferName;
        memcpy( TransferName->name, "TRANSFER CODE", 14 );
        sdata.length = TransferSize;
        sdata.u.name = TransferName;
        sdata.cl.entry = code;
        sdata.acbp = 0x29;
        /* Only absolute bit8 is read by this old AddSegment path; its other
         * stack flag bits are originally undefined and are zeroed here. */
        sdata.flags = 0;
        AddSegment( &sdata, 0, Root, &offset );
        sdata.u.leader->info &= ~0x40;
        DBIGenModule( TransferMod );
    }
    CurrMod = 0;
}

unsigned long GetPEHeaderSize( void )
{
    unsigned long size;

    size = 248 + FindNumObjects() * 40;
    size += GetStubSize();
    return (size + FmtData.u.pe.objalign - 1) & ~(FmtData.u.pe.objalign - 1);
}

/* Old writer identifies the synthetic import object by a selector number. */
static unsigned short IDataSeg;             /* original4502fc */

unsigned FindNumObjects( void )
{
    unsigned num;

    num = NumGroups;
    if( IDataSeg != 0xffff ) ++num;
    if( LinkState & 1 ) ++num;
    if( FmtData.u.os2.exports != 0 ) ++num;
    if( FmtData.u.pe.dgroupsplitseg != 0 ) ++num;
    if( LinkFlags & 0x20 ) ++num;
    if( FmtData.u.os2.description != 0 ) ++num;
    return num;
}

static unsigned long IDataOffset;           /* original4502f8 */
extern void ChkOS2Exports( void );
extern void DBIAddModule( mod_entry *, section * );
extern void DBIGenGlobal( symbol *, section * );
extern void RegisterImport( dll_sym_info * );

void AllocPETransferTable( void )
{
    class_entry *class;
    seg_leader *lead;
    group_entry *group;
    symbol *sym;
    unsigned long off;
    unsigned short seg;

    ChkOS2Exports();
    class = Root->classlist;
    for( ;; ) {
        if( class == 0 ) {
            IDataSeg = 0xffff;
            return;
        }
        if( class->flags & 2 ) break;
        class = class->next_class;
    }
    lead = class->segs;
    while( lead->next_seg != 0 ) lead = lead->next_seg;
    CurrMod = TransferMod;
    DBIAddModule( TransferMod, Root );
    group = lead->group;
    seg = group->grp_addr.seg;
    off = group->grp_addr.off + group->totalsize;
    for( sym = HeadSym; sym != 0; sym = sym->link ) {
        if( (sym->info & 15) == 6 ) {
            sym->addr.seg = seg;
            off -= 6;
            sym->addr.off = off;
            RegisterImport( sym->u.import );
            DBIGenGlobal( sym, Root );
        }
    }
    IDataSeg = seg;
    IDataOffset = off;
    CurrMod = 0;
}

extern void DBIP2Module( void );

void WritePETransferTable( void )
{
    class_entry *class, *code;
    segdata sdata;
    segnode *node;
    unsigned long offset;

    code = 0;
    for( class = Root->classlist; class != 0; class = class->next_class ) {
        if( class->flags & 0x10 ) code = class;
    }
    if( TransferSize != 0 ) {
        /* As in ChkPEData, the original has no initialized code-class
         * pointer if no code class exists. That path is not qualified. */
        if( code == 0 ) TRAP();
        CurrMod = TransferMod;
        sdata.length = TransferSize;
        sdata.cl.entry = code;
        sdata.u.name = TransferName;
        sdata.acbp = 0x29;
        /* Only the cleared absolute bit is consumed from this old
         * uninitialized flag byte; its unused bits are zeroed here. */
        sdata.flags = 0;
        node = AllocNode( SegNodes );
        node->entry = AddSegment( &sdata, node, Root, &offset );
        DBIP2Module();
        FreeNodes( SegNodes );
        CurrMod = 0;
    }
}

extern unsigned long Write_Stub_File( void );
extern void PadLoad( unsigned long size );
extern void GenPETransferTable( unsigned long align );
extern void WriteDataPages( pe_header *header, pe_object *object );
extern void WriteImportInfo( pe_header *header, pe_object *object );
extern void WriteExportInfo( pe_header *header, pe_object *object );
extern void WriteFixupInfo( pe_header *header, pe_object *object );
extern void WriteDescription( pe_header *header, pe_object *object );
extern void WriteDebugTable( pe_header *header, pe_object *object );
extern unsigned long NullAlign( unsigned long align );
extern void WriteDBI( void );
extern void WriteLoad( void *buffer, unsigned short size );

void FiniPELoadFile( void )
{
    pe_header header;
    pe_object *object, *table_object;
    unsigned num_objects, head_size;
    unsigned long stub_len;

    num_objects = FindNumObjects();
    head_size = (LinkFlags & 0x800000UL) ? 192 : sizeof( pe_header );
    memset( &header, 0, head_size );
    header.signature = (FmtData.u.pe.flags & 1) ? 0x4c50 : 0x4550;
    header.cpu_type = 0x14c;
    header.num_objects = num_objects;
    header.magic = 0x10b;
    header.time_stamp = time( 0 );
    header.flags = 0x8180;
    header.nt_hdr_size = head_size - 24;
    if( !(LinkState & 1) ) header.flags |= 0x200;
    if( !(LinkState & 8) ) header.flags |= 2;
    if( FmtData.flags & 1 ) {
        header.flags |= 0x2000;
        if( FmtData.u.os2.flags & 4 ) header.dll_flags |= 1;
        else if( FmtData.u.os2.flags & 0x80 ) header.dll_flags |= 4;
        if( FmtData.u.os2.flags & 0x8000 ) header.dll_flags |= 2;
        else if( FmtData.u.os2.flags & 0x100 ) header.dll_flags |= 8;
    }
    header.lnk_major = 2;
    header.lnk_minor = 0x12;
    header.image_base = FmtData.base;
    header.object_align = FmtData.u.pe.objalign;
    if( FmtData.u.os2.segment_shift < 9 ) FmtData.u.os2.segment_shift = 9;
    if( FmtData.u.os2.segment_shift < 9 ) {
        LnkMsg( 0x323c, "s", "alignment" );
        FmtData.u.os2.segment_shift = 9;
    }
    header.file_align = 1UL << FmtData.u.os2.segment_shift;
    header.os_major = 1;
    header.os_minor = 11;
    header.user_major = FmtData.major;
    header.user_minor = FmtData.minor;
    header.subsys_major = 3;
    header.subsys_minor = 10;
    header.subsystem = FmtData.u.pe.subsystem ? FmtData.u.pe.subsystem : 2;
    header.stack_reserve_size = StackSize;
    header.stack_commit_size = FmtData.u.pe.stackcommit > StackSize ? StackSize : FmtData.u.pe.stackcommit;
    header.heap_reserve_size = FmtData.u.os2.heapsize;
    header.heap_commit_size = FmtData.u.pe.heapcommit > FmtData.u.os2.heapsize ? FmtData.u.os2.heapsize : FmtData.u.pe.heapcommit;
    header.num_tables = (LinkFlags & 0x800000UL) ? 9 : 16;
    CurrSect = Root;
    QSeek( Root->outfile->handle, 0, Root->outfile->fname );
    stub_len = Write_Stub_File();
    object = ChkLAlloc( num_objects * sizeof( pe_object ) );
    memset( object, 0, num_objects * sizeof( pe_object ) );
    PadLoad( head_size + num_objects * sizeof( pe_object ) );
    GenPETransferTable( FmtData.u.pe.objalign );
    WriteDataPages( &header, object );
    table_object = object + NumGroups;
    if( FmtData.u.pe.dgroupsplitseg != 0 ) ++table_object;
    if( IDataSeg != 0xffff ) WriteImportInfo( &header, table_object++ );
    if( FmtData.u.os2.exports != 0 ) WriteExportInfo( &header, table_object++ );
    if( LinkState & 1 ) WriteFixupInfo( &header, table_object++ );
    if( FmtData.u.os2.description != 0 ) WriteDescription( &header, table_object++ );
    if( LinkFlags & 0x20 ) WriteDebugTable( &header, table_object );
    NullAlign( header.file_align );
    header.header_size = object->physical_offset;
    WriteDBI();
    QSeek( Root->outfile->handle, stub_len, Root->outfile->fname );
    WriteLoad( &header, head_size );
    WriteLoad( object, num_objects * sizeof( pe_object ) );
    _LnkFree( object );
}

extern seg_leader *StackSegPtr;
extern unsigned long CalcImpOff( dll_sym_info * );
extern void WriteReloc( group_entry *, unsigned long, void *, unsigned short );
#pragma pack(1)
typedef struct i386_transfer {
    unsigned short opcode;
    unsigned long dest;
} i386_transfer;
#pragma pack()
typedef char i386_transfer_is_6[(sizeof(i386_transfer) == 6) ? 1 : -1];

void GenPETransferTable( unsigned long align )
{
    group_entry *group, *idata;
    symbol *sym;
    unsigned long last, size, end, base, off, first;
    unsigned short fixup;
    i386_transfer transfer;

    if( IDataSeg == 0xffff ) return;
    last = 0;
    idata = 0;
    for( group = Groups; group != 0; group = group->next_group ) {
        if( group->grp_addr.seg == IDataSeg ) idata = group;
        size = group->totalsize;
        if( StackSegPtr != 0 && StackSegPtr->group == group ) size -= StackSize;
        end = ((size + align - 1) & ~(align - 1)) + group->linear;
        if( end > last ) last = end;
    }
    /* A selected import selector must have its actual group. The old code
     * otherwise dereferences an uninitialized local; that path is unproved. */
    if( idata == 0 ) TRAP();
    base = last + FmtData.base;
    transfer.opcode = 0x25ff;
    first = ~0UL;
    for( sym = HeadSym; sym != 0; sym = sym->link ) {
        if( (sym->info & 15) == 6 ) {
            transfer.dest = base + CalcImpOff( sym->u.import );
            if( LinkState & 1 ) {
                fixup = ((idata->linear + sym->addr.off + 2) & 0xfff) | 0x3000;
                WriteReloc( idata, sym->addr.off + 2, &fixup, 2 );
            }
            off = sym->addr.off - idata->grp_addr.off;
            if( off < first ) first = off;
            memcpy( (void *)(idata->data + off), &transfer, sizeof( transfer ) );
        }
    }
    if( first > idata->size ) {
        memset( (void *)(idata->data + idata->size), 0, first - idata->size );
    }
    idata->size = idata->totalsize;
}

extern address StartInfo;

void WriteDataPages( pe_header *header, pe_object *object )
{
    group_entry *group;
    char *name;
    unsigned long size, linear;
    unsigned char have_group;

    header->code_base = ~0UL;
    header->data_base = ~0UL;
    have_group = 0;
    for( group = Groups; group != 0; group = group->next_group ) {
        if( group->totalsize == 0 ) continue;
        name = group->name;
        if( name[0] == 0 ) {
            name = group->leaders[0]->segname->name;
            if( name == 0 ) name = "";
        }
        strncpy( (char *)object->name, name, 8 );
        size = group->totalsize;
        if( group == DataGroup && FmtData.u.pe.dgroupsplitseg != 0 ) {
            size = FmtData.u.pe.dgroupsplitseg->seg_addr.off - DataGroup->grp_addr.off
                 - FmtData.u.pe.bsspad;
        }
        linear = ((size + header->object_align - 1) & ~(header->object_align - 1)) + group->linear;
        have_group = 1;
        if( StartInfo.seg == group->grp_addr.seg ) header->entry_rva = group->linear + StartInfo.off;
        object->rva = group->linear;
        object->physical_size = (size + header->file_align - 1) & ~(header->file_align - 1);
        object->flags = 0;
        if( group->segflags & 1 ) {
            object->flags |= 0x40000040UL;
            if( !(group->segflags & 0x80) ) object->flags |= 0x80000000UL;
            header->init_data_size += object->physical_size;
            if( object->rva < header->data_base ) header->data_base = object->rva;
        } else {
            object->flags |= 0x20000020UL;
            if( !(group->segflags & 0x80) ) object->flags |= 0x40000000UL;
            header->code_size += object->physical_size;
            if( object->rva < header->code_base ) header->code_base = object->rva;
        }
        if( group->segflags & 0x20 ) object->flags |= 0x10000000UL;
        if( size != 0 ) {
            object->physical_offset = NullAlign( header->file_align );
            QWrite( CurrSect->outfile->handle, (void *)group->data,
                    group->size, CurrSect->outfile->fname );
            PadLoad( size - group->size );
        }
        ++object;
        if( group == DataGroup && FmtData.u.pe.dgroupsplitseg != 0 ) {
            strncpy( (char *)object->name, ".bss", 8 );
            size = DataGroup->totalsize - (FmtData.u.pe.dgroupsplitseg->seg_addr.off - DataGroup->grp_addr.off);
            if( StackSegPtr != 0 ) size -= StackSize;
            object->rva = linear;
            linear += (size + header->object_align - 1) & ~(header->object_align - 1);
            object->physical_size = (size + header->file_align - 1) & ~(header->file_align - 1);
            object->flags = 0x40000080UL;
            if( !(group->segflags & 0x80) ) object->flags |= 0x80000000UL;
            if( group->segflags & 0x20 ) object->flags |= 0x10000000UL;
            header->uninit_data_size = object->physical_size;
            ++object;
        }
    }
    /* The original leaves linear undefined with no nonempty group. */
    if( !have_group ) TRAP();
    header->image_size = linear;
    if( header->code_base == ~0UL ) header->code_base = 0;
    if( header->data_base == ~0UL ) header->data_base = 0;
}

void WriteImportInfo( pe_header *header, pe_object *object )
{
    pe_import_directory dir;
    module_import *mod;
    import_name *imp;
    unsigned long table_rva, names_rva, next_name, value, hint, size;
    unsigned pass;

    strncpy( (char *)object->name, ".idata", 8 );
    object->physical_offset = NullAlign( header->file_align );
    object->rva = header->image_size;
    object->flags = 0xc0000040UL;
    table_rva = object->rva + (NumMods + 1) * sizeof( dir );
    names_rva = table_rva + (NumImports + NumMods) * 8;
    dir.time_stamp = 0;
    dir.major = 0;
    dir.minor = 0;
    for( mod = PEImpList; mod != 0; mod = mod->next ) {
        dir.name_rva = names_rva;
        dir.import_lookup_table_rva = table_rva;
        table_rva += (mod->num_entries + 1) * 4;
        dir.import_address_table_rva = table_rva;
        table_rva += (mod->num_entries + 1) * 4;
        names_rva += mod->mod->len + 1;
        WriteLoad( &dir, sizeof( dir ) );
    }
    memset( &dir, 0, sizeof( dir ) );
    WriteLoad( &dir, sizeof( dir ) );
    for( mod = PEImpList; mod != 0; mod = mod->next ) {
        for( pass = 0; pass < 2; ++pass ) {
            next_name = names_rva;
            for( imp = mod->imports; imp != 0; imp = imp->next ) {
                if( imp->imp != 0 ) {
                    next_name += next_name & 1;
                    value = next_name;
                    next_name += imp->imp->len + 3;
                } else {
                    value = imp->dll->entry | 0x80000000UL;
                }
                WriteLoad( &value, sizeof( value ) );
            }
            value = 0;
            WriteLoad( &value, sizeof( value ) );
        }
        names_rva = next_name;
    }
    for( mod = PEImpList; mod != 0; mod = mod->next ) {
        WriteLoad( mod->mod->name, mod->mod->len + 1 );
    }
    for( mod = PEImpList; mod != 0; mod = mod->next ) {
        hint = 1;
        for( imp = mod->imports; imp != 0; imp = imp->next ) {
            if( imp->imp != 0 ) {
                NullAlign( 2 );
                WriteLoad( &hint, 2 );
                WriteLoad( imp->imp->name, imp->imp->len );
                PadLoad( 1 );
                ++hint;
            }
        }
    }
    size = names_rva - object->rva;
    object->physical_size = (size + header->file_align - 1) & ~(header->file_align - 1);
    header->table[PE_TBL_IMPORT].size = size;
    header->table[PE_TBL_IMPORT].rva = object->rva;
    header->image_size += (size + header->object_align - 1) & ~(header->object_align - 1);
}

extern char *RemovePath( char *, int * );
extern int namecmp( const void *, const void * );

void WriteExportInfo( pe_header *header, pe_object *object )
{
    pe_export_directory dir;
    char *name;
    int namelen;
    entry_export **sort, *exp;
    unsigned i, num_entries, high_ord, next_ord;
    unsigned short ord;
    unsigned long eat, size;

    strncpy( (char *)object->name, ".edata", 8 );
    object->physical_offset = NullAlign( header->file_align );
    object->rva = header->image_size;
    object->flags = 0x40000040UL;
    dir.flags = 0;
    dir.time_stamp = 0;
    dir.major = 0;
    dir.minor = 0;
    dir.name_rva = object->rva + sizeof( dir );
    dir.ordinal_base = FmtData.u.os2.exports->ordinal;
    if( FmtData.u.os2.res_module_name != 0 ) name = FmtData.u.os2.res_module_name;
    else name = RemovePath( Root->outfile->fname, &namelen );
    namelen = strlen( name ) + 1;
    dir.address_table_rva = (dir.name_rva + namelen + 3) & ~3UL;
    num_entries = 0;
    for( exp = FmtData.u.os2.exports; exp != 0; exp = exp->next ) {
        high_ord = exp->ordinal;
        ++num_entries;
    }
    dir.num_eat_entries = high_ord - dir.ordinal_base + 1;
    dir.num_name_ptrs = num_entries;
    dir.name_ptr_table_rva = dir.address_table_rva + dir.num_eat_entries * 4;
    dir.ordinal_table_rva = dir.name_ptr_table_rva + num_entries * 4;
    sort = ChkLAlloc( num_entries * sizeof( *sort ) );
    WriteLoad( &dir, sizeof( dir ) );
    WriteLoad( name, namelen );
    NullAlign( 4 );
    i = 0;
    next_ord = dir.ordinal_base;
    for( exp = FmtData.u.os2.exports; exp != 0; exp = exp->next ) {
        sort[i++] = exp;
        eat = exp->addr.off;
        if( next_ord < exp->ordinal ) PadLoad( (exp->ordinal - next_ord) * 4UL );
        next_ord = exp->ordinal + 1;
        WriteLoad( &eat, sizeof( eat ) );
    }
    qsort( sort, num_entries, sizeof( *sort ), namecmp );
    eat = dir.ordinal_table_rva + num_entries * 2;
    for( i = 0; i < num_entries; ++i ) {
        WriteLoad( &eat, sizeof( eat ) );
        eat += strlen( sort[i]->name ) + 1;
    }
    for( i = 0; i < num_entries; ++i ) {
        ord = sort[i]->ordinal - dir.ordinal_base;
        WriteLoad( &ord, sizeof( ord ) );
    }
    for( i = 0; i < num_entries; ++i ) {
        name = sort[i]->name;
        WriteLoad( name, strlen( name ) + 1 );
    }
    _LnkFree( sort );
    size = eat - object->rva;
    object->physical_size = (size + header->file_align - 1) & ~(header->file_align - 1);
    header->table[PE_TBL_EXPORT].size = size;
    header->table[PE_TBL_EXPORT].rva = object->rva;
    header->image_size += (size + header->object_align - 1) & ~(header->object_align - 1);
}

extern unsigned long WriteRelocList( void **, unsigned long, unsigned long, unsigned long );

void WriteFixupInfo( pe_header *header, pe_object *object )
{
    unsigned long pages, high, page_rva, size;
    group_entry *group;
    void ***reloclist;

    strncpy( (char *)object->name, ".reloc", 8 );
    object->physical_offset = NullAlign( header->file_align );
    object->rva = header->image_size;
    object->flags = 0x42000040UL;
    size = 0;
    for( group = Groups; group != 0; group = group->next_group ) {
        reloclist = group->grp_relocs;
        if( reloclist != 0 ) {
            pages = (group->size + 4095) >> 12;
            page_rva = group->linear;
            high = pages >> 10;
            while( high > 0 ) {
                size = WriteRelocList( *reloclist, size, page_rva, 1024 );
                --high;
                ++reloclist;
                page_rva += 0x400000UL;
            }
            size = WriteRelocList( *reloclist, size, page_rva, pages & 1023 );
        }
    }
    object->physical_size = (size + header->file_align - 1) & ~(header->file_align - 1);
    header->table[PE_TBL_FIXUP].size = size;
    header->table[PE_TBL_FIXUP].rva = object->rva;
    header->image_size += (size + header->object_align - 1) & ~(header->object_align - 1);
}

extern unsigned long RelocSize( reloc_info * );
extern unsigned char DumpRelocList( reloc_info * );

unsigned long WriteRelocList( void **reloclist, unsigned long size,
                              unsigned long page_rva, unsigned long count )
{
    unsigned long page_size;
    unsigned char pad;

    while( count > 0 ) {
        page_size = RelocSize( *reloclist );
        if( page_size != 0 ) {
            pad = 0;
            if( (page_size / 2) & 1 ) {
                page_size += 2;
                pad = 1;
            }
            page_size += 8;
            QWrite( CurrSect->outfile->handle, &page_rva, 4, CurrSect->outfile->fname );
            QWrite( CurrSect->outfile->handle, &page_size, 4, CurrSect->outfile->fname );
            DumpRelocList( *reloclist );
            if( pad ) PadLoad( 2 );
            size += page_size;
        }
        ++reloclist;
        page_rva += 4096;
        --count;
    }
    return size;
}

int namecmp( const void *a, const void *b )
{
    entry_export *left, *right;

    left = *(entry_export * const *)a;
    right = *(entry_export * const *)b;
    return strcmp( left->name, right->name );
}

void WriteDescription( pe_header *header, pe_object *object )
{
    unsigned long length;

    length = strlen( FmtData.u.os2.description );
    strncpy( (char *)object->name, ".desc", 8 );
    object->physical_offset = NullAlign( header->file_align );
    object->rva = header->image_size;
    object->flags = 0x40000040UL;
    object->physical_size = (length + header->file_align - 1) & ~(header->file_align - 1);
    WriteLoad( FmtData.u.os2.description, (unsigned short)length );
    header->table[PE_TBL_DESCRIPTION].size = length;
    header->table[PE_TBL_DESCRIPTION].rva = object->rva;
    header->image_size += (length + header->object_align - 1) & ~(header->object_align - 1);
}

extern unsigned long CVSize;
#pragma pack(1)
typedef struct cvdebugdirectory {
    unsigned long flags, timestamp;
    unsigned short major, minor;
    unsigned long type, size, rva, seek;
} cvdebugdirectory;
#pragma pack()
void WriteDebugTable( pe_header *header, pe_object *object )
{
    cvdebugdirectory dir;

    strncpy( (char *)object->name, ".rdata", 8 );
    object->physical_offset = NullAlign( header->file_align );
    object->rva = header->image_size;
    object->flags = 0x40000040UL;
    object->physical_size = (sizeof(dir) + header->file_align - 1) & ~(header->file_align - 1);
    dir.flags = 0;
    dir.timestamp = 0;
    dir.major = 4;
    dir.minor = 0;
    dir.type = 2;
    dir.size = CVSize;
    dir.rva = object->rva;
    dir.seek = object->physical_offset + object->physical_size;
    WriteLoad( &dir, sizeof(dir) );
    header->table[PE_TBL_DEBUG].size = sizeof(dir);
    header->table[PE_TBL_DEBUG].rva = object->rva;
    header->image_size += (sizeof(dir) + header->object_align - 1) & ~(header->object_align - 1);
}
