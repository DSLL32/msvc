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

/* bld/wasm/c/fatal.c at6b9cb4438959d845a2b4dd4bf19f7908ecd608e9.
 * Actual CFf5a0..f614 source/ownership sequence. */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "wasm.h"
#include <recon.h>

void AsmShutDown(void)
{
    if (AsmFiles.file[ASM] != NULL) {
        if (fclose(AsmFiles.file[ASM]) != 0)
            Fatal(2,AsmFiles.fname[ASM]);
    }
    ObjWriteClose(pobjState.file_out);
    ObjRecFini();
    if (!write_to_file || Options.error_count > 0)
        remove(AsmFiles.fname[OBJ]);
    AsmFree(AsmFiles.fname[ASM]);
    AsmFree(AsmFiles.fname[ERR]);
    AsmFree(AsmFiles.fname[OBJ]);
}

/* Initial fatal.h records, actual16-byte rows at CFf540..f5a0.
 * Message IDs are the1994 resource enumeration. */
typedef struct {
    int num;
    int message;
    void (*action)(void);
    int ret;
} Msg_Struct;
const Msg_Struct Fatal_Msg[] = {
    { 0, 1089, AsmShutDown, 0 }, /* OUT_OF_MEMORY */
    { 1, 1096, NULL, 1 },        /* CANNOT_OPEN_FILE */
    { 1, 1097, NULL, 1 },        /* CANNOT_CLOSE_FILE */
    { 0, 1098, AsmShutDown, 1 }, /* CANNOT_GET_FILE */
    { 0, 1099, AsmShutDown, 1 }, /* CANNOT_SET_FILE */
    { 0, 1100, NULL, 1 }         /* TOO_MANY_FILES */
};
void MsgPrintf(int resourceid);

/* CFf614..f69e: terminate using the selected fatal record. */
void Fatal(unsigned message, ...)
{
    va_list args;
    int i;
    if (message >= sizeof(Fatal_Msg) / sizeof(Fatal_Msg[0])) WASM_PENDING();
    MsgPrintf(1086); /* MSG_ERROR */
    MsgPrintf(Fatal_Msg[message].message);
    if (Fatal_Msg[message].num > 0) {
        va_start(args, message);
        for (i = Fatal_Msg[message].num; i > 0; --i)
            printf("%s", va_arg(args, char *));
        va_end(args);
    }
    printf("\n");
    if (Fatal_Msg[message].action != NULL)
        Fatal_Msg[message].action();
    exit(Fatal_Msg[message].ret);
}
