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
#include "cmdline.h"
#include "exenov.h"
#include <recon.h>

#ifdef __WATCOMC__
#pragma intrinsic(strlen,memcpy,memcmp)
#endif
static unsigned long DbgInfoCount;      /* original441ff4 */
static unsigned long DbgInfoLen;        /* original441ff8 */
static unsigned char *NovDbgInfo;       /* original4502d4 */
static unsigned char *CurrDbgLoc;       /* original4502d0 */

void NovDBIAddGlobal( unsigned len )
{
    if( !(FmtData.u.nov.flags & 0x80) ) DbgInfoLen += len + 6;
}

void FindExportedSyms( void )
{
    snamelist *export;
    symbol *sym;
    debug_info *dinfo;

    dinfo = CurrSect->dbg_info;
    if( (FmtData.u.nov.flags & 0x40) && dinfo != 0 ) {
        for( export = FmtData.u.nov.exports; export != 0; export = export->next ) {
            sym = SymOp( 8, export->name, export->len );
            if( sym != 0 && (sym->info & 15) != 6 ) {
                dinfo->global.curr += strlen( sym->name ) + 10;
                sym->info |= 0x2000;
            }
        }
    }
}

void NovDBIAddrStart( void )
{
    if( DbgInfoLen != 0 ) {
        NovDbgInfo = (unsigned char *)AllocStg( DbgInfoLen );
        CurrDbgLoc = NovDbgInfo;
    }
}

#pragma pack(1)
typedef struct nov_dbg_info {
    unsigned char type;
    unsigned long offset;
    unsigned char namelen;
} nov_dbg_info;
#pragma pack()
typedef char nov_debug_record_is_6[(sizeof(nov_dbg_info) == 6) ? 1 : -1];

void NovDBIGenGlobal( symbol *sym )
{
    nov_dbg_info info;
    if( DbgInfoLen != 0 && (!(FmtData.u.nov.flags & 0x20) ||
                                                    (sym->info & 0x40)) ) {
        DbgInfoCount++;
        info.type = sym->addr.seg != 2;
        info.offset = sym->addr.off;
        info.namelen = strlen( sym->name );
        memcpy( CurrDbgLoc, &info, sizeof(info) );
        CurrDbgLoc += sizeof(info);
        memcpy( CurrDbgLoc, sym->name, info.namelen );
        CurrDbgLoc += info.namelen;
    }
}

#pragma pack(1)
typedef struct nov_import {
    unsigned char contents;
    unsigned long num_relocs;
    unsigned long addr[1];
} nov_import;
#pragma pack()
typedef char nov_import_is_9[(sizeof(nov_import) == 9) ? 1 : -1];

void AddNovImpReloc( symbol *sym, unsigned long offset,
                               unsigned char isrelative, unsigned char isdata )
{
    nov_import *imp, *new;
    unsigned long mem;
    unsigned block, off;

    if( !isrelative ) offset |= 0x80000000UL;
    if( !isdata ) offset |= 0x40000000UL;
    imp = (nov_import *)sym->u.import;
    if( imp == (nov_import *)&FmtData.u.nov ) {
        imp = ChkLAlloc( sizeof(nov_import) );
        sym->u.import = (dll_sym_info *)imp;
        imp->contents = 1;
        imp->num_relocs = offset;
    } else if( imp->contents < 10 ) {
        if( imp->contents == 2 ) {
            new = ChkLAlloc( 41 );
            memcpy( new, imp, sizeof(nov_import) );
            _LnkFree( imp );
            imp = new;
            sym->u.import = (dll_sym_info *)imp;
        }
        *(&imp->num_relocs + imp->contents) = offset;
        imp->contents++;
    } else if( imp->contents == 10 ) {
        mem = AllocStg( 512 );
        memcpy( (void *)mem, &imp->num_relocs, 40 );
        memcpy( (void *)(mem + 40), &offset, 4 );
        imp->contents++;
        imp->num_relocs = imp->contents;
        imp->addr[0] = mem;
    } else {
        block = imp->num_relocs / 128;
        off = imp->num_relocs % 128;
        if( off == 0 ) {
            if( block >= (imp->contents - 10) * 9 ) {
                new = ChkLAlloc( 5 + block * 8 );
                memcpy( new, imp, 5 + block * 4 );
                _LnkFree( imp );
                imp = new;
                imp->contents++;
                sym->u.import = (dll_sym_info *)imp;
            }
            imp->addr[block] = AllocStg( 512 );
        }
        memcpy( (void *)(imp->addr[block] + off * 4), &offset, 4 );
        imp->num_relocs++;
    }
}

extern void WriteLoad( void *, unsigned );
extern void PadLoad( unsigned long );
extern unsigned long MemorySize( void );
extern void WriteDBI( void );

void FiniNovellLoadFile( void )
/************************************/
{
    unsigned long         file_size;
    fixed_header        nov_header;
    fixed_hdr_2         second_header;
    fixed_hdr_3         third_header;
    extended_nlm_header ext_header;
    unsigned long         temp;
    unsigned long         image_size;
    char *              filename;
    char *              lastslash;
    char                ch;
    unsigned char          len;
    struct tm *         currtime;
    time_t              thetime;

// find module name (output file name without the path.

    lastslash = filename = Root->outfile->fname;
    while( *filename != '\0' ) {
        ch = *filename++;
        if( (ch == '\\' || ch == '/' || ch == ':') ) {
            lastslash = filename;     // NOTE: 1 added already.
        }
    }
    strupr( lastslash );
    len = strlen( lastslash );       // length of module name;

    file_size = strlen( FmtData.u.nov.description ) + sizeof(fixed_header)
                + sizeof(extended_nlm_header) + 2*sizeof(unsigned long)
                + 12*sizeof(unsigned char);
    if( FmtData.u.nov.screenname != NULL ) {
        file_size += strlen( FmtData.u.nov.screenname );
    }
    if( FmtData.u.nov.threadname != NULL ) {
        file_size += strlen( FmtData.u.nov.threadname );
    } else {
        file_size += len;
    }
    if( FmtData.major != 0 || FmtData.minor != 0 ) {
        file_size += sizeof( fixed_hdr_2 );
    }
    if( FmtData.u.nov.copyright != NULL ) {
        file_size += sizeof(fixed_hdr_3) + strlen( FmtData.u.nov.copyright );
    }
    QSeek( Root->outfile->handle, file_size, Root->outfile->fname );
    nov_header.codeImageOffset = file_size;
    image_size = WriteNovData( file_size, &nov_header );
    temp = MemorySize() - image_size;
    if( temp > 0 ) {       // write out BSS.
        PadLoad( temp );
        nov_header.dataImageSize += temp;
        image_size += temp;
    }
    file_size += image_size;
    nov_header.relocationFixupOffset = file_size;
    file_size += WriteNovRelocs( &nov_header );
    nov_header.externalReferencesOffset = file_size;
    file_size += WriteNovImports( &nov_header );
    nov_header.publicsOffset = file_size;
    file_size += WriteNovExports( &nov_header );
    nov_header.moduleDependencyOffset = file_size;
    file_size += WriteNovModules( &nov_header );
    nov_header.customDataOffset = file_size;
    temp = AppendToLoadFile( FmtData.u.nov.customdata );
    nov_header.customDataSize = temp;
    /* The old writer records the message offset before adding custom bytes. */
    memset( &ext_header, 0, sizeof(ext_header) );
    memcpy( ext_header.stamp, EXTENDED_NLM_SIGNATURE,
                              EXTENDED_NLM_SIGNATURE_LENGTH );
    ext_header.messageFileOffset = file_size;
    file_size += WriteMessages( &ext_header ) + temp;
    ext_header.helpFileOffset = file_size;
    temp = AppendToLoadFile( FmtData.u.nov.help );
    ext_header.helpFileLength = temp;
    file_size += temp;
    ext_header.RPCDataOffset = file_size;
    temp = AppendToLoadFile( FmtData.u.nov.rpcdata );
    ext_header.RPCDataLength = temp;
    file_size += temp;
    file_size += WriteSharedNLM( &ext_header, file_size );
    nov_header.debugInfoOffset = file_size;
    file_size += WriteNovDBI( &nov_header );
    WriteDBI();
    memcpy( nov_header.signature, NLM_SIGNATURE, sizeof( NLM_SIGNATURE ) );
    nov_header.version = NLM_VERSION;
    nov_header.moduleName[0] = len;
    memcpy( &nov_header.moduleName[1], lastslash, len );
    memset( &nov_header.moduleName[ len + 1 ], 0, 13-len ); // zero rest.
    nov_header.uninitializedDataSize = 0; // MemorySize() - image_size;
    GetProcOffsets( &nov_header );
    nov_header.moduleType = FmtData.u.nov.moduletype;
    nov_header.flags = (FmtData.u.nov.flags & 0x1f);
    nov_header.descriptionLength = strlen( FmtData.u.nov.description );
    QSeek( Root->outfile->handle, 0L, Root->outfile->fname );
    WriteLoad( &nov_header, sizeof( nov_header ) );
    WriteLoad( FmtData.u.nov.description, nov_header.descriptionLength + 1 );
    WriteLoad( &StackSize, sizeof( unsigned long ) );
    temp = 0;
    WriteLoad( &temp, sizeof( unsigned long ) ); // reserved.
    WriteLoad( " LONG", OLD_THREAD_NAME_LENGTH );
    NovNameWrite( FmtData.u.nov.screenname );
    if( FmtData.u.nov.threadname != NULL ) {
        NovNameWrite( FmtData.u.nov.threadname );
    } else {
        NovNameWrite( lastslash );      // use module name as a default
    }
    if( FmtData.major != 0 || FmtData.minor != 0 ) {
        memcpy( second_header.versionSignature, VERSION_SIGNATURE,
                                            VERSION_SIGNATURE_LENGTH);
        second_header.majorVersion = FmtData.major;
        second_header.minorVersion = FmtData.minor;
        second_header.revision = FmtData.revision;
        thetime = time( NULL );
        currtime = localtime( &thetime );
        second_header.year = currtime->tm_year + 1900;
        second_header.month = currtime->tm_mon + 1;
        second_header.day = currtime->tm_mday;
        WriteLoad( &second_header, sizeof( second_header ) );
    }

    if( FmtData.u.nov.copyright != NULL ) {
        memcpy( third_header.copyrightSignature, COPYRIGHT_SIGNATURE,
                                              COPYRIGHT_SIGNATURE_LENGTH);
        WriteLoad( &third_header, sizeof( third_header ) );
        NovNameWrite( FmtData.u.nov.copyright );
    }
    WriteLoad( &ext_header, sizeof(ext_header) );
}


unsigned long WriteNovData( unsigned long pos, fixed_header *header )
{
    unsigned long codesize;
    OrderGroups( CompareProtSegments );
    CurrSect = Root;
    Root->outfile->file_loc = pos;
    Root->u.file_loc = pos;
    Root->sect_addr.off = Groups->grp_addr.off;
    Root->sect_addr.seg = 1;
    codesize = WriteNovImage( pos, 1 );
    header->codeImageSize = codesize;
    pos += codesize;
    header->dataImageOffset = pos;
    Root->outfile->file_loc = pos;
    Root->u.file_loc = pos;
    Root->sect_addr.off = Groups->grp_addr.off;
    Root->sect_addr.seg = 2;
    header->dataImageSize = WriteNovImage( pos, 0 );
    return codesize + header->dataImageSize;
}

extern unsigned char WriteDOSGroup( group_entry * );
unsigned long WriteNovImage( unsigned long pos, unsigned char docode )
{
    group_entry *group;
    outfilelist *file;
    unsigned char iscode;

    file = Root->outfile;
    for( group = Groups; group != 0; group = group->next_group ) {
        iscode = group->grp_addr.seg == 1;
        if( iscode == docode ) {
            if( WriteDOSGroup( group ) ) QSeek( file->handle, file->file_loc, file->fname );
        }
    }
    return file->file_loc - pos;
}

extern unsigned char DumpRelocList( reloc_info * );
unsigned long WriteNovRelocs( fixed_header *header )
{
    DumpRelocList( Root->reloclist );
    header->numberOfRelocationFixups = Root->relocs;
    return (unsigned long)Root->relocs * 4;
}

unsigned long WriteNovImports( fixed_header *header )
{
    nov_import *imp;
    symbol *sym;
    unsigned long count, wrote, refs;
    unsigned long *mem;
    unsigned char len;

    count = wrote = 0;
    for( sym = HeadSym; sym != 0; sym = sym->link ) {
        if( (sym->info & 15) != 6 ) continue;
        if( sym->u.import == (dll_sym_info *)&FmtData.u.nov ) sym->u.import = 0;
        imp = (nov_import *)sym->u.import;
        if( imp != 0 ) {
            count++;
            len = strlen( sym->name );
            WriteLoad( &len, 1 );
            WriteLoad( sym->name, len );
            wrote += len + 5;
            if( imp->contents <= 10 ) {
                refs = imp->contents;
                WriteLoad( &refs, 4 );
                refs *= 4;
                WriteLoad( &imp->num_relocs, refs );
            } else {
                refs = imp->num_relocs;
                WriteLoad( &refs, 4 );
                mem = imp->addr;
                while( refs > 128 ) {
                    QWrite( CurrSect->outfile->handle, (void *)*mem, 512,
                                                    CurrSect->outfile->fname );
                    mem++;
                    refs -= 128;
                }
                QWrite( CurrSect->outfile->handle, (void *)*mem, refs * 4,
                                                    CurrSect->outfile->fname );
                refs = imp->num_relocs * 4;
            }
            wrote += refs;
        }
    }
    header->numberOfExternalReferences = count;
    return wrote;
}

unsigned long WriteNovExports( fixed_header *header )
{
    snamelist *export;
    symbol *sym;
    unsigned long count, wrote, off;
    unsigned char len;

    count = wrote = 0;
    for( export = FmtData.u.nov.exports; export != 0; export = export->next ) {
        len = export->len;
        sym = SymOp( 8, export->name, len );
        if( sym == 0 ) LnkMsg( 0x322c, "s", export->name );
        else if( (sym->info & 15) != 6 ) {
            WriteLoad( &len, 1 );
            WriteLoad( export->name, len );
            count++;
            off = sym->addr.off;
            if( sym->addr.seg == 1 ) off |= 0x80000000UL;
            WriteLoad( &off, 4 );
            wrote += len + 5;
        }
    }
    header->numberOfPublics = count;
    return wrote;
}

unsigned long WriteNovModules( fixed_header *header )
{
    snamelist *module;
    unsigned long count, wrote;
    count = wrote = 0;
    for( module = FmtData.u.nov.modules; module != 0; module = module->next ) {
        WriteLoad( &module->len, 1 );
        WriteLoad( module->name, module->len );
        count++;
        wrote += module->len + 1;
    }
    header->numberOfModuleDependencies = count;
    return wrote;
}

unsigned long AppendToLoadFile( char *name )
{
    if( name != 0 ) return CopyToLoad( QOpenR( name ), name );
    return 0;
}

unsigned long WriteMessages( extended_nlm_header *header )
{
    int handle;
    char *name;
    unsigned long buf[2];

    name = FmtData.u.nov.messages;
    header->messageFileLength = 0;
    if( name != 0 ) {
        handle = QOpenR( name );
        QRead( handle, TokBuff, MSG_FILE_SIGNATURE_LENGTH, name );
        if( memcmp( TokBuff, MSG_FILE_SIGNATURE, MSG_FILE_SIGNATURE_LENGTH ) != 0 ) {
            LnkMsg( 0x3286, "s", name );
            QClose( handle, name );
        } else {
            QSeek( handle, LANGUAGE_ID_OFFSET, name );
            QRead( handle, buf, sizeof(buf), name );
            header->languageID = buf[0];
            header->messageCount = buf[1];
            QSeek( handle, 0, name );
            header->messageFileLength = CopyToLoad( handle, name );
        }
    }
    return header->messageFileLength;
}

unsigned long WriteSharedNLM( extended_nlm_header *header, unsigned long size )
{
    int handle;
    char *name;
    fixed_header *share;

    name = FmtData.u.nov.sharednlm;
    if( name != 0 ) {
        handle = QOpenR( name );
        QRead( handle, TokBuff, sizeof(fixed_header), name );
        if( memcmp( TokBuff, NLM_SIGNATURE, sizeof(NLM_SIGNATURE) - 1 ) != 0 ) {
            LnkMsg( 0x3282, "s", name );
            QClose( handle, name );
        } else {
            share = (fixed_header *)TokBuff;
            header->sharedCodeOffset = share->codeImageOffset + size;
            header->sharedCodeLength = share->codeImageSize;
            header->sharedDataOffset = share->dataImageOffset + size;
            header->sharedDataLength = share->dataImageSize;
            header->sharedRelocationFixupOffset = share->relocationFixupOffset + size;
            header->sharedRelocationFixupCount = share->numberOfRelocationFixups;
            header->sharedExternalReferenceOffset = share->externalReferencesOffset + size;
            header->sharedExternalReferenceCount = share->numberOfExternalReferences;
            header->sharedPublicsOffset = share->publicsOffset + size;
            header->sharedPublicsCount = share->numberOfPublics;
            header->sharedDebugRecordOffset = share->debugInfoOffset + size;
            header->sharedDebugRecordCount = share->numberOfDebugRecords;
            /* The old writer also rebases these two fields. */
            header->sharedInitializationOffset = share->codeStartOffset + size;
            header->sharedExitProcedureOffset = share->exitProcedureOffset + size;
            QSeek( handle, 0, name );
            return CopyToLoad( handle, name );
        }
    }
    return 0;
}

unsigned long WriteNovDBI( fixed_header *header )
{
    snamelist *export;
    symbol *sym;
    unsigned long count, wrote;
    unsigned char len;
    nov_dbg_info info;

    if( DbgInfoLen > 0 ) {
        QWrite( CurrSect->outfile->handle, NovDbgInfo, CurrDbgLoc - NovDbgInfo,
                                                   CurrSect->outfile->fname );
        header->numberOfDebugRecords = DbgInfoCount;
        return CurrDbgLoc - NovDbgInfo;
    }
    if( FmtData.u.nov.flags & 0x80 ) {
        count = wrote = 0;
        for( export = FmtData.u.nov.exports; export != 0; export = export->next ) {
            len = export->len;
            sym = SymOp( 8, export->name, len );
            if( sym != 0 && (sym->info & 15) != 6 ) {
                count++;
                info.type = sym->addr.seg != 2;
                info.offset = sym->addr.off;
                info.namelen = len;
                WriteLoad( &info, sizeof(info) );
                WriteLoad( export->name, len );
                wrote += sizeof(info) + len;
            }
        }
        header->numberOfDebugRecords = count;
        return wrote;
    }
    header->numberOfDebugRecords = 0;
    return 0;
}

extern symbol *FindISymbol( char * );
void GetProcOffsets( fixed_header *header )
{
    symbol *sym;
    char *name;

    header->checkUnloadProcedureOffset = 0;
    if( FmtData.u.nov.checkfn != 0 ) {
        sym = FindISymbol( FmtData.u.nov.checkfn );
        if( sym == 0 ) LnkMsg( 0x3245, 0 );
        else header->checkUnloadProcedureOffset = sym->addr.off;
    }
    name = FmtData.u.nov.startfn;
    if( name == 0 ) name = "_Prelude";
    sym = FindISymbol( name );
    if( sym == 0 ) LnkMsg( 0x3346, 0 );
    else header->codeStartOffset = sym->addr.off;
    name = FmtData.u.nov.exitfn;
    if( name == 0 ) name = "_Stop";
    sym = FindISymbol( name );
    if( sym == 0 ) LnkMsg( 0x3347, 0 );
    else header->exitProcedureOffset = sym->addr.off;
}

void NovNameWrite( char *name )
{
    unsigned char len;
    if( name != 0 ) len = strlen( name );
    else {
        len = 0;
        name = (char *)&len;
    }
    WriteLoad( &len, 1 );
    WriteLoad( name, len + 1 );
}
