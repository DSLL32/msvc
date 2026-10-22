#ifndef RC_MENU_H
#define RC_MENU_H
#include "tokens.h"
#include "resource-data.h"
struct MenuItem {
    WORD command;
    WCHAR text[257];
    BYTE flags, popup;
};
extern unsigned int current_menu_offset;
void __stdcall parse_menu_item(BOOL);
BOOL __stdcall parse_menu_flag(unsigned int, struct MenuItem *);
void __cdecl mark_menu_item(void);
void __cdecl finish_menu(void);
void __stdcall put_menu_item(struct MenuItem *);
void __stdcall combine_word_operators(WORD *);
#endif
