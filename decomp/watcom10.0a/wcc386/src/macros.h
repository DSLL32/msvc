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
struct macro_seg_list {
    struct macro_seg_list *next;
    char *segment;
};
extern char *MacroSegment,*MacroOffset,*MacroLimit;
extern struct macro_seg_list *MacSegList;
extern MEPTR *MacHash;
extern char Buffer[BUF_SIZE+16];
void AllocMacroSegment(unsigned minimum);
void MacroAdd(MEPTR entry,char *buffer,int length,unsigned flags);
void TimeInit(void);
extern int MacHashValue;
MEPTR MacroLkUp(char *name,int hash);
int MacroCompare(MEPTR first,MEPTR second);
void MacroOverflow(unsigned amount,unsigned used);
void MacroCopy(const char *source,char *destination,unsigned length);
void MacLkAdd(MEPTR entry,unsigned length,unsigned flags);
#endif
