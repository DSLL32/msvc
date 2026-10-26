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

/* Initial fe_misc/c/cmdscan.c at6b9cb443, checked against actual1994
 * input cursor/switch-start state. LowerChar tests uppercase before tolower,
 * rather than the later unconditional call. */
#include <ctype.h>
#include "cmdline.h"
#include "scanchar.h"
#include "cmdln-options.h"
#include <recon.h>

static const char *cmd_scanner, *switch_start;

void CmdScanInit(const char *position)
{
    cmd_scanner = position;
}

int CmdScanChar(void)
{
    return (unsigned char)*cmd_scanner++;
}

int CmdScanLowerChar(void)
{
    int c;
    c = CmdScanChar();
    if (CharSet[c] & C_AL) c = tolower(c);
    return c;
}

int CmdScanWhiteSpace(void)
{
    int c;
    do {
        c = CmdScanLowerChar();
    } while ((CharSet[c] & C_WS) || c == '\n' || c == '\r');
    return c;
}

void CmdScanSwitchBegin(void)
{
    switch_start = cmd_scanner - 1;
}

const char *CmdScanUngetChar(void)
{
    --cmd_scanner;
    return CmdScanAddr();
}

const char *CmdScanAddr(void)
{
    return cmd_scanner;
}

unsigned CmdScanId(const char **name)
{
    const char *start, *p;
    start = p = CmdScanUngetChar();
    *name = p;
    while ((CharSet[(unsigned char)*p] & (C_AL|C_DI)) || *p == '_') ++p;
    cmd_scanner = p;
    return (unsigned)(p-start);
}

int CmdPeekChar(void)
{
    int c;
    c = CmdScanLowerChar();
    CmdScanUngetChar();
    return c;
}

int CmdScanRecogEquals(void)
{
    int c;
    c = CmdPeekChar();
    if (c == '#' || c == '=') {
        CmdScanChar();
        return 1;
    }
    return 0;
}

int CmdRecogChar(int expected)
{
    if (CmdScanLowerChar()==expected) return 1;
    CmdScanUngetChar();
    return 0;
}

int CmdScanSwEnd(void)
{
    unsigned c=(unsigned char)*cmd_scanner;
    return c==0 || (CharSet[c]&C_WS) || c==(unsigned)SwitchChar || c=='\r' || c=='\n';
}

int cmdFileChar(void)
{
    if (*cmd_scanner=='-') return 1;
    return !CmdScanSwEnd();
}

int CmdDelimitChar(void)
{
    if (!cmdFileChar()) return 1;
    return *cmd_scanner=='-';
}

unsigned CmdScanFilename(const char **name)
{
    const char *start=cmd_scanner;
    *name=start;
    while (cmdFileChar()) ++cmd_scanner;
    return (unsigned)(cmd_scanner-start);
}
