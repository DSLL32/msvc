#ifndef WCC_SCANNER_H
#define WCC_SCANNER_H
#include "compiler.h"
#include "types.h"
#include <recon.h>
/* The 1994 FCB is48 bytes; the later rseekpos member is absent. */
struct fcb_struct {
    char *src_name;
    int src_fno;
    struct fname_list *src_flist;
    int src_line;
    FILE *src_fp;
    FCB *prev_file;
    int prev_currchar,src_cnt;
    char *src_ptr;
    int src_bufsize;
    char *src_buf;
    int peeking;
};
enum { T_NULL=0,T_ID=0x33,T_CONSTANT=0x34,T_STRING=0x35,
       T_MACRO=0x66,T_WHITE_SPACE=0x75,T_BAD_CHAR=0x76,T_LSTRING=0x80,
       T_SWITCH=0x3a,T_CASE=0x57,T_FOR=0x5f,T_FUNC_DEFN=0x67,T_FUNC_END=0x68,
       T_CALL=0x6e,T_CALL_NO_PARM=0x6f,T_LABEL_DEFN=0x70,T_END_FOR=0x72,
       T_TRY=0x81,T_EXCEPT=0x82,T_FINALLY=0x83,T_UNROLL=0x88,T_UNWIND=0x89 };
enum { C_AL=1,C_DI=2,C_HX=4,C_EX=8,C_D1=16,C_D2=32,C_WS=64,C_DB=128,C_BC=0 };
extern unsigned char CharSet[];
/* Encountered operator and token spellings, before the hashed keywords. */
extern char *Tokens[101];
extern char *ScanCharPtr;
extern int CurToken,TokenLen,TokenLine,SrcFileLineNum;
extern char *SavedId;
extern int LAToken;
extern int SrcLineNum,SrcFno;
extern int CLitLength;
extern int BadTokenInfo;
int ESCChar(int character,int rtn);
int GetCharCheck(int character);
int getCharAfterBackSlash(void);
int ReadBuffer(FCB *file);
void CloseSrcFile(FCB *file);
int OpenFCB(FILE *stream,char *name);
int FCB_Alloc(FILE *stream,char *name);
int TryOpen(char *path,char *separator,char *name,char *extension);
int UsePreCompiledHeader(char *name);
int tryBackSlashNewLine(void);
int getTestCharFromFile(void);
unsigned char translateTriGraph(unsigned char character);
int getCharAfterOneQuestion(void);
int getCharAfterTwoQuestion(void);
int tryTrigraphAgain(void);
int IdLookup(void);
extern int KwHashValue,HashValue,SavedHash;
int KwLookup(void);
void SkipAhead(void);
void SkipWhiteSpace(int character);
void ScanComment(void);
void CppComment(int character);
void UnGetChar(int character);
int ScanName(void);
int ScanWide(void);
int ScanNum(void);
int ScanQuestionMark(void);
int ScanSlash(void);
int ScanMinus(void);
int ScanDelim1(void);
int ScanDelim2(void);
int ScanDot(void);
int ScanString(void);
int ScanStringContinue(void);
int ScanCharConst(void);
int CharConst(DATA_TYPE type);
int ScanNewline(void);
int TBreak(void);
void ChkControl(void);
void PreProcStmt(void);
int PPNextToken(void);
void Flush2EOL(void);
int ScanWhiteSpace(void);
int ScanInvalid(void);
int ScanMacroToken(void);
int ScanEof(void);

void ReScanInit(char *text);
char *ReScanPos(void);
int ReScanToken(void);
int ReScanBuffer(void);
int ScanToken(void);
int SaveConstantToken(int index);
int SaveNextChar(void);
int ScanHex(int max,int rtn);
extern unsigned Constant;
extern DATA_TYPE ConstType;
#endif
