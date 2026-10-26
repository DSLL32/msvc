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
#include <recon.h>

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
