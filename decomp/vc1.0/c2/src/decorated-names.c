/* Compiler name-decoding support, including its original small-POD ABI. */
#include <string.h>
#include "decorated-names.h"
#include <recon.h>
static char undecorated_name[246]; /* original7e978..7ea6e, shared result identity */

/* Original6f620..6f648, ECX name/RET0. Failed decoding exposes an empty
   static string; the existing formatter then retains the original spelling. */
char *__fastcall undecorate_function_name(char *name)
{
    if (!decode_decorated_name(undecorated_name,name,sizeof undecorated_name).next)
        undecorated_name[0]=0;
    return undecorated_name;
}

/* Original853e0..853f0: failure POD and the two decoder-state DWORDs.
   Original7e928..7e978: ten source slices used by decimal back references. */
NameBuffer invalid_name_buffer;
NameSlice decorated_tokens[10];
int decorated_last_token=-1,decorated_scope_mode;
/* Original85338..853dc: all41 operator spellings, addressed only after
   digits2..9, lettersA..Z or underscore-digits0..6 have selected a valid row. */
const char *decorated_operator_names[41] = {
    " new", " delete", "=", ">>", "<<", "!", "==", "!=",
    "[]", "<UDC>", "->", "*", "++", "--", "-", "+", "&", "->*",
    "/", "%", "<", "<=", ">", ">=", ",", "()", "~", "^", "|",
    "&&", "||", "*=", "+=", "-=", "/=", "%=", ">>=", "<<=", "&=", "|=", "^="
};

/* Original6f650..6fa47, fastcall ECX destination/EDX source/stack capacity,
   EAX:EDX NameBuffer/RET4. Outer names collect cached components; recursive
   scope mode writes qualified components from inside out. Signed lengths,
   truncation stores and early malformed-name returns follow the original. */
NameBuffer __fastcall decode_decorated_name(char *destination,char *source,int capacity)
{
    NameBuffer result;
    NameSlice name;
    const char *operator_name;
    int operator_index=-1,destructor=0,copy_count,index,length;
    result.next=destination;
    result.remaining=capacity;
    if (decorated_scope_mode) {
        if (*source=='?') return result;
        name=scan_decorated_token(&source);
        if (!name.text) return invalid_name_buffer;
        if (!*source) return result;
        if (*source!='@') result=decode_decorated_name(destination,source,capacity);
        if (!result.next) return invalid_name_buffer;
        copy_count=name.length<result.remaining?name.length:result.remaining;
        strncpy(result.next,name.text,copy_count);
        if (name.length<result.remaining) {
            result.next+=name.length;
            result.remaining-=name.length;
            for (index=0;index<2 && result.remaining;index++) {
                *result.next++=':';
                --result.remaining;
            }
        } else {
            result.next[result.remaining-1]=0;
            result.next+=result.remaining;
            result.remaining=0;
        }
        return result;
    }
    decorated_last_token=-1;
    if (*source++!='?') return invalid_name_buffer;
    if (*source=='?') {
        ++source;
        if (!*source) return invalid_name_buffer;
        if (*source=='_') {
            ++source;
            if (!*source || *source<'0' || *source>'6') return invalid_name_buffer;
            operator_index=34+(*source-'0');
        } else if (*source=='0') {
            /* Constructor: use the first decoded class component. */
        } else if (*source=='1') destructor=1;
        else if (*source>='2' && *source<='9') operator_index=*source-'2';
        else if (*source>='A' && *source<='Z') operator_index=8+(*source-'A');
        else return invalid_name_buffer;
        ++source;
    } else {
        name=scan_decorated_token(&source);
        if (!name.text) return invalid_name_buffer;
    }
    if (!*source) return invalid_name_buffer;
    if (*source!='@') {
        decorated_scope_mode=1;
        result=decode_decorated_name(destination,source,capacity);
        decorated_scope_mode=0;
        if (!result.next) return invalid_name_buffer;
    }
    if (!result.remaining) {
        *result.next=0;
        goto complete;
    }
    if (operator_index>=0) {
        strncpy(result.next,"operator",result.remaining);
        if (result.remaining>8) {
            result.remaining-=8;
            result.next+=8;
            operator_name=decorated_operator_names[operator_index];
            strncpy(result.next,operator_name,result.remaining);
            for (length=0;operator_name[length];++length) ;
            if (length<result.remaining) goto complete;
        }
        result.next[result.remaining-1]=0;
        goto complete;
    }
    if (destructor && result.remaining>0) {
        *result.next++='~';
        --result.remaining;
    }
    if (decorated_last_token<0) return invalid_name_buffer;
    copy_count=decorated_tokens[0].length<result.remaining?
        decorated_tokens[0].length:result.remaining;
    strncpy(result.next,decorated_tokens[0].text,copy_count);
    if (decorated_tokens[0].length<result.remaining)
        result.next[decorated_tokens[0].length]=0;
    else result.next[result.remaining-1]=0;
complete:
    result.next=destination;
    result.remaining=capacity;
    return result;
}

/* Pending original6fa50, ECX source-pointer owner, EAX:EDX slice/RET0. */
NameSlice __fastcall scan_decorated_token(char **source)
{
    __asm { int 3 }
}
