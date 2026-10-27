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


/* Tokens used by the encountered1994 keyword/diagnostic consumers.
 * Unknown diagnostic entries are explicitly NULL. Readers trap in both
 * builds before consuming one; no substituted spelling is supplied. */
#include <stddef.h>
#include "scan.h"
#include <recon.h>
char *Tokens[164]={
    "end of line", /* token 0x0 */
    "end of file", /* token 0x1 */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "(", /* token 0x7 */
    ")", /* token 0x8 */
    NULL,
    NULL,
    NULL,
    "}", /* token 0xc: restored after deferred inline definitions */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "+", /* token0x23: encountered MODEL_WIDTH replacement */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "while", /* T_WHILE = 0x39 */
    "__pragma", /* T___PRAGMA = 0x3a */
    "__segname", /* T___SEGNAME = 0x3b */
    "operator", /* T_OPERATOR = 0x3c */
    "if", /* T_IF = 0x3d */
    "for", /* T_FOR = 0x3e */
    "virtual", /* T_VIRTUAL = 0x3f */
    "__interrupt", /* T___INTERRUPT = 0x40 */
    "private", /* T_PRIVATE = 0x41 */
    "_Packed", /* T__PACKED = 0x42 */
    "extern", /* T_EXTERN = 0x43 */
    "__typeof", /* T___TYPEOF = 0x44 */
    "__far16", /* T___FAR16 = 0x45 */
    "__offsetof", /* T___OFFSETOF = 0x46 */
    "short", /* T_SHORT = 0x47 */
    "continue", /* T_CONTINUE = 0x48 */
    "__based", /* T___BASED = 0x49 */
    "new", /* T_NEW = 0x4a */
    "_Seg16", /* T__SEG16 = 0x4b */
    "__cdecl", /* T___CDECL = 0x4c */
    "__far", /* T___FAR = 0x4d */
    "__near", /* T___NEAR = 0x4e */
    "register", /* T_REGISTER = 0x4f */
    "__pascal", /* T___PASCAL = 0x50 */
    "__fortran", /* T___FORTRAN = 0x51 */
    "double", /* T_DOUBLE = 0x52 */
    "__self", /* T___SELF = 0x53 */
    "volatile", /* T_VOLATILE = 0x54 */
    "catch", /* T_CATCH = 0x55 */
    "__stdcall", /* T___STDCALL = 0x56 */
    "const", /* T_CONST = 0x57 */
    "static", /* T_STATIC = 0x58 */
    "default", /* T_DEFAULT = 0x59 */
    "long", /* T_LONG = 0x5a */
    "union", /* T_UNION = 0x5b */
    "_Syscall", /* T__SYSCALL = 0x5c */
    "float", /* T_FLOAT = 0x5d */
    "this", /* T_THIS = 0x5e */
    "enum", /* T_ENUM = 0x5f */
    "__segment", /* T___SEGMENT = 0x60 */
    "typedef", /* T_TYPEDEF = 0x61 */
    "case", /* T_CASE = 0x62 */
    "__export", /* T___EXPORT = 0x63 */
    "unsigned", /* T_UNSIGNED = 0x64 */
    "goto", /* T_GOTO = 0x65 */
    "return", /* T_RETURN = 0x66 */
    "else", /* T_ELSE = 0x67 */
    "class", /* T_CLASS = 0x68 */
    "protected", /* T_PROTECTED = 0x69 */
    "auto", /* T_AUTO = 0x6a */
    "break", /* T_BREAK = 0x6b */
    "asm", /* T_ASM = 0x6c */
    "delete", /* T_DELETE = 0x6d */
    "throw", /* T_THROW = 0x6e */
    "template", /* T_TEMPLATE = 0x6f */
    "struct", /* T_STRUCT = 0x70 */
    "__huge", /* T___HUGE = 0x71 */
    "switch", /* T_SWITCH = 0x72 */
    "do", /* T_DO = 0x73 */
    "int", /* T_INT = 0x74 */
    "__saveregs", /* T___SAVEREGS = 0x75 */
    "void", /* T_VOID = 0x76 */
    "inline", /* T_INLINE = 0x77 */
    "sizeof", /* T_SIZEOF = 0x78 */
    "char", /* T_CHAR = 0x79 */
    "__loadds", /* T___LOADDS = 0x7a */
    "friend", /* T_FRIEND = 0x7b */
    "try", /* T_TRY = 0x7c */
    "public", /* T_PUBLIC = 0x7d */
    "signed", /* T_SIGNED = 0x7e */
};
