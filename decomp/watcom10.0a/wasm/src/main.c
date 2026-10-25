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
* Description: Original-source attribution retained for the adapted entry.
*
****************************************************************************/

/* Adapted from bld/wasm/c/main.c, Open Watcom initial commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9. Actual Watcom 10.0a CF
 * cb28..cb5d determines the six edges, quiet+2 and unsigned error_count+8.
 * main was qualified by the first ordinary original/native pair. */
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "asminit.h"
#include <recon.h>

#ifdef __WATCOMC__
/* add_constant uses the shipped compiler's inline strchr form. */
#pragma intrinsic(strchr,strlen,strcpy)
#endif

global_options Options = {
    0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 20, 2, 0,
    NULL, NULL, NULL, NULL, NULL, NULL, 0
};

int main(int argc, char **argv)
{
    main_init();
    do_init_stuff(argc, argv);
    WriteObjModule();
    if (!Options.quiet) {
        PrintStats();
    }
    MsgFini();
    main_fini();
    return (int)Options.error_count;
}

/* Source main_init, checked against CF cc0c..cc3f. */
File_Info AsmFiles;

void main_init(void)
{
    int i;
    for (i = ASM; i <= OBJ; ++i) {
        AsmFiles.file[i] = NULL;
        AsmFiles.fname[i] = NULL;
    }
    ObjRecInit();
    GenMSOmfInit();
}

/* Actual 1994 ca94..cb26 differs from the later version macro and MsgInit. */
void do_init_stuff(int argc, char **argv)
{
    char *env;
    if (MsgInit(argv[0]) != 0) {
        exit(1);
    }
    AsmInit(-1, -1, -1);
    add_constant("__WASM__");
    do_envvar_cmdline("WASM", 0);
    parse_cmdline(argc, argv);
    set_build_target();
    get_os_include();
    env = getenv("INCLUDE");
    if (env != NULL) {
        AddStringToIncludePath(env);
    }
    if (!Options.quiet && !Options.banner_printed) {
        Options.banner_printed = 1;
        trademark();
    }
    open_files();
    PushLineQueue();
}

/* CFd184..d205; the caller owns any writable separator in the input string. */
void add_constant(char *string)
{
    char *tmp;
    tmp = strchr(string, '=');
    if (tmp == NULL) {
        tmp = strchr(string, '#');
        if (tmp == NULL) {
            tmp = "1";
        } else {
            *tmp++ = '\0';
        }
    } else {
        *tmp++ = '\0';
    }
    if (isvalidident(string) == -1) {
        AsmError(51); /* SYNTAX_ERROR */
        return;
    }
    StoreConstant(string, tmp, 0);
}

/* CFc970..c9e5; only the digit/lower classes from the original IsTable. */
int isvalidident(char *id)
{
    char *s;
    char lwr_char;
    if (isdigit(*id)) return -1;
    for (s = id; *s != '\0'; ++s) {
        lwr_char = tolower((unsigned char)*s);
        if (!(lwr_char == '_' || lwr_char == '.' || lwr_char == '$'
              || lwr_char == '@' || lwr_char == '?'
              || isdigit(lwr_char) || islower(lwr_char))) {
            return -1;
        }
    }
    return 1;
}

/* CFdd50..dd53, folded AsmQueryExternal/AsmQueryType stand-alone hooks.
 * The byte result is zero; upper register bits are not part of the ABI. */
unsigned char AsmQueryExternal(char *name)
{
    (void)name;
    return 0;
}

/* CFd0e8..d181. The shipped space test compares with 0x2f74 ('/t'),
 * not a tab. It also skips NUL in the leading scan. Preserve those original
 * bugs; this case's explicitly absent WASM variable avoids those paths. */
void do_envvar_cmdline(char *envvar, int level)
{
    char *cmdline, *token, *next;
    if (level >= 10) return;
    cmdline = getenv(envvar);
    if (cmdline == NULL) return;
    while (*cmdline == ' ' || (unsigned char)*cmdline == 0x2f74 ||
           *cmdline == 0) ++cmdline;
    token = next = cmdline;
    while (*token != 0) {
        while (*next != ' ' && (unsigned char)*next != 0x2f74 && *next != 0)
            ++next;
        if (*next == 0) {
            parse_token(token, level + 1);
            break;
        }
        *next++ = 0;
        parse_token(token, level + 1);
        while (*next == ' ' || (unsigned char)*next == 0x2f74) ++next;
        if (*next == 0) break;
        token = next;
    }
}

/* CFdcf4..dd50. */
void parse_cmdline(int argc, char **argv)
{
    int i;
    char msgbuf[80];
    if (argc == 1) usage_msg();
    for (i = 1; i < argc; ++i) parse_token(argv[i], 0);
    if (AsmFiles.fname[ASM] == NULL) {
        MsgGet(1088, msgbuf);
        Fatal(1, msgbuf);
    }
}

/* CFd7bc..dcf3 and its 12-entry local jump table CFd78c..d7bc.
 * 1994 /h immediately invokes help. No later /hc,/hd,/hw or /u handling;
 * only d1/d2 set debug; all other d suffixes define a constant. */
void parse_token( char *token, int nesting_level )
/*******************************************************/
{
    char *ptr;
    signed char len;

    for( ;; ) {
        ptr = NULL;
        switch( *token ) {
        case '?':
            usage_msg();
            break;
        case '=':
        case '#':
            AsmError( 1085 );
            break;
    #ifndef __QNX__
        case '/':
    #endif
        case '-':
            /* options */
            token++;
            switch( tolower( *token ) ) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '7':
                ptr = set_processor_type( token );
                break;
            case 'b':
                if( *(token+1) == 't' ) {
                    token += 2;
                    if( (*token == '=' || *token == '#') ) token++;
                    if( Options.build_target != NULL ) {
                        AsmFree( Options.build_target );
                    }
                    Options.build_target = AsmAlloc( strlen( token ) + 1 );
                    strcpy( Options.build_target, token );
                    break;
                }
                MsgPrintf1( 1102, token );
                exit( 1 );
            case 'c':
                Options.output_data_in_code_records = 0;
                ptr = token+1;
                break;
            case 'h':
                usage_msg();
                break;
            case 'm':
                set_mem_type( *(token+1) );
                ptr = token+2;
                break;
            case 'n':
                if( (token[2] == '=' || token[2] == '#') ) {
                    set_some_kinda_name( *(token+1), token+3 );
                } else {
                    set_some_kinda_name( *(token+1), token+2 );
                }
                break;
            case '?':
                usage_msg();
                break;
            case 'i':
                /* set include path */
                token++;
                if( (*token == '=' || *token == '#') ) token++;
                AddStringToIncludePath( token );
                break;
            case 'e':
                if( isdigit( *(token+1) ) ) {
                    Options.error_limit = (char)atoi( token+1 );
                } else if( *(token+1) == '\0' ) {
                    /* stop reading asm file at the END directive */
                    Options.stop_at_end = 1;
                }
                ptr = token + 1 + strcspn( token+1, "/-" );
                break;
            case 'w':
                if( isdigit( *(token+1) ) ) {
                    Options.warning_level = (char)atoi( token+1 );
                } else if( *(token+1) == 'e' ) {
                    Options.warning_error = 1;
                }
                ptr = token + 1 + strcspn( token+1, "/-" );
                break;
            case 'j':
            case 's':
                /* force use of sbyte, sword, sdword etc. for signed values */
                Options.sign_value = 1;
                ptr = token+1;
                break;
            case 'o':
                Options.allow_c_octals = 1;
                ptr = token+1;
                break;
            case 'd':
                switch( *(token+1) ) {
                case '1':
                case '2':
                    Options.debug_flag = 1;
                    ptr = token + 1 + strcspn( token+1, "/-" );
                    break;
                    // make d2 different sometime

                default:
                    add_constant( token+1 );
                }
                break;
            case 'f':
                switch( *(token+1) ) {
                case 'i':
                    /* force file following to be included */
                    token += 2;
                    if( (*token == '=' || *token == '#') ) token++;
                    InputQueueFile( token );
                    break;
                case 'o':
                    token += 2;
                    if( (*token == '=' || *token == '#') ) token++;
                    get_fname( token, OBJ );
                    break;
                case 'r':
                case 'e': /* for backwards compatablity */
                    /* use this as the error file, default fname.err */
                    token += 2;
                    if( (*token == '=' || *token == '#') ) token++;
                    get_fname( token, ERR ); /* +3 to get past fe= */
                    break;
                case 'p':
                    ptr = token + 1 + strcspn( token+1, "/-" );
                    len = ptr - token;
                    if( strnicmp( token, "fpi", len ) == 0 ) {
                        Options.floating_point = 0;
                        add_constant("__FPI__");
                        set_processor_type( "7" );
                        break;
                    } else if( strnicmp( token, "fpi87", len ) == 0 ) {
                        Options.floating_point = 1;
                        add_constant("__FPI87__");
                        set_processor_type( "7" );
                        break;
                    } else if( strnicmp( token, "fpc", len ) == 0 ) {
                        Options.floating_point = 2;
                        add_constant("__FPC__");
                        set_processor_type( token );
                        break;
                    } else {
                        switch( *(token+2) ) {
                        case '0':
                        case '2':
                        case '3':
                        case '5':
                            ptr = set_processor_type( token );
                        }
                    }
                    break;
                default:
                    MsgPrintf1( 1102, token );
                    exit( 1 );
                }
                break;
            case 'q':
                Options.quiet = 1;
                ptr = token + 1;
                break;
            case 'z':
                switch( *(token+1) ) {
                case 'q':
                    Options.quiet = 1;
                    ptr = token + 2;
                    break;
                default:
                    MsgPrintf1( 1102, token );
                    exit( 1 );
                }
                break;
            default:
                MsgPrintf1( 1102, token );
                exit( 1 );
            }
            break;
        case '@':
            do_envvar_cmdline( token+1, nesting_level );
            break;
        default:
            /* must be a filename */
            get_fname( token, ASM );
            break;
        }
        if( ptr == NULL ) break;
        switch( *ptr ) {
        #ifndef __QNX__
            case '/':
        #endif
        case '-':
            break;
        default:
            return;
        }
        token = ptr;
    }
}


/* CFcb60..cb74; help deliberately terminates with status 1. */
void usage_msg(void)
{
    PrintfUsage(2000);
    exit(1);
}

/* CFcb74..cba9. These three encountered strings form the original
 * NULL-terminated FingerMsg pointer array. */
const char *FingerMsg[] = {
    "WATCOM Assembler Version 1.0",
    "Copyright by WATCOM International Corp. 1992, 1994. All rights reserved.",
    "WATCOM is a trademark of WATCOM International Corp.",
    NULL
};
int trademark(void)
{
    int count = 0;
    if (!Options.quiet) {
        while (FingerMsg[count] != NULL) printf("%s\n", FingerMsg[count++]);
    }
    return count;
}
