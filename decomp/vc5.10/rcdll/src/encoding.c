#include <windows.h>
#include "command.h"
#include <recon.h>

/* RCDLL carries its own text test rather than importing IsTextUnicode. */
int __stdcall rc_is_text_unicode(const void *sample,unsigned bytes,unsigned *flags)
{
    const WCHAR *text=sample;
    const BYTE *raw=sample;
    CPINFO info;
    unsigned words=(bytes&~1u)>512?256:bytes/2;
    unsigned i,value,low=bytes,high=bytes,previous_low=0,previous_high=0;
    unsigned low_changes=0,high_changes=0,zero_bytes=0,byte_controls=0;
    unsigned controls=0,reverse_controls=0,illegal=0,lead_bytes=0;
    unsigned examined=bytes<513?bytes:512,result=0,factor=3;

    if((int)bytes<2){
        if(flags) *flags=5;
        return 0;
    }
    for(i=0;i<words;++i){
        value=text[i];
        switch(value){
        case 9: case 10: case 13: case 0x20: case 0x3000:
            ++controls;break;
        case 0x900: case 0xa00: case 0xd00: case 0x2000:
            ++reverse_controls;break;
        case 0: case 0xa0d: case 0xfffe: case 0xffff:
            ++illegal;break;
        }
        low=value&255;high=value>>8;
        if((low==13&&previous_high==10)||(low==10&&previous_high==13))
            ++byte_controls;
        zero_bytes+=(low==0)+(high==0);
        high_changes+=high>previous_high?high-previous_high:previous_high-high;
        low_changes+=low>previous_low?low-previous_low:previous_low-low;
        previous_low=low;previous_high=high;
    }
    if((low==13&&previous_high==10)||(low==10&&previous_high==13))
        ++byte_controls;
    if(high==0) --zero_bytes;
    if(high==0x1a) ++byte_controls;
    GetCPInfo(0,&info);
    if(info.MaxCharSize!=1){
        for(i=0;i<examined;++i){
            if(IsDBCSLeadByteEx(rc_codepage,raw[i])){++lead_bytes;++i;}
        }
    }
    if(low_changes<127&&!high_changes) result=1;
    if(high_changes&&!low_changes) result|=0x10;
    if(!info.MaxCharSize&&lead_bytes&&flags&&(*flags&0x400)){
        if(lead_bytes<((words>>1)-1)/3) factor=3;
        else factor=(lead_bytes<((words&~1u)-2)/3)+1;
        result|=0x400;
    }
    if(high_changes*factor<low_changes) result|=2;
    if(low_changes*factor<high_changes) result|=0x20;
    if(controls) result|=4;
    if(reverse_controls) result|=0x40;
    if(illegal||(byte_controls&&examined/40<=byte_controls)) result|=0x100;
    if(bytes&1) result|=0x200;
    if(zero_bytes) result|=0x1000;
    if(*text==0xfeff) result|=8;
    else if(*text==0xfffe) result|=0x80;
    if(flags){result&=*flags;*flags=result;}
    return ((result&8)&&!(result&0xb00))||
           (!(result&0xf0)&&!(result&0xf00)&&((result&0xf000)||(result&15)));
}
