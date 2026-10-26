#ifndef WPP_SCAN_H
#define WPP_SCAN_H
#include <recon.h>
extern int CurrChar, CurToken, LAToken, TokenLen;
int ScanToken(int expanding);
extern unsigned long Constant;
extern unsigned ConstType;
extern char *SavedId;
/* Separate original bd508 diagnostic slot; constant type is bd4fc. */
extern unsigned BadTokenInfo;
extern int TokenSavedLen;
extern char *Tokens[164];
int saveNextChar(void);
void nextMacroToken(void);
void SrcFileGuardStateSig(void);
extern unsigned char PPState;
extern unsigned TokenLine, TokenColumn;
void SrcFileCurrentLocation(void);
void SrcFileLineLocation(void);
int TBreak(void);
void ChkControl(void);
extern int (*NextChar)(void);
int GetNextChar(void);
int SrcFileCmdLnGetChar(void);
void SrcFileCmdLnDummyOpen(void);
void SrcFileCmdLnDummyClose(void);
typedef void (*TOKEN_SOURCE)(void);
TOKEN_SOURCE SetTokenSource(TOKEN_SOURCE source);
void ResetTokenSource(TOKEN_SOURCE source);
#endif
