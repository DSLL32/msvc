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

#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <direct.h>
#include <string.h>
#include "cmdline.h"
#include <recon.h>

#ifdef __WATCOMC__
void reconstruction_trap( void );
#pragma aux reconstruction_trap = 0xcc;
#pragma intrinsic(strlen,memcpy,strcpy)
#endif

/* The 1994 table has 10-byte rows and no later flags member. */
parse_entry Directives[] = {
    { "File", ProcFiles, 0xffff },
    { "Library", ProcLibrary, 0xffff },
    { "Name", ProcName, 0xffff },
    { "OPtion", ProcOptions, 0xffff },
    { "Debug", ProcDebug, 0xffff },
    { "SYStem", ProcSystem, 0xffff },
    { "LIBPath", ProcLibPath, 0xffff },
    { "LIBFile", ProcLibFile, 0xffff },
    { "Path", ProcPath, 0xffff },
    { "FORMat", ProcFormat, 0xffff },
    { "MODTrace", ProcModTrace, 0xffff },
    { "SYMTrace", ProcSymTrace, 0xffff },
    { "Alias", ProcAlias, 0xffff },
    { "REFerence", ProcReference, 0xffff },
    { "DISAble", ProcDisable, 0xffff },
    { "SOrt", ProcSort, 0xffff },
    { "LANGuage", ProcLanguage, 0xffff },
    { "COMmit", ProcCommit, 0x0010 },
    { "ANONymousexport", ProcAnonExport, 0x000f },
    { "IMPort", ProcImport, 0x811f },
    { "EXPort", ProcExport, 0x811f },
    { "SEGment", ProcSegment, 0x421f },
    { "OVerlay", ProcOverlay, 0x0080 },
    { "Begin", ProcBegin, 0x0080 },
    { "FIXedlib", ProcFixedLib, 0x0080 },
    { "NOVector", ProcNoVector, 0x0080 },
    { "VEctor", ProcVector, 0x0080 },
    { "FORCEVEctor", ProcForceVector, 0x0080 },
    { "RUntime", ProcRuntime, 0x3c10 },
    { "MODUle", ProcModule, 0x8100 },
    { "NEWsegment", ProcNewSegment, 0x42e9 },
    { 0, 0, 0 }
};

void DoCmdParse( void )
{
    while( !GetToken( 4, 1 ) ) {
        if( !ProcOne( Directives, 0, 1 ) ) Syntax();
        RestoreParser();
    }
}

unsigned char GetToken( unsigned req, unsigned ctrl )
/***************************************************/
/* return TRUE if no problem */
/* return FALSE if problem   */
{
    char    hmm;
    unsigned char ret;
    unsigned char need_sep;

    if( Token.thumb == REJECT ) {
        Token.thumb = OK;
        if( Token.quoted ) return( TRUE );
        Token.next = Token.this;        /* re-process last token */
    }
    need_sep = TRUE;
    for(;;) {                           /* finite state machine */
        switch( Token.where ) {
        case MIDST:
            EatWhite();
            hmm = *Token.next;
            switch( hmm ) {
            case CTRLZ:
                Token.where = ENDOFFILE;
                break;
            case '\0':
                if( Token.how == BUFFERED
                 || Token.how == ENVIRONMENT
                 || Token.how == SYSTEM ) {
                    Token.where = ENDOFFILE;
                    break;
                }                // NOTE the fall through.
            case '\n':
                if( Token.how == BUFFERED
                 || Token.how == ENVIRONMENT
                 || Token.how == SYSTEM ) {
                    Token.next++;               // just skip this.
                } else if( Token.how == COMMANDLINE ) {
                    Token.where = ENDOFCMD;
                } else {
                    Token.where = ENDOFLINE;
                }
                Token.line++;
                break;
            case '@':
                if( req != SEP_SPACE ) {
                    Token.next++;
                    EatWhite();
                    if( *Token.next == '%' ) ExpandEnvVariable();
                    MakeToken( TOK_INCLUDE_DOT, SEP_NO );
                    StartNewFile();
                    break;
                }
                Token.next--;   /* make a token out of this */
                ret = MakeToken( ctrl, req );
                Token.quoted = FALSE;
                return( ret );
            case '#':
                Token.where = ENDOFLINE;            /* begin comment */
                Token.line++;
                break;
            case '^':
                if( req != SEP_SPACE ) {    /* if not storing system blocks */
                    Token.next++;
                    BackupParser();
                    break;
                }
                Token.next--;   /* make a token out of this */
                ret = MakeToken( ctrl, req );
                Token.quoted = FALSE;
                return( ret );
            case '%':
                if( req != SEP_SPACE ) {
                    ExpandEnvVariable();
                    break;
                }
            default:
                if( need_sep ) {
                    Token.quoted = FALSE;
                    switch( req ) {
                    case SEP_NO:
                        if( hmm == ',' || hmm == '=' ) return( FALSE );
                        break;
                    case SEP_COMMA:
                        if( hmm != ',' ) return( FALSE);
                        Token.next++;
                        break;
                    case SEP_EQUALS:
                        if( hmm != '=' ) return( FALSE );
                        Token.next++;
                        break;
                    case SEP_PERIOD:
                    case SEP_DOT_EXT:
                        if( hmm != '.' ) return( FALSE );
                        Token.next++;
                        break;
                    case SEP_PAREN:
                        if( hmm != '(' ) return( FALSE );
                        Token.next++;
                        break;
                    case SEP_LCURLY:
                        if( hmm != '{' ) return( FALSE );
                        Token.next++;
                        break;
                    case SEP_QUOTE:
                        if( hmm != '\'' ) return FALSE;
                        Token.next++;
                        Token.quoted = TRUE;
                        break;
                    case SEP_RCURLY:
                        if( hmm != '}' ) return( FALSE );
                        Token.next++;
                        return( TRUE );
                    case SEP_END:
                        return( FALSE );
                    }
                    need_sep = FALSE;
                    EatWhite();
                } else {                /*  must have good separator here */
                    if( hmm == '\'' && req != SEP_PAREN ) {
                        req = SEP_QUOTE;   /* token has been quoted */
                        Token.next++;      /* don't include the quote */
                        Token.quoted = TRUE;
                    }
                    ret = MakeToken( ctrl, req );
                    return( ret );
                }
                break;
            }
            break;
        case ENDOFLINE:
            GetNewLine();
            break;
        case ENDOFFILE:
            if( Token.locked ) return( FALSE );
            RestoreCmdLine();
            Token.quoted = FALSE;
            break;
        case ENDOFCMD:
            if( CmdFile->next != NULL ) {
                RestoreParser();
                break;
            }
            Token.quoted = FALSE;
            ret = ( req == SEP_END ) ? TRUE : FALSE;
            return( ret );
        }
    }
}


unsigned char MakeToken( unsigned ctrl, unsigned separator )
/**********************************************************/
{
    unsigned char    quit;
    char    hmm = 0;
    int     len;
    unsigned char    forcematch;
    unsigned char    hitmatch;
    unsigned char    keepspecial;

    /* The old first MapDoubleByteChar call reads an uninitialized register.
     * English mode ignores it; zero it here. Other language modes remain
     * unsupported in both builds rather than inventing that missing input. */
    if( CmdFlags & 0x3000 ) {
#ifdef __WATCOMC__
        reconstruction_trap();
#else
        __asm__ __volatile__( "int3" );
#endif
    }
    Token.this = Token.next;
    len = 0;
    quit = FALSE;
    forcematch = (separator == SEP_QUOTE) || (separator == SEP_PAREN)
                 || (separator == SEP_PERCENT);
    keepspecial = (separator == SEP_SPACE) || (separator == SEP_DOT_EXT);
    if( separator == SEP_DOT_EXT ) {    /* KLUDGE! we want to allow a zero*/
        len--;                  /* length token for parsing wlib files, so */
        Token.next--;           /* artificially back up one here. */
    }
    if( *Token.next == '\\' && separator == SEP_QUOTE
                         && !(ctrl & TOK_IS_FILENAME) ) {
        MapEscapeChar();        /* get escape chars starting in 1st pos. */
    }
    len += MapDoubleByteChar( hmm );
    hitmatch = FALSE;
    for(;;) {
        len++;
        hmm = *++Token.next;
        switch( hmm ) {
        case '\'':
            if( separator == SEP_QUOTE ) {
                ++Token.next;      // don't include end quote in next token.
                hitmatch = TRUE;
                quit = TRUE;
            }
            break;
        case ')':
            if( separator == SEP_PAREN ) {
                ++Token.next;    // don't include end paren in next token.
                hitmatch = TRUE;
                quit = TRUE;
            }
            break;
        case '%':
            if( separator == SEP_PERCENT ) {
                ++Token.next;    // don't include end percent in next token.
                hitmatch = TRUE;
                quit = TRUE;
            }
            break;
        case '.':
            if( !(ctrl & TOK_INCLUDE_DOT) && !forcematch ) {
                quit = TRUE;
            }
            break;
        case '{':
        case '}':
        case '(':
        case ',':
        case '=':
        case '#':
        case '@':
            if( keepspecial ) {
                break;
            }                   // NOTE the potential fall through
        case '\t':
        case ' ':
            if( !forcematch ) {
                quit = TRUE;
            }
            break;
        case '\\':
            if( separator == SEP_QUOTE && !(ctrl & TOK_IS_FILENAME) ) {
                MapEscapeChar();
            }
            break;
        case '\0':
        case '\r':
        case '\n':
        case CTRLZ:
            quit = TRUE;
            break;
        default:
            len += MapDoubleByteChar( hmm );
        }
        if( quit ) {
            break;
        }
    }
    Token.len = len;
    if( forcematch && !hitmatch ) {
        return( FALSE );     // end quote/paren not found before token end.
    }
    return( TRUE );
}



int MapDoubleByteChar( unsigned char c )
{
    switch( CmdFlags & 0x3000 ) {
    case 0x1000:
        if( (c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xfc) ) {
            ++Token.next;
            return 1;
        }
        break;
    case 0x2000:
        if( c > 0xfc ) break;
        /* fall through */
    case 0x3000:
        if( c > 0xfd || c < 0x81 ) break;
        ++Token.next;
        return 1;
    }
    return 0;
}

void StartNewFile( void )
{
    char *fname, *envstring, *buff;
    unsigned envlen;
    int file;

    fname = FileName( Token.this, Token.len, 0, 0 );
    file = QObjOpen( fname );
    if( file == -1 ) {
        _LnkFree( fname );
        fname = tostring();
        envstring = getenv( fname );
        if( envstring != 0 ) {
            envlen = strlen( envstring );
            buff = ChkLAlloc( envlen + 1 );
            memcpy( buff, envstring, envlen );
            buff[envlen] = '\0';
            NewCommandSource( fname, buff, ENVIRONMENT );
        } else {
            LnkMsg( 0x7b5d, "s", fname );
            _LnkFree( fname );
            Suicide();
        }
    } else {
        SetCommandFile( file, fname );
    }
}

/* Filename extension selection is the actual 1994 command-format table. */
static const char *DefExt[] = {
    ".lnk", ".map", ".lib", ".obj", ".exe", ".ovl", ".dll", ".exp",
    ".nlm", ".lan", ".dsk", ".nam", ".com", ".rex", ".qnx", ".sym", ".lbc"
};
char *FileName( char *buff, unsigned len, unsigned etype, int force )
/******************************************************************/
{
    char *  namptr;
    char *  namstart;
    char *  ptr;
    int     cnt;
    int     namelen;

    namptr = buff + len;
    cnt = 0;
    while( cnt != len ) {
        cnt++;
        --namptr;
        if( (*namptr == '\\' || *namptr == '/' || *namptr == ':') ) break;
    }
    if( (*namptr == '\\' || *namptr == '/' || *namptr == ':') ) {
        namptr++;
    }
    namstart = namptr;
    cnt = len - ( (int) namptr - (int) buff );
    if( cnt == 0 ) {
        ptr = alloca( len + 1 );
        memcpy( ptr, buff, len );
        ptr[len] = '\0';
        LnkMsg( 0x7c8a, "s", ptr );
    }
    namelen = cnt;
    namptr = buff + len - 1;
    while( --cnt != 0 && *namptr != '.' ) {
        namptr--;
    }
    if( force || *namptr != '.' ) {
        if( force && etype == 1 ) {         // op map goes in current dir.
            buff = namstart;
            len = namelen;
        }
        if( cnt != 0 ) {
            len = namptr - buff;
        }
        ptr = ChkLAlloc( len + strlen( DefExt[ etype ] ) + 1 );
        memcpy( ptr, buff, len );
        strcpy( ptr + len, DefExt[ etype ] );
    } else {
        ptr = ChkLAlloc( len + 1 );
        memcpy( ptr, buff, len );
        ptr[ len ] = '\0';
    }
    return( ptr );
}


void SetCommandFile( int file, char *fname )
{
    unsigned long size;
    char *buff;

    if( QIsADevice( file ) ) size = 0x10000UL;
    else size = QFileSize( file );
    buff = 0;
    if( size < 65510 ) {
        buff = LAlloc( size + 1 );
        if( buff != 0 ) {
            size = QRead( file, buff, size, fname );
            buff[size] = '\0';
            NewCommandSource( fname, buff, BUFFERED );
        }
    }
    if( buff == 0 ) {
        buff = ChkLAlloc( 257 );
        NewCommandSource( fname, buff, NONBUFFERED );
        Token.where = ENDOFLINE;
        ++Token.line;
    }
    CmdFile->file = file;
}

void GetNewLine( void )
{
    if( Token.how == BUFFERED || Token.how == ENVIRONMENT || Token.how == SYSTEM ) {
        Token.where = MIDST;
        while( *Token.next != '\n' ) {
            if( *Token.next == '\0' || *Token.next == CTRLZ ) {
                Token.where = ENDOFFILE;
                break;
            }
            ++Token.next;
        }
        ++Token.next;
    } else if( Token.how == NONBUFFERED ) {
        if( QReadStr( CmdFile->file, Token.buff, 256, CmdFile->name ) )
            Token.where = ENDOFFILE;
        else Token.where = MIDST;
        Token.next = Token.buff;
    } else {
        OutPutPrompt( "WLINK>" );
        Token.how = INTERACTIVE;
        if( QReadStr( fileno(stdin), Token.buff, 256, "console" ) )
            Token.where = ENDOFCMD;
        else Token.where = MIDST;
        Token.next = Token.buff;
    }
}

unsigned char ProcOne( parse_entry *entry, unsigned req, unsigned char suicide )
/*******************************************************************/
/* recognize token out of parse table, with required separator            */
/* return FALSE if no separator, Suicide if not recognized (if suicide is */
/* TRUE) otherwise use return code from action routine in matching entry  */
{
    char                *key;
    char                *ptr;
    int                 plen;
    unsigned char       ret;
    char                keybuff[20];

    ret = GetToken( req, TOK_INCLUDE_DOT );
    if( ret == FALSE ) {
        return( ret );
    }
    while( entry->keyword != NULL ) {
        key = entry->keyword;
        ptr = Token.this;
        plen = Token.len;
        for(;;) {
            if( plen == 0 && !isupper( *key ) ) {
                if( HintFormat( entry->format ) ) {
                    ret = (*entry->rtn)();
                } else {
                    strcpy( keybuff, entry->keyword );
                    strlwr( keybuff );
                    LnkMsg( 0x7a3a, "s", keybuff );
                    ret = TRUE;
                }
                return( ret );
            }
            if( *key == '\0' || tolower( *ptr ) != tolower( *key ) ) break;
            ptr++;
            key++;
            plen--;
        }
        /* here if this is no match */
        entry++;
    }
    /* here if no match in table */
    if( suicide ) {
        Syntax();
    } else {
        Token.thumb = REJECT;       /*  try again later */
        ret = FALSE;
    }
    return( ret );
}


static parse_entry Models[] = {
    { "Dos", ProcDos, 0x00e0 },
    { "OS2", ProcOS2, 0x0007 },
    { "WINdows", ProcOS2, 0x0018 },
    { "PHARlap", ProcPharlap, 0x3c00 },
    { "NOVell", ProcNovell, 0x0100 },
    { "QNX", ProcQNX, 0x4200 },
    { "ELF", ProcELF, 0x8000 },
    { 0, 0, 0 }
};

unsigned char ProcFormat( void )
{
    if( LinkState & 0x20 ) LnkMsg( 0x7c39, 0 );
    LinkState |= 0x20;
    return ProcOne( Models, SEP_NO, 1 );
}

void RestoreParser( void )
{
    if( CmdFile->next == 0 ) return;
    memcpy( &CmdFile->token, &Token, sizeof(tok) );
    CmdFile = CmdFile->next;
    memcpy( &Token, &CmdFile->token, sizeof(tok) );
}

unsigned char ProcRuntime( void )
{
    return ProcArgList( AddRuntime, TOK_INCLUDE_DOT );
}

unsigned char ProcArgList( unsigned char (*rtn)(void), unsigned ctrl )
/*************************************************************/
{
    if( GetToken( SEP_LCURLY, ctrl ) ) {
        for(;;) {
            if( !WildCard( rtn, ctrl ) ) {
                return( FALSE );
            }
            if( CheckFence() ) {
                break;
            } else if( !GetToken( SEP_NO, ctrl ) ) {
                LnkMsg( 0x7b84, NULL );
                break;
            }
        }
    } else {
        if( GetToken( SEP_NO, ctrl ) == FALSE ) return( FALSE );
        do {
            if( !WildCard( rtn, ctrl ) ) {
                return( FALSE );
            }
        } while( GetToken( SEP_COMMA, ctrl ) );
    }
    return( TRUE );
}


unsigned char WildCard( unsigned char (*rtn)(void), unsigned ctrl )
{
    char *              p;
    char *              start;
    DIR *               dir;
    struct dirent *     dirent;
    char                drive[_MAX_DRIVE];
    char                directory[_MAX_DIR];
    char                name[_MAX_FNAME];
    char                extin[_MAX_EXT];
    char                pathin[_MAX_PATH];
    unsigned char       wildcrd;
    unsigned char       retval = 0;

    wildcrd = FALSE;
    if( ctrl & TOK_IS_FILENAME ) {
        p = Token.this;
        for(;;) {               // check if wildcard
            /* end of parm: NULLCHAR or blank */
            if( *p == '\'' ) break;     // don't wildcard a quoted string.
            if( *p == '\0' ) break;
            if( *p == ' ' ) break;
            if( *p == '?' || *p == '*' ) {
                wildcrd = TRUE;
                break;
            }
            p++;
        }
    }
    /* The old successful wildcard branch returns an uninitialized local.
     * Its value is not reconstructed. Trap that domain in both builds while
     * retaining its authentic callback and CRT edges below. */
    if( wildcrd ) {
#ifdef __WATCOMC__
        reconstruction_trap();
#else
        __asm__ __volatile__( "int3" );
#endif
    }
    if( !wildcrd ) {
        retval = rtn();
    } else {
        retval = TRUE;
        /* expand file names */
        start = tostring();
        dir = opendir( start );
        if( dir != NULL ) {
            _splitpath( start, drive, directory, NULL, NULL );
            for(;;) {
                dirent = readdir( dir );
                if( dirent == NULL ) break;
                if( dirent->d_attr &
                  (_A_HIDDEN+_A_SYSTEM+_A_VOLID+_A_SUBDIR) ) continue;
                _splitpath( dirent->d_name, NULL, NULL, name, extin );
                _makepath( pathin, drive, directory, name, extin );
                Token.this = pathin;            // dangerous?
                Token.len = strlen( pathin );
                if( !(*rtn)() ) {
                    Token.this = NULL;
                    Token.thumb = OK;   // make _sure_ we don't use token.this
                    retval = FALSE;
                    break;
                }
            }
            closedir( dir );
        } else {
            retval = rtn();
        }
        _LnkFree( start );
    }
    return retval;
}

static parse_entry RunOptions[] = {
    { "MINReal", ProcMinReal, 0x0800 },
    { "MAXReal", ProcMaxReal, 0x0800 },
    { "REALBreak", ProcRealBreak, 0x0800 },
    { "CALLbufs", ProcCallBufs, 0x0800 },
    { "MINIBuf", ProcMiniBuf, 0x0800 },
    { "MAXIBuf", ProcMaxiBuf, 0x0800 },
    { "NISTack", ProcNIStack, 0x0800 },
    { "ISTKsize", ProcIStkSize, 0x0800 },
    { "UNPRIVileged", ProcUnpriv, 0x0800 },
    { "PRIVileged", ProcPriv, 0x0800 },
    { "FLAGs", ProcFlags, 0x0800 },
    { "NATive", ProcRunNative, 0x0010 },
    { "WINdows", ProcRunWindows, 0x0010 },
    { "CONsole", ProcRunConsole, 0x0010 },
    { "POSix", ProcRunPosix, 0x0010 },
    { "OS2", ProcRunOS2, 0x0010 },
    { "DOSstyle", ProcRunDosstyle, 0x0010 },
    { 0, 0, 0 }
};

unsigned char AddRuntime( void )
{
    Token.thumb = REJECT;
    return ProcOne( RunOptions, SEP_NO, 0 );
}

static parse_entry DbgMods[] = {
    { "All", ProcAllDBI, 0xffff },
    { "Watcom", ProcWatcomDBI, 0xffff },
    { "Dwarf", ProcDwarfDBI, 0xffff },
    { "COdeview", ProcCodeviewDBI, 0xffff },
    { "Novell", ProcNovDBI, 0x0100 },
    { "ONLYExports", ProcExportsDBI, 0x0100 },
    { 0, 0, 0 }
};

unsigned char ProcDebug( void )
{
    if( CmdFlags & 8 ) LnkMsg( 0x7a26, 0 );
    if( ProcOne( DbgMods, SEP_NO, 0 ) ) return 1;
    return ProcWatcomDBI();
}

/* The eight-byte old wrapper falls through into the independently callable
 * ProcAll entry. Keep that real tail edge rather than duplicating its body. */
unsigned char ProcAllDBI( void )
{
    ProcWatcomDBI();
    return ProcAll();
}

static parse_entry PosDbgMods[] = {
    { "LInes", ProcLine, 0xffff },
    { "Types", ProcType, 0xffff },
    { "LOcals", ProcLocal, 0xffff },
    { "STatic", ProcDBIStatic, 0xffff },
    { "All", ProcAll, 0xffff },
    { 0, 0, 0 }
};

unsigned char ProcWatcomDBI( void )
{
    if( LinkFlags & 0x21 ) {
        LnkMsg( 0x7a95, 0 );
        return 1;
    }
    LinkFlags |= 0x4000;
    DBIFlag &= ~0x1f;
    if( ProcOne( PosDbgMods, SEP_NO, 0 ) ) {
        while( ProcOne( PosDbgMods, SEP_COMMA, 0 ) ) {}
    } else {
        DBIFlag |= 1;
    }
    return 1;
}

unsigned char ProcAll( void )
{
    DBIFlag |= 0x17;
    return 1;
}

unsigned char ProcOptions( void )
{
    return ProcArgList( AddOption, TOK_INCLUDE_DOT );
}

unsigned char GetLong( unsigned long *addr )
{
    unsigned long value;
    if( !HaveEquals( 0 ) ) return 0;
    if( getatol( &value ) != 0 ) return 0;
    *addr = value;
    return 1;
}

unsigned char HaveEquals( unsigned ctrl )
{
    if( !GetToken( SEP_EQUALS, ctrl ) ) {
        Token.this = Token.next;
        GetToken( SEP_NO, ctrl );
        return 0;
    }
    return 1;
}

unsigned char getatol( unsigned long *pnt )
/*******************************************/
{
    char *          p;
    int             len;
    unsigned long   value;
    unsigned        radix;
    int             isvalid;
    int             isdig;
    unsigned char   gotdigit;
    char            ch;

    len = Token.len;
    if( len <= 0 ) return( 1 );
    p = Token.this;
    gotdigit = FALSE;
    value = 0ul;
    radix = 10;
    if( *p == '0' ) {
        --len;
        if( tolower(*++p) == 'x') {
            radix = 16;
            ++p;
            --len;
        }
    }
    for( ; len != 0; --len ) {
        ch = tolower( *p++ );
        if( ch == 'k' ) {         // constant of the form 64k
            if( len > 1 || !gotdigit ) {
                return( 1 );
            } else {
                value <<= 10;        // value = value * 1024;
            }
        } else if( ch == 'm' ) {        // constant of the form 64M
            if( len > 1 || !gotdigit ) {
                return( 1 );
            } else {
                value <<= 20;
            }
        } else {
            isdig = isdigit( ch );
            if( radix == 10 ) {
                isvalid = isdig;
            } else {
                isvalid = isxdigit( ch );
            }
            if( !isvalid ) {
                return( 1 );
            }
            value *= radix;
            if( isdig ) {
                value += ch - '0';
            } else {
                value += ch - 'a' + 10;
            }
            gotdigit = TRUE;
        }
    }
    *pnt = value;
    return( 0 );
}


char *tostring( void )
{
    char *str;
    unsigned len;
    len = Token.len;
    str = ChkLAlloc( len + 1 );
    memcpy( str, Token.this, len );
    str[len] = '\0';
    return str;
}

char *GetFileName( char **membname, unsigned char setname )
{
    char *objname;
    unsigned namelen;

    namelen = Token.len;
    objname = alloca( namelen );
    memcpy( objname, Token.this, namelen );
    if( GetToken( SEP_PAREN, TOK_INCLUDE_DOT ) ) {
        /* The old version keeps the member token verbatim; it has no later
         * RemovePath call before allocating the member name. */
        *membname = tostring();
        return FileName( objname, namelen, 2, 0 );
    }
    *membname = 0;
    if( setname && Name == 0 ) {
        Name = ChkLAlloc( namelen + 1 );
        memcpy( Name, objname, namelen );
        Name[namelen] = '\0';
    }
    return FileName( objname, namelen, 3, 0 );
}

unsigned char getatoi( unsigned short *pnt )
{
    unsigned char status;
    unsigned long value;

    status = getatol( &value );
    if( status == 0 ) {
        if( value > 65535UL ) return 2;
        *pnt = value;
    }
    return status;
}

unsigned char ProcDwarfDBI( void )
{
    if( LinkFlags & 0x4020 ) {
        LnkMsg( 0x7a95, 0 );
        return 1;
    }
    LinkFlags |= 1;
    DBIFlag |= 0x17;
    return 1;
}

void ExpandEnvVariable( void )
{
    char *name, *env, *buff;
    unsigned len;

    ++Token.next;
    if( !MakeToken( TOK_INCLUDE_DOT, SEP_PERCENT ) ) LnkMsg( 0x7c72, 0 );
    name = tostring();
    env = getenv( name );
    if( env == 0 ) {
        LnkMsg( 0x7a73, "s", name );
        _LnkFree( name );
    } else {
        len = strlen( env );
        if( *Token.next != ' ' && *Token.next != '\t' && *Token.next != '\r' ) {
            MakeToken( TOK_INCLUDE_DOT, SEP_SPACE );
            buff = ChkLAlloc( len + Token.len + 1 );
            memcpy( buff, env, len );
            memcpy( buff + len, Token.this, Token.len );
            buff[len + Token.len] = 0;
        } else {
            buff = ChkLAlloc( len + 1 );
            memcpy( buff, env, len );
            buff[len] = 0;
        }
        NewCommandSource( name, buff, ENVIRONMENT );
    }
}

char *totext( void )
{
    Token.thumb = REJECT;
    if( !GetToken( SEP_NO, 0 ) ) GetToken( SEP_NO, TOK_INCLUDE_DOT );
    return tostring();
}

unsigned char ProcLine( void )
{
    DBIFlag |= 0x1;
    return 1;
}

unsigned char ProcType( void )
{
    DBIFlag |= 0x2;
    return 1;
}

unsigned char ProcLocal( void )
{
    DBIFlag |= 0x4;
    return 1;
}

unsigned char ProcDBIStatic( void )
{
    DBIFlag |= 0x10;
    return 1;
}

void MapEscapeChar( void )
/*******************************/
/* turn the current character located at Token.next into a possibly unprintable
 * character using C escape codes */
{
    char *      str;
    int         shift;

    shift = 2;
    str = Token.next + 1;
    switch( *str ) {
    case 'a':
        *Token.next = '\a';
        break;
    case 'b':
        *Token.next = '\b';
        break;
    case 'f':
        *Token.next = '\f';
        break;
    case 'n':
        *Token.next = '\n';
        break;
    case 'r':
        *Token.next = '\r';
        break;
    case 't':
        *Token.next = '\t';
        break;
    case 'v':
        *Token.next = '\v';
        break;
    case 'x':
        shift += ParseNumber( ++str, 16 );
        break;
    case '0': case '1': case '2': case '3': case '4': case'5': case '6':
    case '7': case '8': case '9':
        shift += ParseNumber( str, 8 ) - 1;
        break;
    default:
        *Token.next = *str;
        break;
    }
    str = Token.next + shift;
    memmove( Token.next + 1, str, strlen( str ) + 1 );
}


int ParseNumber( char * str, int radix )
/*********************************************/
/* read a (possibly hexadecimal) number */
{
    int         isdig;
    int         isvalid;
    unsigned char ch;
    int         size;
    unsigned    value;

    size = 0;
    value = 0;
    for(;;) {
        ch = tolower( (unsigned char)*str );
        isdig = isdigit( ch );
        if( radix == 8 ) {
            /* Preserve the old condition, which also admits octal 8/9. */
            isvalid = isdig && !(ch == '8' && ch == '9');
        } else {
            isvalid = isxdigit( ch );
        }
        if( !isvalid ) break;
        value *= radix;
        if( isdig ) {
            value += ch - '0';
        } else {
            value += ch - 'a' + 10;
        }
        size++;
        str++;
    }
    *Token.next = value;
    return( size );
}


void BackupParser( void )
{
    if( CmdFile->prev == 0 ) {
        LnkMsg( 0x7a70, 0 );
        return;
    }
    memcpy( &CmdFile->token, &Token, sizeof(tok) );
    CmdFile = CmdFile->prev;
    memcpy( &Token, &CmdFile->token, sizeof(tok) );
}

unsigned char CheckFence( void )
{
    if( Token.thumb == REJECT ) {
        if( !Token.quoted && *Token.this == '}' ) {
            ++Token.this;
            return 1;
        }
        return 0;
    }
    return GetToken( SEP_RCURLY, 0 );
}

void OutPutPrompt( char *text )
{
    if( QIsADevice( CmdFile->file ) )
        QWrite( fileno(stdout), text, strlen(text), 0 );
}
