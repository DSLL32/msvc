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
* Description:  Operator and keyword token spellings.
*
****************************************************************************/


#include "scanner.h"
#include "expression.h"
#include <recon.h>
/* ctokens.h's semantic operator order. The old :> token precedes the
 * identifier/constant/string entries. Its47-keyword perfect-hash order is
 * the initial keyword set in released keywords.h, with the old class numbers. */
char *Tokens[101]={
    "end of line","end of file",",","?",":",";","(",")","[","]","{","}",".","~",
    "=","==","!","!=","|","|=","||","^","^=","&","&=","&&",
    ">",">=",">>",">>=","<","<=","<<","<<=","+","+=","++","-","-=","--",
    "*","*=","/","/=","%","%=","#","...","##","->",":>",
    "identifier","constant","string",
    "__cdecl",     "__pascal",     "__syscall",     "auto",
    "switch",     "if",     "enum",     "volatile",
    "while",     "return",     "goto",     "break",
    "__huge",     "void",     "else",     "__segname",
    "__export",     "__segment",     "__loadds",     "__interrupt",
    "__saveregs",     "short",     "struct",     "__based",
    "__far",     "__near",     "double",     "__self",
    "do",     "default",     "signed",     "unsigned",
    "float",     "case",     "typedef",     "sizeof",
    "const",     "continue",     "long",     "static",
    "int",     "for",     "__fortran",     "extern",
    "register",     "union",     "char"
};

unsigned char TokenClass[101]={
 TC_XX,TC_XX,TC_COMMA,TC_QUESTION,TC_COLON,TC_XX,
 TC_FUNC_CALL,TC_RIGHT_PAREN,TC_LEFT_BRACKET,TC_RIGHT_BRACKET,TC_XX,TC_XX,
 TC_DOT,TC_XX,TC_ASSIGN_OP,TC_EQ_NE,TC_XX,TC_EQ_NE,
 TC_OR,TC_ASSIGN_OP,TC_OR_OR,TC_XOR,TC_ASSIGN_OP,TC_AND,TC_ASSIGN_OP,TC_AND_AND,
 TC_REL_OP,TC_REL_OP,TC_SHIFT_OP,TC_ASSIGN_OP,TC_REL_OP,TC_REL_OP,TC_SHIFT_OP,TC_ASSIGN_OP,
 TC_ADD_OP,TC_ASSIGN_OP,TC_POSTINC,TC_ADD_OP,TC_ASSIGN_OP,TC_POSTINC,
 TC_MUL_OP,TC_ASSIGN_OP,TC_MUL_OP,TC_ASSIGN_OP,TC_MUL_OP,TC_ASSIGN_OP,
 TC_XX,TC_XX,TC_XX,TC_ARROW,TC_SEG_OP,TC_XX,TC_XX,TC_XX,
    TC_CDECL,     TC_PASCAL,     TC_SYSCALL,     TC_STG_CLASS,
    TC_KEYWORD,     TC_KEYWORD,     TC_KEYWORD,     TC_QUALIFIER,
    TC_KEYWORD,     TC_KEYWORD,     TC_KEYWORD,     TC_KEYWORD,
    TC_HUGE,     TC_KEYWORD,     TC_KEYWORD,     TC_SEGNAME,
    TC_EXPORT,     TC_SEGMENT,     TC_LOADDS,     TC_INTERRUPT,
    TC_SAVEREGS,     TC_KEYWORD,     TC_KEYWORD,     TC_BASED,
    TC_FAR,     TC_NEAR,     TC_KEYWORD,     TC_SELF,
    TC_KEYWORD,     TC_KEYWORD,     TC_KEYWORD,     TC_KEYWORD,
    TC_KEYWORD,     TC_KEYWORD,     TC_STG_CLASS,     TC_KEYWORD,
    TC_QUALIFIER,     TC_KEYWORD,     TC_KEYWORD,     TC_STG_CLASS,
    TC_KEYWORD,     TC_KEYWORD,     TC_FORTRAN,     TC_STG_CLASS,
    TC_STG_CLASS,     TC_KEYWORD,     TC_KEYWORD
};
