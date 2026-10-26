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
#include <malloc.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "asminit.h"
#include "asmparse.h"
#include "breakout.h"
#include <recon.h>

#ifdef __WATCOMC__
/* add_constant uses the shipped compiler's inline strchr form. */
#pragma intrinsic(strchr,strlen,strcpy,strcat)
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
                case 'e': /* original1994 error-file option */
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

/* CFccb0..d0e7; original local buffers264/264/260/80. */
void get_fname( char *token, int type )
/********************************************/
/*
 * figure out the source file name & store it in AsmFiles
 * fill in default object file name if it is null
 */
{
    char        *def_drive, *def_dir, *def_fname, *def_ext;
    char        *drive, *dir, *fname, *ext;
    char        buffer[ 264 ];
    char        buffer2[ 264 ];
    char        name [ 260  ];
    char        msgbuf[80];

    /* get filename for source file */

    if( type == ASM ) {
        if( token == NULL ) {
            MsgGet( 1087, msgbuf );
            Fatal( 1, msgbuf );
        }
        if( AsmFiles.fname[ASM] != NULL ) {
            Fatal( 5 );
        }

        _splitpath2( token, buffer, &drive, &dir, &fname, &ext );
        if( *ext == '\0' ) {
            ext = "asm";
        }
        _makepath( name, drive, dir, fname, ext );
        AsmFiles.fname[ASM] = AsmAlloc( strlen( name ) + 1 );
        strcpy( AsmFiles.fname[ASM], name );

        _makepath( name, drive, dir, NULL, NULL );
        /* add the source path to the include path */
        AddStringToIncludePath( name );

        if( AsmFiles.fname[OBJ] == NULL ) {
            /* set up default object and error filename */
            ext = "obj";
            _makepath( name, NULL, NULL, fname, ext );
        } else {
            _splitpath2( AsmFiles.fname[OBJ], buffer2, &def_drive,
                         &def_dir, &def_fname, &def_ext );
            if( *def_fname == '\0' ) def_fname = fname;
            if( *def_ext == '\0' ) def_ext = "obj";

            _makepath( name, def_drive, def_dir, def_fname, def_ext );
            AsmFree( AsmFiles.fname[OBJ] );
        }
        AsmFiles.fname[OBJ] = AsmAlloc( strlen( name ) + 1 );
        strcpy( AsmFiles.fname[OBJ], name );

        if( AsmFiles.fname[ERR] == NULL ) {
            ext = "err";
            _makepath( name, NULL, NULL, fname, ext );
        } else {
            _splitpath2( AsmFiles.fname[ERR], buffer2, &def_drive,
                         &def_dir, &def_fname, &def_ext );
            if( *def_fname == '\0' ) def_fname = fname;
            if( *def_ext == '\0' ) def_ext = "err";
            _makepath( name, def_drive, def_dir, def_fname, def_ext );
            AsmFree( AsmFiles.fname[ERR] );
        }
        AsmFiles.fname[ERR] = AsmAlloc( strlen( name ) + 1 );
        strcpy( AsmFiles.fname[ERR], name );

    } else {
        /* get filename for object file */
        _splitpath2( token, buffer, &drive, &dir, &fname, &ext );
        if( AsmFiles.fname[ASM] != NULL ) {
            _splitpath2( AsmFiles.fname[ASM], buffer2, &def_drive,
                         &def_dir, &def_fname, &def_ext );
            if( *fname == '\0' ) {
                fname = def_fname;
            }
        }
        if( *ext == '\0' ) {
            ext = type == ERR ? "err" : "obj";
        }
        _makepath( name, drive, dir, fname, ext );
        AsmFiles.fname[type] = AsmAlloc( strlen( name ) + 1 );
        strcpy( AsmFiles.fname[type], name );
    }
}


/* CFd45c..d614. The CF CRT records the loader OS byte before main.
 * The normal NT build uses the released source's ordinary Win32 fallback. */
int set_build_target(void)
{
    char *tmp;
    if (Options.build_target == NULL) {
        Options.build_target = AsmAlloc(8);
#if defined(__NT__) && !defined(__OSI__)
        strcpy(Options.build_target, "NT");
#else
        if (__OS == 0) strcpy(Options.build_target, "DOS");
        else if (__OS == 1) strcpy(Options.build_target, "OS2");
        else if (__OS == 2) strcpy(Options.build_target, "NT");
        else if (__OS == 3) strcpy(Options.build_target, "WINDOWS");
        else strcpy(Options.build_target, "XXX");
#endif
    }
    strupr(Options.build_target);
    tmp = alloca(strlen(Options.build_target) + 5);
    strcpy(tmp, "__");
    strcat(tmp, Options.build_target);
    strcat(tmp, "__");
    add_constant(tmp);
    if (stricmp(Options.build_target, "DOS") == 0) {
        add_constant("__MSDOS__");
    } else if (stricmp(Options.build_target, "NETWARE") == 0) {
        if ((Code->info.cpu & 0x70) >= 0x30)
            add_constant("__NETWARE_386__");
    } else if (stricmp(Options.build_target, "WINDOWS") == 0) {
        if ((Code->info.cpu & 0x70) >= 0x30)
            add_constant("__WINDOWS_386__");
    }
    return 1;
}

/* CFc9e8..ca94; only the loader-selected target's include variable. */
void get_os_include(void)
{
    char *env, *tmp;
    tmp = alloca(strlen(Options.build_target) + 10);
    strcpy(tmp, Options.build_target);
    strcat(tmp, "_INCLUDE");
    env = getenv(tmp);
    if (env != NULL) AddStringToIncludePath(env);
}

/* CFcc50..ccae; fopen returns the CRT-owned source stream. */
void open_files(void)
{
    AsmFiles.file[ASM] = fopen(AsmFiles.fname[ASM], "r");
    if (AsmFiles.file[ASM] == NULL) Fatal(1, AsmFiles.fname[ASM]);
    pobjState.file_out = ObjWriteOpen(AsmFiles.fname[OBJ]);
    if (pobjState.file_out == NULL) Fatal(1, AsmFiles.fname[OBJ]);
    pobjState.pass = 2;
    DelErrFile();
}

/* CFcc40..cc4f, initial-source finalization sequence. */
void main_fini(void)
{
    free_names();
    GenMSOmfFini();
    AsmShutDown();
}

/* CFcbac..cc0c, free each command-line option's original owned string. */
void free_names(void)
{
    if (Options.build_target != NULL) AsmFree(Options.build_target);
    if (Options.code_class != NULL) AsmFree(Options.code_class);
    if (Options.data_seg != NULL) AsmFree(Options.data_seg);
    if (Options.module_name != NULL) AsmFree(Options.module_name);
    if (Options.text_seg != NULL) AsmFree(Options.text_seg);
}

/* CFd208..d45b. The1994 CPU selection has no later686 cases. */
char *set_processor_type(char *input)
/*******************************************/
{
    enum asm_token      token;
    char                protect = 0;
    char                *tmp = input+1;

    for( tmp=input+1; *tmp != '\0' && !isspace( *tmp ); tmp++ ) {
        if( *input == 'f' ) {
            tmp = input + strcspn( input, "/-" );
            break;
        } else if( *tmp == '/' || *tmp == '-' ) {
            break;
        } else if( *tmp == 'r' ) {
            Options.naming_convention = ADD_USCORES;
            add_constant( "__REGISTER__" );
        } else if( *tmp == 's' ) {
            add_constant( "__STACK__" );
            Options.naming_convention = DO_NOTHING;
        } else if( *tmp == '_' ) {
            if( Options.naming_convention == DO_NOTHING ) {
                Options.naming_convention = REMOVE_USCORES;
            } else {
                Options.naming_convention = DO_NOTHING;
            }
        } else if( *tmp == 'p' ) {
            protect = 1;
        } else if( *tmp == '"' ) {
            char *dest;
            tmp++;
            Options.default_name_mangler = AsmAlloc( strlen( tmp ) + 1 );
            dest = Options.default_name_mangler;
            for( ; *tmp != '"'; dest++, tmp++ ) {
                *dest = *tmp;
            }
            *dest = '\0';
        } else {
            MsgPrintf1( 1102, input );
            exit( 1 );
        }
    }

    switch( *input ) {
    case '0':
        token = T_8086;
        break;
    case '1':
        token = T_186;
        break;
    case '2':
        token =  protect ? T_286P : T_286;
        break;
    case '3':
        token =  protect ? T_386P : T_386;
        break;
    case '4':
        token =  protect ? T_486P : T_486;
        break;
    case '5':
        token =  protect ? T_586P : T_586;
        break;
    case '7':
        switch( Code->info.cpu & P_CPU_MASK ) {
        case P_286:
            token =  T_287;
            break;
        case P_386:
        case P_486:
        case P_586:
            token =  T_387;
            break;
        case P_86:
        case P_186:
        default:
            token =  T_8087;
            break;
        }
        break;
    case 'f':       // fp#
        switch( *(input+2) ) {
        case 'c':
            token = T_NO87;
            break;
        case '0':
            token = T_8087;
            break;
        case '2':
            token = T_287;
            break;
        case '3':
        case '5':
            token = T_387;
            break;
        }
        break;
    /* no other cases are possible */
    }
    cpu_directive( token );

    return( tmp );
}

/* CFd614..d70e, command-line memory model selection. */
void set_mem_type( char mem_type )
/***************************************/
{
    char buffer[20];
    char *model;

    switch( mem_type ) {
    case 'c':
        model = "COMPACT";
        break;
    case 'f':
        model = "FLAT";
        break;
    case 'h':
        model = "HUGE";
        break;
    case 'l':
        model = "LARGE";
        break;
    case 'm':
        model = "MEDIUM";
        break;
    case 's':
        model = "SMALL";
        break;
    case 't':
        model = "TINY";
        break;
    default:
        strcpy( buffer, "/m" );
        strcat( buffer, &mem_type );
        MsgPrintf1( 1102, buffer );
        exit( 1 );
    }

    strcpy( buffer, ".MODEL " );
    strcat( buffer, model );
    InputQueueLine( buffer );

    return;
}


/* CFd710..d78a, four separately owned option strings. */
void set_some_kinda_name( char token, char *name )
/*******************************************************/
/* set:  code class / data seg. / module name / text seg */
{
    int len;
    char **tmp;

    len = strlen( name ) + 1;
    switch( token ) {
    case 'c':
        tmp = &Options.code_class;
        break;
    case 'd':
        tmp = &Options.data_seg;
        break;
    case 'm':
        tmp = &Options.module_name;
        break;
    case 't':
        tmp = &Options.text_seg;
        break;
    default:
        return;
    }
    *tmp = AsmAlloc( len );
    strcpy( *tmp, name );
    return;
}
