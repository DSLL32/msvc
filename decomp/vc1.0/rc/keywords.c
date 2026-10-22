/* Original 0x010094b0..0x010094f0: sorted resource-language vocabulary.
 * These are lexical token values, not resource type ordinals. The spelling
 * MESAGETABLE and the shared memory-flag values belong to this RC version. */
#include "tokens.h"
#include <string.h>

WORD __stdcall lookup_keyword(char *text)
{
    static const struct Keyword { char *text; WORD token; } keywords[] = {
        {"ALT",110}, {"ASCII",111}, {"AUTO3STATE",46},
        {"AUTOCHECKBOX",44}, {"AUTORADIOBUTTON",48}, {"BEGIN",13},
        {"BITMAP",67}, {"BLOCK",114}, {"BUTTON",95}, {"CAPTION",135},
        {"CHARACTERISTICS",130}, {"CHECKBOX",41}, {"CHECKED",73},
        {"CLASS",100}, {"COMBOBOX",91}, {"CONTROL",134}, {"CTEXT",63},
        {"DEFPUSHBUTTON",43}, {"DISCARDABLE",65522}, {"DLGINCLUDE",136},
        {"EDIT",64}, {"EDITTEXT",60}, {"END",14}, {"EXSTYLE",65527},
        {"FILEFLAGS",123}, {"FILEFLAGSMASK",122}, {"FILEOS",124},
        {"FILESUBTYPE",126}, {"FILETYPE",125}, {"FILEVERSION",120},
        {"FIXED",65520}, {"FONT",94}, {"GRAYED",74}, {"GROUPBOX",51},
        {"HELP",79}, {"ICON",66}, {"IMPURE",65526}, {"INACTIVE",75},
        {"LANGUAGE",131}, {"LISTBOX",90}, {"LOADONCALL",65523},
        {"LTEXT",61}, {"MENU",70}, {"MENUBARBREAK",76}, {"MENUBREAK",77},
        {"MENUITEM",71}, {"MESAGETABLE",96}, {"MOVEABLE",65521},
        {"NOINVERT",103}, {"NONSHARED",65526}, {"NOT",104}, {"OWNERDRAW",50},
        {"POPUP",78}, {"PRELOAD",65524}, {"PRODUCTVERSION",121},
        {"PURE",65525}, {"PUSHBUTTON",42}, {"RADIOBUTTON",40},
        {"RCDATA",92}, {"RTEXT",62}, {"SCROLLBAR",93}, {"SEPARATOR",72},
        {"SHARED",65525}, {"SHIFT",106}, {"STATE3",45}, {"STATIC",65},
        {"STYLE",133}, {"USERBUTTON",47}, {"VALUE",113}, {"VERSION",132},
        {"VIRTKEY",112}, {NULL,0}
    };
    const struct Keyword *keyword;
    int order;
    for (keyword = keywords; keyword->text; ++keyword) {
        order = _strcmpi(text, keyword->text);
        if (!order)
            return keyword->token;
        if (order < 0)
            break;
    }
    return 0xffff;
}
