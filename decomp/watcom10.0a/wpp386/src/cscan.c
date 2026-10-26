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

/* Initial Open Watcom cscan.c (6b9cb443), adapted to the actual 1994
 * classifier. CR is whitespace here, and delimiters use the older combined
 * classes. Later tokenSource/ReScanPtr initialization is absent. */
#include <string.h>
#include "frontend.h"
#include "pending.h"
#include "scan.h"
#include "types.h"
#include "macro.h"
#include "scanchar.h"
#include "compiler.h"
#include <recon.h>

enum scan_class {
    SCAN_NAME, SCAN_WIDE, SCAN_NUM, SCAN_QUESTION, SCAN_DELIM,
    SCAN_HASH, SCAN_COMPOUND_DELIM, SCAN_ASSIGN_DELIM, SCAN_SLASH,
    SCAN_COLON, SCAN_MINUS, SCAN_FLOAT, SCAN_STRING, SCAN_STRING2,
    SCAN_LSTRING2, SCAN_CHARCONST, SCAN_NEWLINE, SCAN_WHITESPACE,
    SCAN_INVALID, SCAN_EOF
};
enum { LCHR_EOF=256, LCHR_CONTINUE_STRING, LCHR_CONTINUE_LSTRING, SCAN_LCHR_MAX };
static unsigned char ClassTable[SCAN_LCHR_MAX];
unsigned char PPState;
static unsigned char InitClassTable[] = {
    '\n',SCAN_NEWLINE, ' ',SCAN_WHITESPACE, '\f',SCAN_WHITESPACE,
    '\r',SCAN_WHITESPACE, '\t',SCAN_WHITESPACE, '\v',SCAN_WHITESPACE,
    '\'',SCAN_CHARCONST, '"',SCAN_STRING,
    '(',SCAN_DELIM, ')',SCAN_DELIM, ',',SCAN_DELIM, ';',SCAN_DELIM,
    '?',SCAN_QUESTION, '[',SCAN_DELIM, ']',SCAN_DELIM,
    '{',SCAN_DELIM, '}',SCAN_DELIM, '~',SCAN_DELIM, '.',SCAN_FLOAT,
    '!',SCAN_ASSIGN_DELIM, '#',SCAN_HASH, '%',SCAN_ASSIGN_DELIM,
    '&',SCAN_COMPOUND_DELIM, '*',SCAN_ASSIGN_DELIM,
    '+',SCAN_COMPOUND_DELIM, '-',SCAN_MINUS, '/',SCAN_SLASH, ':',SCAN_COLON,
    '<',SCAN_COMPOUND_DELIM, '=',SCAN_ASSIGN_DELIM, '>',SCAN_COMPOUND_DELIM,
    '^',SCAN_ASSIGN_DELIM, '|',SCAN_COMPOUND_DELIM,
    '_',SCAN_NAME, 'L',SCAN_WIDE, 0,0
};

void ScanInit(void)
{
    const unsigned char *p;

    PPState = 0;
    CompFlags.scanning_c_comment = 0;
    memset(ClassTable, SCAN_INVALID, sizeof(ClassTable));
    memset(&ClassTable['A'], SCAN_NAME, 26);
    memset(&ClassTable['a'], SCAN_NAME, 26);
    memset(&ClassTable['0'], SCAN_NUM, 10);
    ClassTable[LCHR_EOF] = SCAN_EOF;
    ClassTable[LCHR_CONTINUE_STRING] = SCAN_STRING2;
    ClassTable[LCHR_CONTINUE_LSTRING] = SCAN_LSTRING2;
    for (p = InitClassTable; *p; p += 2) ClassTable[p[0]] = p[1];
}

static void (*tokenSource)(void)=nextMacroToken;

int NextToken(void)
{
    if (CurToken==0x91) {
        CurToken=LAToken;
        return CurToken;
    }
    tokenSource();
    return CurToken;
}

void nextMacroToken(void)
{
    do {
        CurToken=0;
        if (macro_expansion_state) {
            GetMacroToken(0);
            if (CurToken==0) {
                if (CompFlags.cpp_output) PrtChar(' ');
                CurToken=ScanToken(0);
            }
        } else {
            if (CompFlags.cpp_output) PrtChar(' ');
            CurToken=ScanToken(0);
        }
    } while (CurToken==0x8f);
}

int scanName(int expanding);
int scanWide(int expanding);
int scanNum(int expanding);
int scanHex(int expanding);
int doScanFloat(void);
void unGetChar(int c);
int scanQuestion(int expanding);
int scanDelim(int expanding);
int scanHash(int expanding);
int scanCompoundDelim(int expanding);
int scanAssignDelim(int expanding);
int scanSlash(int expanding);
int scanColon(int expanding);
int scanMinus(int expanding);
int scanFloat(int expanding);
int scanString(int expanding);
int scanStringContinue(int expanding);
int scanLStringContinue(int expanding);
int scanCharConst(int expanding);
int scanNewline(int expanding);
int SrcFileScanWhiteSpace(int expanding);
int scanInvalid(int expanding);
int scanEof(int expanding);

/* The encountered1994 classifier has20 semantic scanner classes. */
static int (*scanFunc[])(int)={
    scanName, scanWide, scanNum, scanQuestion, scanDelim, scanHash, scanCompoundDelim, scanAssignDelim, scanSlash, scanColon, scanMinus, scanFloat, scanString, scanStringContinue, scanLStringContinue, scanCharConst, scanNewline, SrcFileScanWhiteSpace, scanInvalid, scanEof
};


char *SavedId;
void LookPastName(void)
{
    SavedId=NameCreateNoLen(Buffer);
    NextToken();
    LAToken=CurToken;
    CurToken=0x91;
}

int ScanToken(int expanding)
{
    Buffer[0]=(char)CurrChar;
    TokenLen=1;
    return scanFunc[ClassTable[CurrChar]](expanding);
}

static char *ReScanPtr;
void ReScanInit(char *text)
{
    ReScanPtr=text;
}
int rescanBuffer(void)
{
    CurrChar=(unsigned char)*ReScanPtr++;
    if (!CurrChar) CompFlags.rescan_buffer_done=1;
    return CurrChar;
}
int ReScanToken(void)
{
    unsigned line=TokenLine, column=TokenColumn;
    int current=CurrChar;
    int (*next)(void)=NextChar;
    CompFlags.rescan_buffer_done=0;
    NextChar=rescanBuffer;
    NextChar();
    CurToken=ScanToken(1);
    --ReScanPtr;
    CurrChar=current;
    NextChar=next;
    TokenLine=line;
    TokenColumn=column;
    return !CompFlags.rescan_buffer_done;
}


int CurrChar, CurToken, LAToken, TokenLen;
unsigned TokenLine, TokenColumn;
int doScanName(int c, int expanding);
int idLookup(unsigned length);
int KwLookup(unsigned length);
void SkipAhead(void);
MEPTR fmentry;

int scanName(int expanding)
{
    int c;
    SrcFileCurrentLocation();
    c=NextChar();
    ++TokenLen;
    Buffer[TokenLen-1]=(char)c;
    return doScanName(c,expanding);
}
int scanWide(int expanding) { (void)expanding; STUB_TRAP(); return 0; }
int scanNum(int expanding)
{
    int c, first, max_digit, too_big;
    unsigned long max_value;
    SrcFileCurrentLocation();
    Constant=0;
    c=CurrChar;
    too_big=0;
    if (c=='0') {
        first=c=saveNextChar();
        if ((c|0x20)=='x') {
            if (scanHex(expanding)) c=CurrChar;
            else {
                unGetChar(CurrChar);
                --TokenLen;
                CurrChar=c=first;
            }
        } else {
            max_digit=expanding ? '9' : '7';
            while (c>='0' && c<=max_digit) {
                if (Constant&0xe0000000ul) too_big=1;
                Constant=(Constant<<3)+(c-'0');
                c=saveNextChar();
            }
            c=CurrChar;
            switch (c) {
            case '8': case '9':
                while (CharSet[c]&C_DI) c=saveNextChar();
                if (c!='.' && c!='e' && c!='E') {
                    if (NestLevel==SkipLevel) CErr1(678);
                }
                /* fall through: recover as a floating token. */
            case '.': case 'e': case 'E': return doScanFloat();
            }
        }
        max_value=0xfffffffful;
    } else {
        while (CharSet[c]&C_DI) {
            if (Constant>0x19999999ul ||
                (Constant==0x19999999ul && c>'5')) too_big=1;
            Constant=Constant*10+(c-'0');
            c=saveNextChar();
        }
        if (c=='.' || (c|0x20)=='e') return doScanFloat();
        max_value=0x7ffffffful;
    }
    c|=0x20;
    if (c=='l') {
        ConstType=9; /* TYP_SLONG */
        c=saveNextChar();
        if ((c|0x20)=='u') {
            saveNextChar();
            ConstType=10;
        } else if (Constant>0x7ffffffful) ConstType=10;
    } else if (c=='u') {
        c=saveNextChar();
        if ((c|0x20)=='l') { saveNextChar(); ConstType=10; }
        else ConstType=8;
    } else {
        ConstType=7;
        if (Constant>max_value) {
            ConstType=9;
            if (Constant>0x7ffffffful) ConstType=10;
        } else if (Constant>0x7ffffffful) ConstType=8;
    }
    if (too_big && NestLevel==SkipLevel) CErr1(2);
    Buffer[--TokenLen]='\0';
    return 0x37;
}
int scanQuestion(int expanding)
{
    (void)expanding;
    SrcFileCurrentLocation();
    Buffer[1]=0;
    NextChar();
    return 3;
}
int scanDelim(int expanding)
{
    int token;
    (void)expanding;
    SrcFileCurrentLocation();
    Buffer[1]=0;
    token=TokValue[(unsigned char)CurrChar];
    NextChar();
    return token;
}
int scanHash(int expanding) { (void)expanding; STUB_TRAP(); return 0; }
int scanCompoundDelim(int expanding)
{
    int first, next, token, length=1;
    (void)expanding;
    SrcFileCurrentLocation();
    first=CurrChar;
    token=TokValue[(unsigned char)first];
    next=NextChar();
    Buffer[1]=(char)next;
    if (next=='=') {
        NextChar();
        ++token;
        length=2;
    } else if (next==first) {
        next=NextChar();
        length=2;
        token+=2;
        if (next=='=' && (token==0x21 || token==0x1d)) {
            Buffer[2]='=';
            ++token;
            ++length;
            NextChar();
        }
    }
    TokenLen=length;
    Buffer[length]=0;
    return token;
}
int scanAssignDelim(int expanding)
{
    int token, c, length;
    (void)expanding;
    SrcFileCurrentLocation();
    /* Reached portion of the old TokValue operator mapping. The scanner
     * class routes exactly these five punctuation characters here. */
    switch (CurrChar) {
    case '!': token=0x11; break;
    case '%': token=0x2d; break;
    case '*': token=0x29; break;
    case '=': token=0x0f; break;
    case '^': token=0x16; break;
    default: STUB_TRAP(); return 0;
    }
    length=1;
    c=NextChar();
    Buffer[1]=(char)c;
    if (c=='=') {
        NextChar();
        ++token;
        length=2;
    }
    TokenLen=length;
    Buffer[length]='\0';
    return token;
}
int scanSlash(int expanding) { (void)expanding; STUB_TRAP(); return 0; }
int scanColon(int expanding)
{
    int next,token=4,length=1;
    (void)expanding;
    SrcFileCurrentLocation();
    next=NextChar();
    Buffer[1]=(char)next;
    if (next==':') {
        NextChar();
        token=6;
        length=2;
    } else if (next=='>') {
        NextChar();
        token=51;
        length=2;
    }
    TokenLen=length;
    Buffer[length]=0;
    return token;
}
int scanMinus(int expanding)
{
    int c,token=38,length=1;
    (void)expanding;
    SrcFileCurrentLocation();
    c=NextChar();
    Buffer[1]=(char)c;
    if(c=='=') { NextChar(); token=39; length=2; }
    else if(c=='-') { NextChar(); token=40; length=2; }
    else if(c=='>') {
        c=NextChar();
        length=2;
        if(c=='*') { token=52; length=3; Buffer[2]=(char)c; NextChar(); }
        else token=50;
    }
    TokenLen=length;
    Buffer[length]=0;
    return token;
}
int scanFloat(int expanding)
{
    (void)expanding;
    SrcFileCurrentLocation();
    return doScanFloat();
}
extern int TokenSavedLen;
int doESCChar(int c);
int doScanString(int string_type);
int scanString(int expanding)
{
    (void)expanding;
    SrcFileCurrentLocation();
    return doScanString(1);
}
/* Old scanner keeps literal escape spelling in Buffer; doESCChar advances
 * over the escape payload. Later source gained separate conversion modes. */
int doScanString(int string_type)
{
    int c, ok=0;
    c=NextChar();
    TokenLen=1;
    Buffer[0]=(char)c;
    for (;;) {
        if (c=='\n' || c==256) break;
        if (c=='"') { NextChar(); ok=1; break; }
        if (c=='\\') {
            c=NextChar();
            Buffer[TokenLen++]=(char)c;
            if (!(CharSet[c]&C_WS)) doESCChar(c);
            c=CurrChar;
        } else {
            if (CharSet[c]&C_DB) Buffer[TokenLen++]=(char)NextChar();
            if (TokenLen>480) {
                CurrChar=257+(string_type==4);
                ++TokenLen;
                ok=1;
                break;
            }
            c=NextChar();
            Buffer[TokenLen++]=(char)c;
        }
    }
    TokenSavedLen=TokenLen;
    Buffer[TokenLen-1]=0;
    if (ok) return string_type==4 ? 0xa3 : 0x38;
    --TokenLen;
    BadTokenInfo=67;
    Buffer[TokenLen-1]=0;
    return 0x94;
}
int doESCChar(int c) { (void)c; STUB_TRAP(); return 0; }
int scanStringContinue(int expanding) { (void)expanding; STUB_TRAP(); return 0; }
int scanLStringContinue(int expanding) { (void)expanding; STUB_TRAP(); return 0; }
int scanCharConst(int expanding) { (void)expanding; STUB_TRAP(); return 0; }
int scanNewline(int expanding)
{
    while (CurrChar=='\n') {
        if (PPState) return 0;
        SrcFileLineLocation();
        if (TBreak()) { CErr1(101); CSuicide(); }
        ChkControl();
    }
    return ScanToken(expanding);
}
int SrcFileScanWhiteSpace(int expanding)
{
    int c;
    (void)expanding;
    do { c=NextChar(); } while (CharSet[c]&C_WS);
    return 0x8f;
}
int scanInvalid(int expanding) { (void)expanding; STUB_TRAP(); return 0; }
int scanEof(int expanding) { (void)expanding; STUB_TRAP(); return 0; }

int doScanName(int c, int expanding)
{
    unsigned length, batch;
    length=TokenLen-1;
    if (CharSet[c] & (C_AL|C_DI)) {
        ++length;
        for (;;) {
            /* Preserve the old scanner's eight-character batches and
             * delayed clamp; its buffer has explicit unrolled-read slack. */
            for (batch=0; batch<8; ++batch) {
                c=NextChar();
                if (!(CharSet[c] & (C_AL|C_DI))) goto name_done;
                Buffer[length++]=(char)c;
            }
            if (length>512) length=512;
        }
    }
name_done:
    if (length>=510) {
        if (NestLevel==SkipLevel) CErr1(106);
        length=510;
    }
    TokenLen=length;
    Buffer[length]='\0';
    if (expanding || PPState==2) return 0x36;
    CurToken=idLookup(length);
    if (CurToken==0x80) {
        if (CompFlags.cpp_output) PrtChar(' ');
        if (fmentry->macro_defn==0) return SpecialMacro(fmentry);
        fmentry->macro_flags |= 8;
        if (fmentry->parm_count) {
            SkipAhead();
            if (CurrChar!='(') {
                if (CompFlags.cpp_output) {
                    Buffer[TokenLen++]=' ';
                    Buffer[TokenLen]='\0';
                    return 0x36;
                }
                return KwLookup(TokenLen);
            }
        }
        DoMacroExpansion();
        GetMacroToken(0);
        if (CurToken==0) CurToken=0x8f;
    }
    return CurToken;
}

MEPTR MacroSearch(void);
int idLookup(unsigned length)
{
    MEPTR entry=MacroSearch();
    if (entry) { fmentry=entry; return 0x80; }
    return KwLookup(length);
}
int skipWhiteSpace(int c);
int ScanOptionalComment(void);
void SkipAhead(void)
{
    int c;
    do {
        c=skipWhiteSpace(CurrChar);
        while (c=='\n') {
            if (!PPState && CompFlags.cpp_output) PrtChar('\n');
            c=NextChar();
            c=skipWhiteSpace(c);
        }
    } while (ScanOptionalComment());
}
int skipWhiteSpace(int c)
{
    if (CompFlags.cpp_output && !PPState) {
        while (CharSet[c]&C_WS) {
            if (c!='\r') PrtChar(c);
            c=NextChar();
        }
    } else {
        while (CharSet[c]&C_WS) {
            c=NextChar();
            if (!(CharSet[c]&C_WS)) break;
            c=NextChar();
        }
    }
    return c;
}
void scanCComment(void) { STUB_TRAP(); }
void scanCppComment(void) { STUB_TRAP(); }
int ScanOptionalComment(void)
{
    int result=0, c;
    for (;;) {
        c=CurrChar;
        if (c!='/') break;
        c=NextChar();
        if (c=='*') scanCComment();
        else if (c=='/') scanCppComment();
        else {
            unGetChar(c);
            CurrChar='/';
            break;
        }
        result=1;
    }
    return result;
}


int TokenSavedLen;
unsigned long Constant;
unsigned ConstType, BadTokenInfo;

int saveNextChar(void)
{
    int c=NextChar();
    if (TokenLen<510) Buffer[TokenLen++]=(char)c;
    else if (TokenLen==510) {
        if (NestLevel==SkipLevel) CErr1(106);
        ++TokenLen;
    }
    return c;
}

int scanHex(int expanding) { (void)expanding; STUB_TRAP(); return 0; }
int doScanFloat(void)
{
    int c=CurrChar;
    if(c=='.') {
        do { c=saveNextChar(); } while(CharSet[c]&C_DI);
        if(TokenLen==2) {
            if(c=='*') {
                NextChar();
                return 0x35;
            }
            if(c=='.') {
                c=saveNextChar();
                if(c=='.') {
                    NextChar();
                    return 0x30;
                }
                unGetChar(c);
                CurrChar='.';
            }
            Buffer[1]=0;
            TokenLen=1;
            return 0x0d;
        }
    }
    CurToken=0x37;
    if((c|0x20)=='e') {
        c=saveNextChar();
        if(c=='+' || c=='-') c=saveNextChar();
        if(!(CharSet[c]&C_DI)) {
            CurToken=0x94;
            BadTokenInfo=90;
        }
        while(CharSet[c]&C_DI) c=saveNextChar();
    }
    c|=0x20;
    if(c=='f') {
        NextChar();
        ConstType=TYP_FLOAT;
    } else if(c=='l') {
        NextChar();
        ConstType=TYP_LONG_DOUBLE;
    } else {
        ConstType=TYP_DOUBLE;
        --TokenLen;
    }
    Buffer[TokenLen]=0;
    return CurToken;
}
void unGetChar(int c) { (void)c; STUB_TRAP(); }

TOKEN_SOURCE SetTokenSource(TOKEN_SOURCE source)
{
    TOKEN_SOURCE previous=tokenSource;
    tokenSource=source;
    return previous;
}
void ResetTokenSource(TOKEN_SOURCE source) { tokenSource=source; }
