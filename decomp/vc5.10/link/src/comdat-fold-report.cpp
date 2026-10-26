/* Verbose folded-COMDAT groups retain a temporary copy of the candidate array. */
#include <windows.h>
#include <new.h>
#include <stdio.h>
#include "../crt/crt-imports.h"
#include <string.h>
#include "link-driver.h"
#include "full-link-functions.h"
#include "symbol-buffer.h"
#include "initializers.h"
#include <recon.h>

RECON_RETAIN inline ScopedBuffer::~ScopedBuffer()
{
    if(data) operator delete(data);
}
RECON_FORCEINLINE ScopedBuffer::ScopedBuffer(unsigned elements)
{
    data=elements?operator new(elements*4):NULL;
    used=capacity=data?elements:0;
}
extern "C" {
extern char short_symbol_name[9];
void __cdecl link_message(unsigned,...);
char *__stdcall display_symbol_name(const char *,DWORD);
char *__stdcall format_contribution_module(Contribution *,char *);
StringNode *__stdcall find_contribution_symbol(LinkContext *,Contribution *);
}
#pragma intrinsic(memcpy)
void RECON_THISCALL LinkContext::PrintFoldedComdats(void)
{
    DWORD count=buffer4.remaining,total=0,index,other,message;
    ContributionInfo **candidates,*info;
    Contribution *canonical,*contribution,*target;
    StringNode *symbol;
    const char *name;
    char *display;
    WORD kind;
    char module_name[520];
    if(count) {
        ScopedBuffer copy(count);
        memcpy(copy.data,buffer4.base,count*4);
        candidates=(ContributionInfo **)copy.data;
        for(index=0;index<count;++index) {
            info=candidates[index];
            if(!info || !info->fold_candidate ||
               !(info->fold_candidate->characteristics&0x4000))continue;
            canonical=info->fold_candidate;
            while(canonical && (canonical->characteristics&0x4000))
                canonical=canonical->associated_contribution;
            symbol=find_contribution_symbol(this,canonical);
            if(symbol) {
                name=get_symbol_name(symbol,symbols->hash.string_storage,&kind,NULL);
                if(kind==1)name=strncpy(short_symbol_name,name,8);
                display=display_symbol_name(name,1);
                message=9029;
            } else {
                name=canonical->group->name;
                display=(char *)name;
                message=9030;
            }
            fputs("\n",stdout);
            link_message(9026);
            link_message(message,display,format_contribution_module(canonical,module_name));
            if(display!=name)release_memory(display);
            link_message(9027);
            for(other=index;other<count;++other) {
                info=candidates[other];
                contribution=info->fold_candidate;
                if(!contribution || !(contribution->characteristics&0x4000))continue;
                target=contribution;
                while(target && (target->characteristics&0x4000))
                    target=target->associated_contribution;
                if(target!=canonical)continue;
                total+=contribution->raw_size-contribution->padding;
                symbol=find_contribution_symbol(this,contribution);
                if(symbol) {
                    name=get_symbol_name(symbol,symbols->hash.string_storage,&kind,NULL);
                    if(kind==1)name=strncpy(short_symbol_name,name,8);
                    display=display_symbol_name(name,1);
                    message=9029;
                } else {
                    name=contribution->group->name;
                    display=(char *)name;
                    message=9030;
                }
                link_message(message,display,format_contribution_module(contribution,module_name));
                if(display!=name)release_memory(display);
                info->fold_candidate=NULL;
            }
        }
    }
    if(total) {
        fputs("\n",stdout);
        link_message(9028,total);
        fputs("\n",stdout);
    }
}
