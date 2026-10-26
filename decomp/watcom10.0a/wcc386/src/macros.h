#ifndef WCC_MACROS_H
#define WCC_MACROS_H
#include <recon.h>
#define MACRO_HASH_SIZE 4093
#define BUF_SIZE 512
typedef struct macro_entry {
    struct macro_entry *next_macro;
    unsigned short macro_defn,macro_len;
    unsigned char parm_count,macro_flags;
    char macro_name[1];
} MEDEFN,*MEPTR;
/* Earlier 14-byte conditional frame uses byte kind/processing fields. */
#pragma pack(1)
struct cpp_info {
    struct cpp_info *prev_cpp;
    char *file_name;
    unsigned line_num;
    unsigned char cpp_type,processing;
};
#pragma pack()
extern struct cpp_info *CppStack;
struct macro_seg_list {
    struct macro_seg_list *next;
    char *segment;
};
extern char *MacroSegment,*MacroOffset,*MacroLimit,*MacroPtr;
extern struct macro_seg_list *MacSegList;
extern MEPTR *MacHash;
extern char Buffer[BUF_SIZE+16],TokenBuf[BUF_SIZE];
void AllocMacroSegment(unsigned minimum);
void MacroAdd(MEPTR entry,char *buffer,int length,unsigned flags);
void TimeInit(void);
void PreDefine_Macro(char *text);
void Define_Extensions(void);
char *Define_Macro(char *text);
char *Def_Macro_Tokens(char *text,int multiple_tokens,int flags);
extern int MacHashValue,NestLevel,SkipLevel;
extern unsigned char InitialMacroFlag;
extern unsigned MacroSize;
extern MEPTR NextMacro;
int SpecialMacro(MEPTR entry);
#pragma pack(1)
struct macro_token {
    struct macro_token *next;
    unsigned char token;
    char data[1];
};
struct macro_arg { char *arg; };
struct tokens {
    struct tokens *next;
    int length;
    char buf[1];
};
struct nested_macros {
    struct nested_macros *next;
    MEPTR mentry;
    struct macro_arg *macro_parms;
    unsigned char rescanning,substituting_parms;
};
#pragma pack()
extern struct nested_macros *NestedMacros;
int NextMToken(void);
void SaveParm(MEPTR mentry,int length,int parm_count,struct macro_arg *parms,
              struct tokens *token_list,int total);
extern int MTokenLen;
struct macro_arg *CollectParms(void);
int TokLength(char *text);
struct macro_token *BuildAToken(char *text);
int BuildString(int length,char *tokens);
struct macro_token *Glue2Tokens(struct macro_token *first,struct macro_token *second);
struct macro_token *GlueTokens(struct macro_token *head);
struct macro_token *BuildMTokenList(char *tokens,struct macro_arg *parms);
struct macro_token *AppendToken(struct macro_token *head,int token,char *text);
extern struct macro_token *TokenList;
extern int MacroDepth;
struct macro_token *MacroExpansion(int rescanning);
int MacroBeingExpanded(MEPTR mentry);
int Expandable(struct macro_token *mtok,int macro_parm);
void DeleteNestedMacro(void);
struct macro_token *ExpandNestedMacros(struct macro_token *head,int rescanning);
struct macro_token *NestedMacroExpansion(void);
void DoMacroExpansion(void);
void GetMacroToken(void);
MEPTR MacroLookup(void);
MEPTR MacroLkUp(char *name,int hash);
int MacroCompare(MEPTR first,MEPTR second);
void MacroOverflow(unsigned amount,unsigned used);
void MacroCopy(const char *source,char *destination,unsigned length);
void MacLkAdd(MEPTR entry,unsigned length,unsigned flags);
#endif
