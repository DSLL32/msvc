/* Split name[:[key=]value,...], original [0003a580,0003a710). */
#include "options.h"
ParsedSwitch *parse_switch(char *text)
{
    char *colon = strchr(text, ':'), *scan, *copy, *value, *next, *equals;
    unsigned short count=0, index=0;
    unsigned length=0, i;
    ParsedSwitch *result;
    if (colon) {
        count=1; scan=colon;
        while ((scan=strchr(scan+1,',')) != NULL) ++count;
    }
    while (text[length]) ++length;
    result=malloc((unsigned)count*8+length+9);
    if (!result) out_of_memory();
    copy=(char *)result+8+(unsigned)count*8;
    for (i=0;i<=length;++i) copy[i]=text[i];
    if (colon) { value=copy+(colon-text); *value++=0; }
    else value=copy+length;
    if (!*copy) fatal_message(program_name,20,text);
    result->name=copy;
    while (*value) {
        next=strchr(value,',');
        if (next) *next++=0;
        else { next=value; while (*next) ++next; }
        equals=strchr(value,'=');
        if (equals) {
            *equals=0;
            result->values[index].key=value;
            value=equals+1;
        } else result->values[index].key=NULL;
        result->values[index].value=value;
        value=next; ++index;
    }
    result->count=index;
    return result;
}
