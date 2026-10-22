#ifndef RC_DIALOG_H
#define RC_DIALOG_H
#include "menu.h"
#include <recon.h>

/* Scratch objects are Windows WORD-packed templates, not serialized host
 * structs. Serialization still passes through the original writer functions. */
#pragma pack(2)
struct DialogTemplate {
    DWORD style, extended_style;
    WORD controls, x, y, cx, cy;
    WCHAR menu[257], class_name[257], caption[257];
    WORD point_size;
    WCHAR font[257];
    BYTE menu_is_ordinal, class_is_ordinal;
};
struct DialogControl {
    DWORD style, extended_style;
    WORD x, y, cx, cy, command, text_is_ordinal;
    WCHAR class_name[257], text[257];
};
#pragma pack()
#define active_dialog ((struct DialogTemplate *)dialog_scratch)
extern unsigned int dialog_control_count_offset;
extern BOOL control_consumed;
enum ControlToken {
    TOKEN_RADIOBUTTON=40, TOKEN_CHECKBOX=41, TOKEN_PUSHBUTTON=42,
    TOKEN_DEFPUSHBUTTON=43, TOKEN_AUTOCHECKBOX=44, TOKEN_STATE3=45,
    TOKEN_AUTO3STATE=46, TOKEN_USERBUTTON=47, TOKEN_AUTORADIOBUTTON=48,
    TOKEN_GROUPBOX=51, TOKEN_EDITTEXT=60, TOKEN_LTEXT=61, TOKEN_RTEXT=62,
    TOKEN_CTEXT=63, TOKEN_ICON=66, TOKEN_LISTBOX=90, TOKEN_COMBOBOX=91,
    TOKEN_SCROLLBAR=93, TOKEN_GENERIC_CONTROL=134
};
DWORD __stdcall control_style(DWORD);
void __stdcall parse_button_control(struct DialogControl *);
void __stdcall parse_group_control(struct DialogControl *);
void __stdcall parse_static_control(struct DialogControl *);
void __stdcall parse_icon_control(struct DialogControl *);
void __stdcall parse_listbox_control(struct DialogControl *);
void __stdcall parse_scrollbar_control(struct DialogControl *);
void __stdcall parse_combobox_control(struct DialogControl *);
void __stdcall parse_edit_control(struct DialogControl *);
void __stdcall parse_generic_control(struct DialogControl *);
void __stdcall parse_caption_control(struct DialogControl *);
void __stdcall parse_control_text(struct DialogControl *);
void __stdcall parse_control_id(struct DialogControl *);
void __stdcall parse_control_rectangle(struct DialogControl *);
void __stdcall put_dialog_control(struct DialogControl *);
void __cdecl end_dialog_control(void);
void __cdecl increment_dialog_controls(void);
void __stdcall parse_dialog_options(struct ResourceInfo *);
void __stdcall put_dialog_header(struct DialogTemplate *);
void __cdecl parse_dialog_controls(void);
int __stdcall wide_atoi(const WCHAR *);
WCHAR * __stdcall wide_itoa(DWORD, WCHAR *, DWORD);
void __cdecl parse_dialog_font(void);
void __cdecl parse_dialog_menu(void);
void __cdecl parse_dialog_class(void);
void __cdecl parse_dialog_caption(void);
#endif
