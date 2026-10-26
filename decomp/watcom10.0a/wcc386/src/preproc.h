#ifndef WCC_PREPROC_H
#define WCC_PREPROC_H
#include <recon.h>
struct macro_parm {
    struct macro_parm *next_macro_parm;
    char *parm;
};
int FormalParm(struct macro_parm *formal_parms);
void GrabTokens(int parm_count,struct macro_parm *formal_parms,int defn_offset);
void MustRecog(unsigned char token);
int GetConstExpr(void);
void ChkEOL(void);
void WantEOL(void);
void IncLevel(int value);
void ExpectIdentifier(void);
void ExpectMessage(int message);
void CDefine(void);
void CInclude(void);
void CIfDef(void);
void CIfNDef(void);
void CIf(void);
void CElif(void);
void CElse(void);
void CEndif(void);
void CUndef(void);
void CLine(void);
void CError(void);
void CPragma(void);
void CIdent(void);
void CUnknown(void);
void CSkip(void);
#endif
