#ifndef WPP_MACRO_H
#define WPP_MACRO_H
#include <recon.h>
#pragma pack(1)
typedef struct macro_entry {
    struct macro_entry *next_macro;
    unsigned short macro_defn, macro_len;
    unsigned char parm_count, macro_flags;
    char macro_name[1];
} MEDEFN, *MEPTR;
#pragma pack()
void DirectiveInit(void);
void MacroStorageInit(void);
void macroAllocSegment(unsigned minimum);
void *macroAllocateInSeg(unsigned size);
void MacroDefine(MEPTR entry, char *definition, unsigned length);
void MacroOverflow(unsigned amount_needed, unsigned amount_used);
void macroCopy(const void *source, void *dest, unsigned length);
MEPTR macroAdd(MEPTR entry, unsigned length);
int magicPredefined(char *name);
MEPTR MacroLookup(char *name);
unsigned macroHash(const char *name);
int macroCompare(MEPTR first, MEPTR second);
void TimeInit(void);
extern unsigned char InitialMacroFlag;
extern char Buffer[];
#endif
