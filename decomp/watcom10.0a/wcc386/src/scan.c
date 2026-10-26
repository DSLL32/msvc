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
#include "compiler.h"
#include "scanner.h"
#include "macros.h"
#include "stubs.h"
#include <recon.h>

/* The older scanner uses one generic delimiter class for =, :, and *,
 * and treats CR as whitespace. Keep its actual class numbering. */
enum scan_class {
    SCAN_NAME,SCAN_WIDE,SCAN_NUM,SCAN_QUESTION,SCAN_SLASH,SCAN_MINUS,
    SCAN_DELIM1,SCAN_DELIM2,SCAN_DOT,SCAN_STRING,SCAN_STRING2,
    SCAN_CHARCONST,SCAN_NEWLINE,SCAN_WHITESPACE,SCAN_INVALID,
    SCAN_MACRO,SCAN_EOF
};
#define EOF_CHAR 256
#define CONTINUE_CHAR_STRING 257
#define MACRO_CHAR 258
static unsigned char ClassTable[259];
int CurrChar,SizeOfCount;
int (*NextChar)();

unsigned char InitClassTable[]={
    '\n',SCAN_NEWLINE,' ',SCAN_WHITESPACE,'\f',SCAN_WHITESPACE,
    '\r',SCAN_WHITESPACE,'\t',SCAN_WHITESPACE,'\v',SCAN_WHITESPACE,
    '\'',SCAN_CHARCONST,'"',SCAN_STRING,
    '(',SCAN_DELIM1,')',SCAN_DELIM1,',',SCAN_DELIM1,';',SCAN_DELIM1,
    '?',SCAN_QUESTION,'/',SCAN_SLASH,'-',SCAN_MINUS,
    '[',SCAN_DELIM1,']',SCAN_DELIM1,'{',SCAN_DELIM1,'}',SCAN_DELIM1,
    '~',SCAN_DELIM1,'.',SCAN_DOT,'!',SCAN_DELIM2,'#',SCAN_DELIM2,
    '%',SCAN_DELIM2,'&',SCAN_DELIM2,'*',SCAN_DELIM2,'+',SCAN_DELIM2,
    ':',SCAN_DELIM2,'<',SCAN_DELIM2,'=',SCAN_DELIM2,'>',SCAN_DELIM2,
    '^',SCAN_DELIM2,'|',SCAN_DELIM2,'_',SCAN_NAME,'L',SCAN_WIDE,0,0
};

unsigned char TokValue[]={
    /* space !  quote #  $   %    &   quote */
    0,144,0,110,0,172,215,0,
    /* ( ) * + , - . / */
    6,7,168,226,2,229,12,170,
    /* 0 through 9, : ; < = > ? */
    26,25,24,23,22,21,20,19,18,17,4,5,222,142,218,3,
    /* @ A through O */
    126,35,2,24,15,5,20,42,36,30,0,4,39,44,32,11,
    /* P through Z, [ backslash ] ^ _ */
    3,6,18,9,7,8,41,45,10,12,13,8,0,9,149,1,
    /* backtick, a through o */
    0,35,2,24,15,5,20,42,36,30,0,4,39,44,32,11,
    /* p through z, { | } ~ DEL */
    3,6,18,9,7,8,41,45,10,12,13,10,210,11,13,0
};

void ScanInit(void)
{

    int i,c;

    memset(ClassTable,SCAN_INVALID,256);
    memset(&ClassTable['A'],SCAN_NAME,26);
    memset(&ClassTable['a'],SCAN_NAME,26);
    memset(&ClassTable['0'],SCAN_NUM,10);
    ClassTable[EOF_CHAR]=SCAN_EOF;
    ClassTable[CONTINUE_CHAR_STRING]=SCAN_STRING2;
    ClassTable[MACRO_CHAR]=SCAN_MACRO;
    for (i=0;;i+=2) {
        c=InitClassTable[i];
        if (c==0) break;
        ClassTable[c]=InitClassTable[i+1];
    }
    CurrChar='\n';
    CompFlags.pre_processing=0;
    CompFlags.scanning_comment=0;
    SizeOfCount=0;
    NextChar=GetNextChar;
}

int KwHashValue,HashValue,SavedHash;

int CalcHash(char *name,int length)
{
    /* ASCII token/keyword weights, as in the original ascii.c table.
     * Entries cover space through DEL; identifier letters share weights. */

    int hash;

    hash=length+TokValue[(unsigned char)name[0]-' ']+
                TokValue[(unsigned char)name[length-1]-' '];
    while (hash>=47) hash-=47;
    KwHashValue=hash;
    hashpjw(name);
    return HashValue;
}

void hashpjw(char *name)
{
    unsigned hash;
    unsigned char c;

    hash=(unsigned char)*name++;
    if (hash!=0) {
        c=(unsigned char)*name++;
        if (c!=0) {
            hash=(hash<<4)+c;
            for (;;) {
                hash&=0x0fff;
                c=(unsigned char)*name++;
                if (c==0) break;
                hash=(hash<<4)+c;
                hash^=hash>>12;
                hash&=0x0fff;
                c=(unsigned char)*name++;
                if (c==0) break;
                hash=(hash<<4)+c;
                hash^=hash>>12;
            }
        }
    }
    HashValue=hash%241;
    hash&=0x0fff;
    if (hash>=MACRO_HASH_SIZE) hash-=MACRO_HASH_SIZE;
    MacHashValue=hash;
}

int TokenLine,SrcFileLineNum,LAToken;
/* Source location consumed by GenQuad: original7e0a4/7e058. */
int SrcLineNum,SrcFno;
char *SavedId;
int (*ScanFunc[])()={
    ScanName,ScanWide,ScanNum,ScanQuestionMark,ScanSlash,ScanMinus,
    ScanDelim1,ScanDelim2,ScanDot,ScanString,ScanStringContinue,
    ScanCharConst,ScanNewline,ScanWhiteSpace,ScanInvalid,ScanMacroToken,ScanEof
};
int ScanToken(void)
{
    TokenLine=SrcFileLineNum;
    /* These writes are present in1994, commented out in the later source. */
    TokenLen=1;
    Buffer[0]=(char)CurrChar;
    return (*ScanFunc[ClassTable[CurrChar]])();
}

int ScanName(void)
{
    int token,c,i;
    char *scanptr;
    c=CurrChar;
    token=TokenLen-1;
    for (;;) {
        scanptr=ScanCharPtr;
        while (CharSet[c] & (C_AL|C_DI)) {
            /* The original scans eight characters between clamp checks.
             * Buffer's extra16 bytes accommodate this final partial group. */
            for (i=0;i<8;++i) {
                if (!(CharSet[c] & (C_AL|C_DI))) break;
                Buffer[token++]=(char)c;
                c=(unsigned char)*scanptr++;
            }
            if (i==8 && token>=BUF_SIZE) token=BUF_SIZE;
        }
        ScanCharPtr=scanptr;
        if (!(CharSet[c] & C_EX)) break;
        c=GetCharCheck(c);
        if (!(CharSet[c] & (C_AL|C_DI))) break;
    }
    CurrChar=c;
    if (token>=BUF_SIZE-2) {
        if (NestLevel==SkipLevel) CErr1(0x470);
        token=BUF_SIZE-2;
    }
    Buffer[token]='\0';
    TokenLen=token;
    /* Unlike2002, the1994 fast rescan path does not compute the hash. */
    if (CompFlags.doing_macro_expansion) return T_ID;
    CalcHash(Buffer,token);
    if (CompFlags.pre_processing==2) return T_ID;
    token=IdLookup();
    if (token==T_MACRO) {
        if (CompFlags.cpp_output) PrtChar(' ');
        if (NextMacro->macro_defn==0) return SpecialMacro(NextMacro);
        NextMacro->macro_flags |= 8; /* MACRO_REFERENCED */
        if (NextMacro->parm_count!=0) {
            SkipAhead();
            if (CurrChar!='(') {
                if (CompFlags.cpp_output) {
                    Buffer[TokenLen++]=' ';
                    Buffer[TokenLen]='\0';
                    return T_ID;
                }
                return KwLookup();
            }
        }
        DoMacroExpansion();
        GetMacroToken();
        token=CurToken;
        if (token==T_NULL) token=T_WHITE_SPACE;
    }
    return token;
}

unsigned Constant;
DATA_TYPE ConstType;
int ScanNum(void)
{
    int c,overflow,digit_mask;
    unsigned value,maximum;
    value=0;
    overflow=0;
    Constant=0;
    c=CurrChar;
    if (c=='0') {
        c=SaveNextChar();
        if (c=='x' || c=='X') {
            if (ScanHex(0x7f,0)==0 && NestLevel==SkipLevel) CErr1(0x48c);
            value=Constant;
            c=CurrChar;
        } else {
            digit_mask=0;
            while (c>='0' && c<='9') {
                if (value & 0xe0000000U) overflow=1;
                value=(value<<3)+c-'0';
                digit_mask |= c;
                c=SaveNextChar();
            }
            if (c=='.' || c=='e' || c=='E') return ScanDot();
            if ((digit_mask & 8) && NestLevel==SkipLevel) CErr1(0x48b);
        }
        maximum=~0U;
    } else {
        while (c>='0' && c<='9') {
            if (value>0x19999999U || (value==0x19999999U && c>'5')) overflow=1;
            value=value*10+c-'0';
            c=SaveNextChar();
        }
        if (c=='.' || c=='e' || c=='E') return ScanDot();
        maximum=0x7fffffffU;
    }
    Constant=value;
    if (c=='L' || c=='l') {
        ConstType=TYPE_LONG;
        c=SaveNextChar();
        if (c=='u' || c=='U') {
            SaveNextChar();
            ConstType=TYPE_ULONG;
        } else if (value>0x7fffffffU) ConstType=TYPE_ULONG;
    } else if (c=='u' || c=='U') {
        c=SaveNextChar();
        if (c=='L' || c=='l') {
            SaveNextChar();
            ConstType=TYPE_ULONG;
        } else ConstType=TYPE_UINT;
    } else {
        ConstType=TYPE_INT;
        if (value>maximum) {
            ConstType=TYPE_LONG;
            if (value>0x7fffffffU) ConstType=TYPE_ULONG;
        } else if (value>0x7fffffffU) ConstType=TYPE_UINT;
    }
    if (overflow && NestLevel==SkipLevel) CWarn1(6,0x6a);
    --TokenLen;
    Buffer[TokenLen]='\0';
    return T_CONSTANT;
}

int ScanDot(void)
{
    int c;
    c=CurrChar;
    if (c=='.') {
        while ((c=SaveNextChar())>='0' && c<='9') ;
        if (TokenLen==2) {
            if (c=='.') {
                c=SaveNextChar();
                if (c=='.') {
                    NextChar();
                    return 0x2f;
                }
                CurrChar='.';
                UnGetChar(c);
            }
            Buffer[1]='\0';
            TokenLen=1;
            return 0x0c;
        }
    }
    CurToken=T_CONSTANT;
    if (c=='e' || c=='E') {
        c=SaveNextChar();
        if (c=='+' || c=='-') c=SaveNextChar();
        if (c<'0' || c>'9') {
            CurToken=0x7c;
            BadTokenInfo=0x454;
        }
        while (c>='0' && c<='9') c=SaveNextChar();
    }
    if (c=='f' || c=='F') {
        NextChar();
        ConstType=TYPE_FLOAT;
    } else if (c=='l' || c=='L') {
        NextChar();
        ConstType=TYPE_DOUBLE;
    } else {
        --TokenLen;
        ConstType=TYPE_DOUBLE;
    }
    Buffer[TokenLen]='\0';
    return CurToken;
}

int SaveNextChar(void)
{
    int c;
    c=NextChar();
    if (TokenLen<BUF_SIZE-2) Buffer[TokenLen++]=(char)c;
    else if (TokenLen==BUF_SIZE-2) {
        if (NestLevel==SkipLevel) CErr1(0x470);
        ++TokenLen;
    }
    return c;
}

int NextToken(void)
{
    do {
        CurToken=T_NULL;
        if (MacroPtr!=NULL) GetMacroToken();
        if (CurToken==T_NULL) {
            TokenLine=SrcFileLineNum;
            Buffer[0]=(char)CurrChar;
            TokenLen=1;
            CurToken=(*ScanFunc[ClassTable[CurrChar]])();
        }
    } while (CurToken==T_WHITE_SPACE);
    return CurToken;
}

/* Older newline handling repeats directive processing here, including the
 * real backend break-flag callback; it then tails into the normal scanner. */
int ScanNewline(void)
{
    while (CurrChar=='\n') {
        if (CompFlags.pre_processing) return T_NULL;
        if (TBreak()) {
            CErr1(0x465);
            CSuicide();
        }
        ChkControl();
    }
    return ScanToken();
}

void SkipAhead(void)
{
    for (;;) {
        for (;;) {
            if (CharSet[CurrChar]&C_WS) SkipWhiteSpace(CurrChar);
            if (CurrChar!='\n') break;
            if (CompFlags.cpp_output && !CompFlags.pre_processing) PrtChar('\n');
            NextChar();
        }
        if (CurrChar!='/') break;
        NextChar();
        if (CurrChar=='*') {
            ScanComment();
        } else {
            UnGetChar(CurrChar);
            CurrChar='/';
            break;
        }
    }
}

int PPNextToken(void)
{
    do {
        if (MacroPtr!=NULL) {
            GetMacroToken();
            if (CurToken==T_NULL) {
                if (CompFlags.cpp_output) PrtChar(' ');
                CurToken=ScanToken();
            }
        } else {
            CurToken=ScanToken();
        }
    } while (CurToken==T_WHITE_SPACE);
    return CurToken;
}

int ScanWhiteSpace(void)
{
    char *scanptr;
    union { int c; unsigned char uc; } u;
    int i;
    if (NextChar==getCharAfterBackSlash) {
        do {
            u.c=NextChar();
        } while (CharSet[u.c]&C_WS);
    } else {
        u.c=0;
        for (;;) {
            scanptr=ScanCharPtr;
            for (;;) {
                for (i=0;i<8;++i) {
                    u.uc=(unsigned char)*scanptr++;
                    if (!(CharSet[u.c]&C_WS)) break;
                }
                if (i!=8) break;
            }
            ScanCharPtr=scanptr;
            if (!(CharSet[u.c]&C_EX)) break;
            u.c=GetCharCheck(u.c);
            if (!(CharSet[u.c]&C_WS)) break;
        }
        CurrChar=u.c;
    }
    return T_WHITE_SPACE;
}

int CLitLength;
int BadTokenInfo;
int ScanString(void)
{
    int c,ok;
    ok=0;
    CompFlags.wide_char_string=0;
    c=NextChar();
    Buffer[0]=(char)c;
    TokenLen=1;
    for (;;) {
        /* The earlier scanner ends at newline, before the later source's
         * missing-line-continue diagnostic/recovery and trigraph alert. */
        if (c=='\n' || c==256) break;
        if (c=='"') {
            NextChar();
            ok=1;
            break;
        }
        if (c=='\\') {
            c=NextChar();
            Buffer[TokenLen++]=(char)c;
            if (!(CharSet[c]&C_WS)) ESCChar(c,0);
            c=CurrChar;
        } else {
            if (CharSet[c]&C_DB) SaveNextChar();
            if (TokenLen>BUF_SIZE-32) {
                CurrChar=257;
                ++TokenLen;
                ok=1;
                break;
            }
            c=NextChar();
            Buffer[TokenLen++]=(char)c;
        }
    }
    CLitLength=TokenLen;
    Buffer[TokenLen-1]='\0';
    if (ok) return T_STRING;
    --TokenLen;
    Buffer[TokenLen-1]='\0';
    BadTokenInfo=0x42d;
    return 0x7c; /* T_BAD_TOKEN */
}

int ScanWide(void)
{
    int c,token;
    c=NextChar();
    Buffer[TokenLen++]=(char)c;
    if (c=='"') {
        token=ScanString();
        CompFlags.wide_char_string=1;
    } else if (c=='\'') token=CharConst((DATA_TYPE)0x17) /* old wide-character scan mode */;
    else token=ScanName();
    return token;
}

int ScanMinus(void)
{
    int c,token;
    c=NextChar();
    if (c=='>') token=0x31;
    else if (c=='=') token=0x26;
    else if (c=='-') token=0x27;
    else {
        Buffer[1]='\0';
        return 0x25;
    }
    Buffer[1]=(char)c;
    Buffer[2]='\0';
    TokenLen=2;
    NextChar();
    return token;
}

int ScanDelim1(void)
{
    int token,c;
    token=TokValue[CurrChar-' '];
    Buffer[1]='\0';
    c=(unsigned char)*ScanCharPtr++;
    if (CharSet[c]&C_EX) c=GetCharCheck(c);
    CurrChar=c;
    return token;
}

int ScanDelim2(void)
{
    int c,chr2,tok,length;
    unsigned char chrclass;
    c=CurrChar;
    chrclass=TokValue[c-' '];
    tok=chrclass&0x3f;
    chr2=NextChar();
    Buffer[1]=(char)chr2;
    length=1;
    if (chr2=='=') {
        if (chrclass&0x80) {
            NextChar();
            ++tok;
            length=2;
        }
    } else if (chr2==c) {
        if (chrclass&0x40) {
            length=2;
            tok+=2;
            if (NextChar()=='=' && (tok==0x20 || tok==0x1c)) {
                ++tok;
                NextChar();
                Buffer[2]='=';
                length=3;
            }
        }
    } else if (c==':' && chr2=='>') {
        NextChar();
        tok=0x32;
        length=2;
    }
    TokenLen=length;
    Buffer[length]='\0';
    return tok;
}

int IdLookup(void)
{
    MEPTR mentry;
    mentry=MacroLookup();
    if (mentry!=NULL) {
        NextMacro=mentry;
        return T_MACRO;
    }
    return KwLookup();
}

int KwLookup(void)
{
    int token;
    if ((unsigned)KwHashValue>=47) STUB_TRAP();
    token=KwHashValue+54;
    if (strcmp(Tokens[token],Buffer)==0) return token;
    return T_ID;
}

void SkipWhiteSpace(int c)
{
    if (!CompFlags.cpp_output) ScanWhiteSpace();
    else {
        while (CharSet[c]&C_WS) {
            if (c!='\r' && !CompFlags.pre_processing) PrtChar(c);
            c=NextChar();
        }
    }
}

/* cscan.c's hexadecimal accumulator before BadTokenInfo was recorded here. */
int ScanHex(int maximum,int rtn)
{
    int count,c;
    unsigned char chrclass;
    unsigned value;
    int too_big;
    count=maximum;
    value=0;
    too_big=0;
    for (;;) {
        if (rtn==0) c=SaveNextChar();
        else c=(unsigned char)Buffer[++CLitLength];
        if (maximum==0) break;
        chrclass=CharSet[c];
        if (!(chrclass&(C_HX|C_DI))) break;
        if (chrclass&C_HX) c=(c|0x20)-0x27;
        if (value&0xf0000000U) too_big=1;
        value=(value<<4)+(unsigned)c-'0';
        --maximum;
    }
    Constant=value;
    if (count==maximum) return 0;
    if (too_big && NestLevel==SkipLevel) CWarn1(6,0x6a);
    return 1;
}

/* The old scanner accepts // comments unconditionally and retains the last
 * non-CR character to recognize a continued line before CR/LF. */
int ScanSlash(void)
{
    int c,previous,last;
    c=NextChar();
    if (c=='=') {
        NextChar();
        Buffer[1]='=';
        Buffer[2]='\0';
        TokenLen=2;
        return 0x2b;
    }
    if (c=='/') {
        CppComment('/');
        last='/';
        for (;;) {
            previous=CurrChar;
            NextChar();
            if (CurrChar==EOF_CHAR) break;
            if ((CurrChar=='\n' || previous=='\r') && last!='\\') break;
            if (CompFlags.cpp_output && CompFlags.keep_comments && CurrChar!='\r')
                PrtChar(CurrChar);
            if (CurrChar!='\r') last=CurrChar;
        }
        CppComment(0);
        Buffer[0]=' ';
        Buffer[1]='\0';
        return T_WHITE_SPACE;
    }
    if (c=='*') {
        ScanComment();
        Buffer[0]=' ';
        Buffer[1]='\0';
        return T_WHITE_SPACE;
    }
    Buffer[1]='\0';
    return 0x2a;
}

int ScanQuestionMark(void)
{
    Buffer[1]='\0';
    NextChar();
    return 3;
}

int ScanEof(void)
{
    return 1;
}
