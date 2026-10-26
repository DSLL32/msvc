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
#include "diagnostic.h"
#include "exene.h"
#include <recon.h>
#ifdef __WATCOMC__
#pragma intrinsic(strlen)
#endif

void ChkOS2Data( void )
{
    SetSegFlags( FmtData.u.os2.os2_seg_flags );
    FmtData.u.os2.os2_seg_flags = 0;
}

#pragma pack(1)
typedef struct dos_exe_header {
    unsigned short signature, mod_size, file_size, num_relocs, hdr_size;
    unsigned short min_16, max_16, ss, sp, checksum, ip, cs;
    unsigned short reloc_offset, overlay;
} dos_exe_header;
#pragma pack()

unsigned long GetStubSize( void )
{
    unsigned long stub_len, read_len, code_start, reloc_size;
    dos_exe_header dosheader;
    int file;
    char *name;

    name = FmtData.u.os2.stub_file_name;
    stub_len = (78 + DoExeName() + 15) & ~15UL;
    if( name != 0 && stricmp( name, Root->outfile->fname ) != 0 ) {
        file = SearchPath( name );
        if( file != -1 ) {
            QRead( file, &dosheader, sizeof(dosheader), name );
            if( dosheader.signature == 0x5a4d ) {
                if( dosheader.mod_size == 0 ) read_len = 512;
                else read_len = dosheader.mod_size;
                code_start = dosheader.hdr_size * 16UL;
                read_len += (dosheader.file_size - 1) * 512UL - code_start;
                reloc_size = (dosheader.num_relocs * 4UL + 15) & ~15UL;
                dosheader.hdr_size = 4 + reloc_size / 16;
                stub_len = read_len + dosheader.hdr_size * 16UL;
                /* The old instruction ANDs only the low16 bits. */
                stub_len += 7;
                stub_len = (stub_len & 0xffff0000UL)
                         | ((unsigned short)stub_len & 0xfff8);
            }
            QClose( file, name );
        }
    }
    return stub_len;
}

unsigned DoExeName( void )
{
    char rc_buff[128];
    unsigned msgsize;

    Msg_Get( 0x10e, rc_buff );
    msgsize = FmtStr( TokBuff, TokSize, rc_buff );
    TokBuff[msgsize++] = '\r';
    TokBuff[msgsize++] = '\n';
    TokBuff[msgsize] = '$';
    return msgsize + 1;
}

extern void SetGroupFlags( void );
extern void AssignOrdinals( void );

void ChkOS2Exports( void )
{
    entry_export *exp;
    symbol *sym;
    seg_leader *leader;
    group_entry *group;

    SetGroupFlags();
    for( exp = FmtData.u.os2.exports; exp != 0; exp = exp->next ) {
        sym = exp->sym;
        if( !(sym->info & 0x1000) ) LnkMsg( 0x332c, "s", sym->name );
        else {
            exp->addr.off = sym->addr.off;
            exp->addr.seg = sym->addr.seg;
            sym->info |= 0x2000;
            leader = sym->p.seg;
            if( leader == 0 ) LnkMsg( 0x337f, "S", sym );
            else {
                group = leader->group;
                if( FmtData.type & 6 ) exp->addr.off -= group->grp_addr.off;
                else if( FmtData.type & 0x10 ) {
                    exp->addr.off += group->linear - group->grp_addr.off;
                }
                if( group->segflags & 0x10 ) exp->flags |= 4;
            }
        }
    }
    AssignOrdinals();
}

void SetGroupFlags( void )
{
    group_entry *group;
    unsigned short flags;
    int i;

    for( group = Groups; group != 0; group = group->next_group ) {
        group->segflags |= 0x12b6;
        for( i = 0; i < group->num; ++i ) {
            flags = group->leaders[i]->u.p1.segflags;
            group->segflags |= flags & 0x8040;
            group->segflags &= (flags & 0x12b6) | 0xed49;
            if( (flags & 0x0c00) == 0x0800 ) {
                group->segflags = (group->segflags & ~0x0c10) | 0x0810;
            }
        }
    }
}

typedef char dos_header_is_28[(sizeof(dos_exe_header) == 28) ? 1 : -1];
extern unsigned long WriteDefStub( void );
extern void WriteLoad( void *, unsigned short );
extern void PadLoad( unsigned long );
extern unsigned long NullAlign( unsigned long );

unsigned long Write_Stub_File( void )
{
    unsigned long stub_len, read_len, reloc_size, reloc, code_start;
    unsigned amount;
    unsigned short num_relocs;
    int file;
    dos_exe_header header;
    char *name;

    name = FmtData.u.os2.stub_file_name;
    if( name == 0 ) {
        stub_len = WriteDefStub();
    } else if( stricmp( name, Root->outfile->fname ) == 0 ) {
        LnkMsg( 0x3333, 0 );
        stub_len = WriteDefStub();
    } else {
        file = SearchPath( name );
        if( file == -1 ) {
            LnkMsg( 0x325d, "s", name );
            return WriteDefStub();
        }
        QRead( file, &header, sizeof( header ), name );
        if( header.signature != 0x5a4d ) {
            LnkMsg( 0x3331, 0 );
            stub_len = WriteDefStub();
        } else {
            read_len = header.mod_size ? header.mod_size : 512;
            QSeek( file, header.reloc_offset, name );
            code_start = (unsigned long)header.hdr_size * 16;
            read_len += ((unsigned long)header.file_size - 1) * 512 - code_start;
            reloc_size = ((unsigned long)header.num_relocs * 4 + 15) & ~15UL;
            header.hdr_size = 4 + (reloc_size >> 4);
            stub_len = read_len + (unsigned long)header.hdr_size * 16;
            header.file_size = (stub_len + 511) >> 9;
            header.mod_size = stub_len & 511;
            header.reloc_offset = 0x40;
            WriteLoad( &header, sizeof( header ) );
            PadLoad( 0x3c - sizeof( header ) );
            stub_len = (stub_len + 7) & ~7UL;
            WriteLoad( &stub_len, sizeof( stub_len ) );
            for( num_relocs = header.num_relocs; num_relocs > 0; --num_relocs ) {
                QRead( file, &reloc, sizeof( reloc ), name );
                WriteLoad( &reloc, sizeof( reloc ) );
                reloc_size -= sizeof( reloc );
            }
            if( reloc_size != 0 ) PadLoad( reloc_size );
            QSeek( file, code_start, name );
            while( read_len > 0 ) {
                amount = read_len < TokSize ? read_len : TokSize;
                QRead( file, TokBuff, amount, name );
                WriteLoad( TokBuff, amount );
                read_len -= amount;
            }
        }
        QClose( file, name );
        _LnkFree( name );
        FmtData.u.os2.stub_file_name = 0;
        stub_len = NullAlign( 8 );
    }
    return stub_len;
}

#pragma pack(1)
typedef struct dos_stub {
    dos_exe_header header;
    unsigned char reserved[32];
    unsigned long new_header;
    unsigned char program[14];
} dos_stub;
#pragma pack()
typedef char dos_stub_is_78[(sizeof(dos_stub) == 78) ? 1 : -1];

unsigned long WriteDefStub( void )
{
    unsigned length;
    unsigned long stub_len;
    /* This is the emitted DOS compatibility program, not code executed by
     * the reconstruction. Its message follows the14-byte print/exit code. */
    static dos_stub stub = {
        { 0x5a4d, 0x80, 1, 0, 4, 0, 0xffff, 0, 0xb8, 0, 0, 0, 0x40, 0 },
        { 0 }, 0x80,
        { 0x0e, 0x1f, 0xba, 0x0e, 0x00, 0xb4, 0x09, 0xcd,
          0x21, 0xb8, 0x01, 0x4c, 0xcd, 0x21 }
    };

    length = DoExeName();
    stub_len = (length + sizeof( dos_stub ) + 15) & ~15UL;
    stub.new_header = stub_len;
    WriteLoad( &stub, sizeof( stub ) );
    WriteLoad( TokBuff, length );
    PadLoad( stub_len - length - sizeof( stub ) );
    return stub_len;
}

extern char *RemovePath( char *, int * );
unsigned long ResNonResNameTable( unsigned char resident )
{
    entry_export *exp;
    int length;
    unsigned long size;
    char *name;

    size = 0;
    if( resident ) {
        if( FmtData.u.os2.res_module_name != 0 ) {
            name = FmtData.u.os2.res_module_name;
            length = strlen( name );
        } else name = RemovePath( Root->outfile->fname, &length );
    } else {
        if( FmtData.u.os2.description != 0 ) name = FmtData.u.os2.description;
        else if( FmtData.type & 9 ) name = Root->outfile->fname;
        else name = "";
        length = strlen( name );
    }
    if( resident || length > 0 ) {
        WriteLoad( &length, 1 );
        WriteLoad( name, (unsigned short)length );
        PadLoad( 2 );
        size += length + 3;
    }
    if( resident && FmtData.u.os2.res_module_name != 0 ) {
        _LnkFree( name );
        FmtData.u.os2.res_module_name = 0;
    }
    if( !resident && FmtData.u.os2.description != 0 ) {
        _LnkFree( name );
        FmtData.u.os2.description = 0;
    }
    for( exp = FmtData.u.os2.exports; exp != 0; exp = exp->next ) {
        if( !(exp->flags & 2) || (exp->flags & 8) ) continue;
        if( (resident && (exp->flags & 1)) || (!resident && !(exp->flags & 1)) ) {
            if( !(LinkFlags & 0x2000) ) strupr( exp->name );
            length = strlen( exp->name );
            WriteLoad( &length, 1 );
            WriteLoad( exp->name, (unsigned short)length );
            WriteLoad( &exp->ordinal, 2 );
            size += length + 3;
        }
    }
    if( size > 0 ) {
        PadLoad( 1 );
        ++size;
    }
    return size;
}

unsigned short FindEntryOrdinal( address addr, group_entry *group )
{
    unsigned short maxord;
    entry_export **owner, *exp;

    maxord = 0;
    owner = &FmtData.u.os2.exports;
    for( ;; ) {
        exp = *owner;
        if( exp == NULL ) break;
        if( addr.seg == exp->addr.seg && addr.off == exp->addr.off ) {
            return exp->ordinal;
        }
        if( exp->ordinal >= maxord ) maxord = exp->ordinal;
        owner = &exp->next;
    }
    exp = ChkLAlloc( sizeof(*exp) );
    exp->iopl_words = 0;
    exp->name = NULL;
    exp->sym = NULL;
    exp->ordinal = maxord + 1;
    /* Only the four low export flags are defined by the old allocator. */
    exp->flags = (group->segflags & 0x10) ? 4 : 0;
    exp->addr.off = addr.off;
    exp->addr.seg = addr.seg;
    exp->next = NULL;
    *owner = exp;
    return exp->ordinal;
}

extern unsigned long ModRefTable( void );
extern unsigned long ImportNameTable( void );
extern unsigned long DumpEntryTable( void );
extern void WriteOS2Data( unsigned long, os2_exe_header * );
extern seg_leader *StackSegPtr;
extern address StartInfo;
void FiniOS2LoadFile( void )
{
    os2_exe_header header;
    unsigned long stub_len, temp, size, dgroup_size, imageguess;
    unsigned short adseg;
    unsigned padding;
    group_entry *group;
    entry_export *exp;

    CurrSect = Root;
    QSeek( Root->outfile->handle, 0, Root->outfile->fname );
    stub_len = Write_Stub_File();
    header.segment_off = sizeof(header);
    QSeek( Root->outfile->handle, stub_len + sizeof(header), Root->outfile->fname );
    adseg = 0;
    header.segments = 0;
    imageguess = dgroup_size = 0;
    if( DataGroup != NULL ) {
        adseg = DataGroup->grp_addr.seg;
        if( DataGroup->segflags & 0x20 ) FmtData.u.os2.flags |= 0x1000;
        if( StackSegPtr != NULL ) {
            if( DataGroup->totalsize - DataGroup->size < StackSize ) {
                StackSize = DataGroup->totalsize - DataGroup->size;
                DataGroup->totalsize = DataGroup->size;
            } else {
                DataGroup->totalsize -= StackSize;
            }
        }
        dgroup_size = DataGroup->totalsize;
    }
    for( group = Groups; group != NULL; group = group->next_group ) {
        ++header.segments;
        imageguess += group->size;
    }
    temp = sizeof(header) + header.segments * 8UL;
    header.resident_off = temp;
    QSeek( Root->outfile->handle, stub_len + temp, Root->outfile->fname );
    temp += ResNonResNameTable( 1 );
    header.module_off = temp;
    header.modrefs = ModRefTable();
    temp += header.modrefs * 2;
    header.import_off = temp;
    temp += ImportNameTable();
    header.entry_off = temp;
    size = DumpEntryTable();
    header.entry_size = size;
    temp += size + stub_len;
    header.nonres_off = temp;
    header.nonres_size = ResNonResNameTable( 0 );
    temp += header.nonres_size;
    header.resource_off = header.resident_off;
    header.resource = 0;
    if( FmtData.u.os2.segment_shift == 0 ) {
        imageguess += temp + Root->relocs * 8UL + stub_len + header.segments * 3UL;
        padding = binary_log( (unsigned short)((imageguess >> 16) * 2) );
        imageguess += ((1UL << padding) - 1) * header.segments;
        FmtData.u.os2.segment_shift = binary_log( (unsigned short)((imageguess >> 16) * 2) );
        if( FmtData.u.os2.segment_shift == 0 ) FmtData.u.os2.segment_shift = 1;
    }
    WriteOS2Data( stub_len, &header );
    header.signature = 0x454e;
    header.version = 0x0105;
    header.chk_sum = 0;
    header.info = 0;
    header.target = (FmtData.type & 8) ? 2 : 1;
    if( FmtData.u.os2.flags & 0x0200 ) header.info |= 8;
    if( FmtData.u.os2.flags & 1 ) header.info |= 1;
    else if( FmtData.u.os2.flags & 2 ) header.info |= 2;
    else adseg = 0;
    if( FmtData.u.os2.flags & 0x4000 ) header.info |= 0x0100;
    else if( FmtData.u.os2.flags & 0x20 ) header.info |= 0x0300;
    else header.info |= 0x0200;
    if( FmtData.u.os2.flags & 0x2000 ) header.info |= 0x4000;
    if( FmtData.flags & 1 ) {
        header.info |= 0x8000;
        if( FmtData.u.os2.flags & 4 ) header.info |= 4;
    }
    if( LinkState & 8 ) header.info |= 0x2000;
    header.adsegnum = adseg;
    header.heap = FmtData.u.os2.heapsize;
    header.SP = 0;
    if( FmtData.u.os2.flags & 8 ) header.stacknum = adseg;
    else header.stacknum = StackAddr.seg != 0xffff ? StackAddr.seg : 0;
    header.stack = StackSize;
    if( dgroup_size + header.heap > 65536UL ) {
        LnkMsg( 0x3276, 0 );
        if( dgroup_size < 65536UL ) header.heap = 65536UL - dgroup_size;
    }
    header.IP = StartInfo.off;
    header.entrynum = StartMod != NULL ? StartInfo.seg : 0;
    header.align = FmtData.u.os2.segment_shift;
    header.movable = 0;
    for( exp = FmtData.u.os2.exports; exp != NULL; exp = exp->next ) {
        if( exp->flags & 4 ) ++header.movable;
    }
    header.otherflags = (FmtData.u.os2.flags & 0x40) != 0;
    if( FmtData.u.os2.flags & 0x0100 ) header.otherflags |= 6;
    else if( FmtData.u.os2.flags & 0x80 ) header.otherflags |= 4;
    header.gangstart = header.ganglength = header.swaparea = 0;
    if( FmtData.type & 8 ) {
        if( FmtData.flags & 2 ) header.expver = (FmtData.major << 8) | (FmtData.minor & 255);
        else header.expver = 0x0300;
    } else header.expver = 0;
    QSeek( Root->outfile->handle, stub_len, Root->outfile->fname );
    WriteLoad( &header, sizeof(header) );
}

extern unsigned long ImportProcTable( unsigned long * );
extern unsigned long ImportModTable( unsigned long * );
unsigned long ImportNameTable( void )
{
    unsigned long count, size;

    PadLoad( 1 );
    size = 1 + ImportProcTable( &count );
    size += ImportModTable( &count );
    return size;
}

#pragma pack(1)
typedef struct bundle_prefix {
    unsigned char number, type;
} bundle_prefix;
typedef union bundle_item {
    struct { unsigned char info; unsigned short entry; } fixed;
    struct {
        unsigned char info;
        unsigned short reserved;
        unsigned char entrynum;
        unsigned short entry;
    } movable;
} bundle_item;
#pragma pack()
unsigned long DumpEntryTable( void )
{
    entry_export *start, *place, *prev;
    unsigned short prevord;
    unsigned long size;
    unsigned gap, entries;
    bundle_prefix prefix;
    bundle_item item;

    size = 0;
    prevord = 0;
    place = FmtData.u.os2.exports;
    while( place != NULL ) {
        gap = place->ordinal - prevord;
        if( gap > 1 ) {
            --gap;
            prefix.number = 255;
            prefix.type = 0;
            while( gap > 255 ) {
                WriteLoad( &prefix, sizeof(prefix) );
                gap -= 255;
                size += 2;
            }
            prefix.number = gap;
            WriteLoad( &prefix, sizeof(prefix) );
            size += 2;
        }
        entries = 1;
        prev = start = place;
        place = place->next;
        while( place != NULL ) {
            if( entries >= 255 ) break;
            if( start->flags & 4 ) {
                if( !(place->flags & 4) ) break;
            } else {
                if( place->addr.seg != start->addr.seg ) break;
            }
            if( (int)place->ordinal - (int)prev->ordinal > 1 ) break;
            ++entries;
            prev = place;
            place = place->next;
        }
        if( start->flags & 4 ) {
            prefix.type = 255;
            item.movable.reserved = 0x3fcd;
            size += entries * sizeof(item.movable);
        } else {
            prefix.type = start->addr.seg;
            size += entries * sizeof(item.fixed);
        }
        prevord = prev->ordinal;
        prefix.number = entries;
        WriteLoad( &prefix, sizeof(prefix) );
        size += 2;
        for( ; entries > 0; --entries, start = start->next ) {
            item.fixed.info = start->iopl_words << 2;
            if( start->flags & 2 ) {
                item.fixed.info |= 1;
                if( FmtData.u.os2.flags & 0x1000 ) item.fixed.info |= 2;
            }
            if( start->flags & 4 ) {
                if( start->addr.seg > 255 ) LnkMsg( 0x3275, 0 );
                item.movable.entrynum = start->addr.seg;
                item.movable.entry = start->addr.off;
                WriteLoad( &item, sizeof(item.movable) );
            } else {
                item.fixed.entry = start->addr.off;
                WriteLoad( &item, sizeof(item.fixed) );
            }
        }
    }
    PadLoad( 2 );
    return size + 2;
}

extern unsigned char WriteOS2Relocs( group_entry * );
extern void WriteDBI( void );
#pragma pack(1)
typedef struct segment_record {
    unsigned short address, size, info, min;
} segment_record;
#pragma pack()
void WriteOS2Data( unsigned long stub_len, os2_exe_header *header )
{
    group_entry *group;
    unsigned table_off;
    unsigned long off, seg_addr;
    segment_record record;

    table_off = 0;
    for( group = Groups; group != NULL; group = group->next_group ) {
        record.min = (group->totalsize + 1) & ~1UL;
        record.info = group->segflags;
        record.size = (group->size + 1) & ~1UL;
        if( record.size != 0 ) {
            off = NullAlign( 1UL << FmtData.u.os2.segment_shift );
            seg_addr = off >> FmtData.u.os2.segment_shift;
            if( seg_addr > 0xffffUL ) LnkMsg( 0x3327, 0 );
            record.address = seg_addr;
            QWrite( CurrSect->outfile->handle, (void *)group->data,
                    group->size, CurrSect->outfile->fname );
            if( group->size & 1 ) PadLoad( 1 );
            if( WriteOS2Relocs( group ) ) record.info |= 0x0100;
        } else {
            record.address = 0;
        }
        off = QPos( Root->outfile->handle );
        QSeek( Root->outfile->handle, header->segment_off + stub_len + table_off,
                Root->outfile->fname );
        WriteLoad( &record, sizeof(record) );
        QSeek( Root->outfile->handle, off, Root->outfile->fname );
        table_off += sizeof(record);
    }
    WriteDBI();
}

extern unsigned long RelocSize( reloc_info * );
extern unsigned char DumpRelocList( reloc_info * );
unsigned char WriteOS2Relocs( group_entry *group )
{
    unsigned long count;

    count = RelocSize( group->grp_relocs ) >> 3;
    if( (unsigned short)count == 0 ) return 0;
    WriteLoad( &count, 2 );
    DumpRelocList( group->grp_relocs );
    return 1;
}
