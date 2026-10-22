/* Optional user-specified ordering of code contributions. */
#include "pass-one.h"
#include "options.h"
#include "link-pass.h"
#include <recon.h>
#pragma intrinsic(strlen)
unsigned char order_enabled;
void reorder_code_contributions(Image *);

void apply_order_file(Image *image)
{
    unsigned char allowed=!debug_enabled(24);
    if (order_enabled && allowed) reorder_code_contributions(image);
}

FatalRoutine order_fatal;
WarningRoutine order_warning;
void initialize_order(FatalRoutine fatal,WarningRoutine warning)
{
    order_fatal=fatal;
    order_warning=warning;
    order_enabled=1;
}
unsigned long read_ordered_contributions(SymbolTable *,Contribution ***,Group *,Image *);

/* Original [00017ab0,00017b60). RVA is a temporary selection mark here. */
void reorder_code_contributions(Image *image)
{
    Group *code=find_section_by_name(".text",&image->sections)->groups;
    Contribution **ordered,*remaining=NULL,*current,*next,*first,**tail;
    unsigned long count=read_ordered_contributions(image->symbols,&ordered,code,image),index;
    if (!order_enabled || !count) return;
    for (current=code->first;current;current=next) {
        next=current->next;
        if (!current->rva) {
            current->next=remaining;
            remaining=current;
        }
    }
    first=NULL;
    tail=&first;
    for (index=0;index<count;++index) {
        current=ordered[index];
        if (current->rva) {
            current->rva=0;
            *tail=current;
            tail=&current->next;
        }
    }
    *tail=remaining;
    code->first=first;
}

/* Original [00017b60,00017de0). Resolve each order-file line to a contribution. */
unsigned long read_ordered_contributions(SymbolTable *symbols,Contribution ***ordered,
                                          Group *code,Image *image)
{
    unsigned long count=0,capacity=64,index,length;
    long last;
    char *buffer,*line,*name,*end;
    FILE *stream;
    External *external;
    Contribution *contribution;
    if (!order_enabled) return 0;
    *ordered=calloc(64,64);
    if (!*ordered) order_fatal(NULL,5,"");
    buffer=malloc(1024);
    if (!buffer) order_fatal(NULL,5,"");
    stream=fopen(order_filename,"rt");
    if (!stream) order_fatal("Linker",7,order_filename);
    line=buffer+1;
    while (fgets(line,261,stream)) {
        last=strlen(line)-1;
        if (line[last]=='\n') line[last]=0;
        if (line[last-1]=='\r') line[last-1]=0;
        end=strchr(line,';');
        if (end) *end=0;
        name=line;
        while (*name==' ' || *name=='\t') ++name;
        end=name;
        while (*end && *end!=' ' && *end!='\t') ++end;
        *end=0;
        length=strlen(name);
        if (!length) continue;
        if (image->file.Machine==0x14c && *name!='?' && *name!='@') {
            *--name='_';
            ++length;
        }
        external=find_external(symbols,(unsigned char)(length<=8),name);
        if (!external || !(external->flags&EXTERNAL_DEFINED)) {
            order_warning(program_name,0x6c,name,"");
            continue;
        }
        contribution=external->owner.contribution;
        if (contribution->group!=code) {
            order_warning(program_name,0x6b,name,"");
            continue;
        }
        contribution->rva=1;
        (*ordered)[count++]=contribution;
        if (count==capacity) {
            *ordered=realloc(*ordered,capacity*8);
            if (!*ordered) order_fatal(NULL,5,"");
            for (index=capacity;index<capacity*2;++index) (*ordered)[index]=NULL;
            capacity*=2;
        }
    }
    fclose(stream);
    free(buffer);
    return count;
}
