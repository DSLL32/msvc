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


/* bld/wasm/c/asmmsg.c at6b9cb443; CF16294..1630b.
 * The1994 limit byte is zero-extended and compared with integer-1. */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "asminit.h"
#include <recon.h>
void print_include_file_nesting_structure(void);
void AsmSuicide(void);
void PrtMsg(char *prefix, int message, va_list first, va_list second);

void AsmErr(int message, ...)
{
    va_list args1, args2;
    va_start(args1, message);
    va_start(args2, message);
    if (Options.error_limit == -1 || Options.error_count < Options.error_limit) {
        PrtMsg("Error!", message, args1, args2);
        va_end(args1);
        va_end(args2);
        ++Options.error_count;
        print_include_file_nesting_structure();
    } else {
        PrtMsg("", 1117, args1, args2);
        AsmSuicide();
    }
}

/* CF164a4..164ae: terminal error-limit path, no App return. */
void AsmSuicide(void) { exit(1); }
/* CF16380..163f6: the two independent va_list arguments are array
 * pointers in the original Watcom ABI. */
static FILE *ErrFile;
static int Errfile_Written;
void OpenErrFile(void);
void PutMsg(FILE *fp, char *prefix, int message, va_list args);

void PrtMsg(char *prefix, int message, va_list first, va_list second)
{
    if (!Options.banner_printed) {
        Options.banner_printed = 1;
        trademark();
    }
    if (ErrFile == NULL) OpenErrFile();
    PutMsg(stdout, prefix, message, first);
    fflush(stdout);
    if (ErrFile != NULL) {
        Errfile_Written = 1;
        PutMsg(ErrFile, prefix, message, second);
    }
}

/* CF16404..16430: nonterminal stdout suppresses the requested error file. */
void OpenErrFile(void)
{
    if (!isatty(fileno(stdout))) return;
    if (AsmFiles.fname[ERR] != NULL)
        ErrFile = fopen(AsmFiles.fname[ERR], "w");
}
/* CF16430..164a1, caller-owned256-byte message buffer. */
void PutMsg(FILE *fp, char *prefix, int message, va_list args)
{
    char *name;
    unsigned line;
    char buffer[256];
    if (fp != NULL) {
        name = get_curr_filename();
        line = LineNumber;
        if (line != 0 && name != NULL)
            fprintf(fp, "%s(%u): ", name, line);
        fprintf(fp, "%s %c%03d: ", prefix, *prefix, message);
        MsgGet(message, buffer);
        vfprintf(fp, buffer, args);
        fprintf(fp, "\n");
    }
}

/* CF1625c..16291; retain two independent va_list owners. */
void AsmNote(int message, ...)
{
    va_list first, second;
    va_start(first, message);
    va_start(second, message);
    PrtMsg("Note!", message, first, second);
    va_end(first);
    va_end(second);
}

/* CF1630c..1637d; the warning limit is an unsigned byte. */
void AsmWarn(int level, int message, ...)
{
    va_list first, second;
    if (level <= Options.warning_level) {
        va_start(first, message);
        va_start(second, message);
        if (!Options.warning_error) {
            PrtMsg("Warning!", message, first, second);
            ++Options.warning_count;
        } else {
            PrtMsg("Error!", message, first, second);
            ++Options.error_count;
        }
        va_end(first);
        va_end(second);
    }
}

/* CF16250..1625a: fixed-message entry used by the instruction library. */
void AsmError(int message)
{
    AsmErr(message);
}
