/* LINK's display wrapper and embedded C++ name formatter. */
#include "demangle.h"
DemangleHeap demangle_heap;
DemangleReplicator *demangle_type_replicator,*demangle_name_replicator;
const char *demangle_original,*demangle_input;
char *demangle_output;
int demangle_capacity;
unsigned short demangle_flags;

#pragma intrinsic(strlen,memcpy)
/* Original [0003d740,0003d8b0). */
char *undecorate_symbol_name(const char *name,unsigned char decorated)
{
    const char *input=name;
    char *display;
    unsigned imported=!strncmp(name,"__imp_",6),prefix,length;
    if (imported) input+=6;
    if (*input!='?') return (char *)name;
    display=demangle_symbol(NULL,input,0,malloc,free,0x800);
    if (!display) return (char *)name;
    if (imported) {
        display=realloc(display,strlen(display)+1+22);
        if (!display) out_of_memory();
        memmove(display+22,display,strlen(display)+1);
        memcpy(display,"__declspec(dllimport) ",22);
    }
    prefix=decorated?strlen(name)+1:0;
    length=strlen(display);
    display=realloc(display,prefix+length+3);
    if (!display) out_of_memory();
    memmove(display+prefix+1,display,length);
    if (decorated) {
        memcpy(display,name,prefix-1);
        display[prefix-1]=' ';
    }
    display[prefix]='(';
    display[prefix+length+1]=')';
    display[prefix+length+2]=0;
    return display;
}
#pragma function(strlen,memcpy)

/* Public undecorator [0003d8b0,0003dae0). */
char *demangle_symbol(char *buffer,const char *name,int capacity,
                      DemangleAllocate allocate,DemangleRelease release,unsigned short flags)
{
    DemangleReplicator names,arguments;
    DemangleName result,display,parsed,prefixed;
    DemangleName *value;
    DemangleBlock *block;
    char *output;
    if (!allocate || !release) return NULL;
    demangle_heap.allocate=allocate;
    demangle_heap.release=release;
    demangle_heap.remaining=0;
    demangle_heap.head=demangle_heap.tail=NULL;
    demangle_name_status(&names.error,NULL,3);
    demangle_name_status(&names.empty,NULL,1);
    names.index=-1;
    demangle_name_status(&arguments.error,NULL,3);
    demangle_name_status(&arguments.empty,NULL,1);
    arguments.index=-1;
    demangle_original=name;
    demangle_input=name;
    demangle_capacity=capacity;
    demangle_output=buffer;
    demangle_name_replicator=&arguments;
    demangle_type_replicator=&names;
    demangle_flags=flags;
    demangle_name_empty(&result,NULL);
    demangle_name_empty(&display,NULL);
    if (demangle_original) {
        if (demangle_original[0]=='?' && demangle_original[1]=='@') {
            demangle_input+=2;
            value=demangle_parse_declaration(&parsed);
            value=demangle_prepend_text(&prefixed,"CV: ",value);
        } else value=demangle_parse_declaration(&parsed);
        demangle_name_assign(&result,NULL,value);
    }
    if ((result.flags&15)==3) output=NULL;
    else {
        if (!*demangle_input && (result.flags&15)!=1)
            demangle_name_assign(&display,NULL,&result);
        else demangle_name_assign_text(&display,NULL,demangle_original);
        if (!demangle_output) {
            demangle_capacity=demangle_name_length(&display,NULL)+1;
            demangle_output=demangle_allocate(&demangle_heap,NULL,demangle_capacity,1);
        }
        output=demangle_output;
        if (output) demangle_name_write(&display,NULL,output,demangle_capacity);
    }
    while ((block=demangle_heap.head)!=NULL) {
        demangle_heap.head=block->next;
        demangle_heap.release(block);
    }
    demangle_heap.head=demangle_heap.tail=NULL;
    return output;
}

/* Native thiscall [00040150,000401c0); EDX is reserved only for C fastcall. */
DemangleName *__fastcall demangle_name_status(DemangleName *self,void *unused,int status)
{
    DemangleStatusNode *node;
    self->flags=(self->flags&~15UL)|((status==1 || status==3)?status:0);
    node=demangle_allocate(&demangle_heap,NULL,sizeof(*node),0);
    self->head=node?(DemangleNode *)demangle_status_node(node,NULL,status):NULL;
    self->flags&=~0x30UL;
    if (!self->head) self->flags=(self->flags&~15UL)|3;
    return self;
}

/* Native thiscall [0003fd20,0003fdd0); RET8. */
void *__fastcall demangle_allocate(DemangleHeap *self,void *unused,unsigned size,int standalone)
{
    DemangleBlock *block;
    size=(size+2)&~2U;
    if (standalone) return self->allocate(size);
    if (!size) size=1;
    if (size>self->remaining) {
        if (size>508) return NULL;
        block=demangle_allocate(&demangle_heap,NULL,512,1);
        if (block) block->next=NULL;
        if (!block) return NULL;
        if (self->tail) {
            self->tail->next=block;
            self->tail=self->tail->next;
        } else self->head=self->tail=block;
        self->remaining=508;
    }
    self->remaining-=size;
    return self->tail->bytes+self->remaining;
}

/* Native thiscall [00040dc0,00040df0); RET4. */
DemangleStatusNode *__fastcall demangle_status_node(DemangleStatusNode *self,void *unused,int status)
{
    demangle_node_initialize(&self->base,NULL,3);
    self->status=status;
    self->length=status==2?4:0;
    return self;
}

/* Native thiscall [00040ae0,00040b00); RET4. */
DemangleNode *__fastcall demangle_node_initialize(DemangleNode *self,void *unused,int kind)
{
    self->kind=kind;
    self->next=NULL;
    return self;
}

/* Native thiscall [0003fe60,0003fe80); no stack arguments. */
DemangleName *__fastcall demangle_name_empty(DemangleName *self,void *unused)
{
    self->head=NULL;
    self->flags&=~0x3fUL;
    return self;
}

/* Parse a decorated declaration [0003dae0,0003dc60). */
DemangleName *demangle_parse_declaration(DemangleName *result)
{
    DemangleName name,scope,separator,qualified;
    DemangleName *value;
    int conversion;
    if (*demangle_input!='?')
        return demangle_name_status(result,NULL,*demangle_input?1:2);
    ++demangle_input;
    if (*demangle_input=='?') {
        ++demangle_input;
        demangle_parse_operator(&name);
    } else demangle_parse_name(&name);
    conversion=!demangle_name_is_empty(&name,NULL) && (name.flags&0x20);
    if (!demangle_name_is_valid(&name,NULL)) return demangle_name_copy(result,NULL,&name);
    if (*demangle_input && *demangle_input!='@') {
        value=demangle_parse_scope(&scope);
        value=demangle_name_plus_text(value,NULL,&separator,"::");
        value=demangle_name_plus_name(value,NULL,&qualified,&name);
        demangle_name_assign(&name,NULL,value);
    }
    if (conversion && !demangle_name_is_empty(&name,NULL)) name.flags|=0x20;
    if (demangle_name_is_empty(&name,NULL)) return demangle_name_copy(result,NULL,&name);
    if (*demangle_input) {
        if (*demangle_input!='@') return demangle_name_status(result,NULL,1);
        ++demangle_input;
    }
    return demangle_parse_type_declaration(result,&name);
}

/* Parse an identifier or numeric name backreference [0003dc60,0003dcf0). */
DemangleName *demangle_parse_name(DemangleName *result)
{
    int index=(signed char)*demangle_input-'0';
    DemangleName name,*value;
    if (index>=0 && index<=9) {
        ++demangle_input;
        value=demangle_replicator_get(demangle_name_replicator,NULL,index);
        return demangle_name_copy(result,NULL,value);
    }
    demangle_name_read_until(&name,NULL,&demangle_input,'@');
    if (demangle_name_replicator->index!=9)
        demangle_replicator_add(demangle_name_replicator,NULL,&name);
    return demangle_name_copy(result,NULL,&name);
}

/* Native thiscall [0003ffc0,000400e0); RET8. */
DemangleName *__fastcall demangle_name_read_until(DemangleName *self,void *unused,
                                                   const char **input,int terminator)
{
    const char *start=*input;
    unsigned length=0;
    signed char character;
    self->flags&=~0x3fUL;
    self->head=NULL;
    if (!start) self->flags=(self->flags&~15UL)|1;
    else if (!*start) self->flags=(self->flags&~15UL)|2;
    else {
        while ((character=(signed char)**input)!=0) {
            if (character==(signed char)terminator) break;
            if (character!='_' && character!='$' &&
                !(character>='a' && character<='z') &&
                !(character>='A' && character<='Z') &&
                !(character>='0' && character<='9')) {
                self->flags=(self->flags&~15UL)|1;
                return self;
            }
            ++length;
            ++*input;
        }
        demangle_name_set_range(self,NULL,start,length);
        if (**input) {
            character=(signed char)*(*input)++;
            if (character==(signed char)terminator) self->flags&=~15UL;
            else {
                self->flags=(self->flags&~15UL)|3;
                self->head=NULL;
            }
        } else if (!(self->flags&15)) self->flags=(self->flags&~15UL)|2;
    }
    return self;
}

/* Native thiscall [00040980,00040a60); RET8. */
void __fastcall demangle_name_set_range(DemangleName *self,void *unused,const char *text,int length)
{
    DemangleTextNode *string;
    DemangleCharNode *character;
    if ((self->flags&15)==1 || (self->flags&15)==3) return;
    if (self->head) {
        demangle_name_assign_status(self,NULL,3);
        return;
    }
    if (!text || !length) {
        self->flags=(self->flags&~15UL)|1;
        return;
    }
    if (length==1) {
        character=demangle_allocate(&demangle_heap,NULL,sizeof(*character),0);
        self->head=character?(DemangleNode *)demangle_char_node(character,NULL,*text):NULL;
    } else {
        string=demangle_allocate(&demangle_heap,NULL,sizeof(*string),0);
        self->head=string?(DemangleNode *)demangle_text_node(string,NULL,text,length):NULL;
    }
    if (!self->head) self->flags=(self->flags&~15UL)|3;
}

/* Native thiscall [00040c90,00040d10); RET8. */
DemangleTextNode *__fastcall demangle_text_node(DemangleTextNode *self,void *unused,
                                                const char *text,int length)
{
    demangle_node_initialize(&self->base,NULL,1);
    if (!length && text) length=demangle_text_length(text);
    if (length && text) {
        self->text=demangle_allocate(&demangle_heap,NULL,length,0);
        self->length=length;
        if (self->text) demangle_copy_text(self->text,text,length);
    } else {
        self->text=NULL;
        self->length=0;
    }
    return self;
}

/* Original [00040e50,00040e80). */
char *demangle_copy_text(char *output,const char *input,unsigned length)
{
    char *destination=output;
    while (length) {
        *destination=*input;
        if (!*destination) break;
        ++destination;
        ++input;
        --length;
    }
    return output;
}

/* Native thiscall [00040a60,00040ab0); RET4. */
DemangleReplicator *__fastcall demangle_replicator_add(DemangleReplicator *self,
                                                       void *unused,const DemangleName *name)
{
    DemangleName *copy;
    if (self->index!=9 && !demangle_name_is_empty(name,NULL)) {
        copy=demangle_allocate(&demangle_heap,NULL,sizeof(*copy),0);
        if (copy) copy=demangle_name_copy(copy,NULL,name);
        if (copy) self->values[++self->index]=copy;
    }
    return self;
}

/* Native thiscall [000401e0,00040200); no stack arguments. */
int __fastcall demangle_name_is_empty(const DemangleName *self,void *unused)
{
    return !self->head || !demangle_name_is_valid(self,NULL);
}

/* Native thiscall [000401c0,000401e0); no stack arguments. */
int __fastcall demangle_name_is_valid(const DemangleName *self,void *unused)
{
    unsigned status=self->flags&15;
    return status==0 || status==2;
}

/* Native thiscall [0003fec0,0003ff10); RET4. */
DemangleName *__fastcall demangle_name_copy(DemangleName *self,void *unused,
                                            const DemangleName *source)
{
    self->flags=(self->flags&~0x3fUL)|(source->flags&0x3f);
    self->head=source->head;
    return self;
}
