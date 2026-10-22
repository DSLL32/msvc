/* LINK's display wrapper and embedded C++ name formatter. */
#include "demangle.h"
#include <recon.h>
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
    DemangleReplicator types,names;
    DemangleName result,display,parsed,prefixed;
    DemangleName *value;
    DemangleBlock *block;
    char *output;
    if (!allocate || !release) return NULL;
    demangle_heap.allocate=allocate;
    demangle_heap.release=release;
    demangle_heap.remaining=0;
    demangle_heap.head=demangle_heap.tail=NULL;
    demangle_name_status(&types.error,NULL,3);
    demangle_name_status(&types.missing,NULL,1);
    types.index=-1;
    demangle_name_status(&names.error,NULL,3);
    demangle_name_status(&names.missing,NULL,1);
    names.index=-1;
    demangle_original=name;
    demangle_input=name;
    demangle_capacity=capacity;
    demangle_output=buffer;
    demangle_name_replicator=&names;
    demangle_type_replicator=&types;
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
    demangle_node_initialize(&self->base,NULL,DEMANGLE_STATUS);
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
    demangle_node_initialize(&self->base,NULL,DEMANGLE_TEXT);
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

/* Parse enclosing scopes [0003dcf0,0003df10). */
DemangleName *demangle_parse_scope(DemangleName *result)
{
    DemangleName scope,separator,component,qualified,quoted,closed,truncated;
    DemangleName *value;
    int first=1;
    demangle_name_empty(&scope,NULL);
    while (!(scope.flags&15) && *demangle_input && *demangle_input!='@') {
        if (first) first=0;
        else {
            value=demangle_prepend_text(&separator,"::",&scope);
            demangle_name_assign(&scope,NULL,value);
        }
        if (*demangle_input=='?') {
            ++demangle_input;
            if (*demangle_input=='$') {
                ++demangle_input;
                value=demangle_parse_template(&component);
                value=demangle_name_plus_name(value,NULL,&qualified,&scope);
                demangle_name_assign(&scope,NULL,value);
            } else {
                if (*demangle_input=='?') value=demangle_parse_declaration(&component);
                else value=demangle_parse_dimension(&component);
                value=demangle_prepend_char(&quoted,'`',value);
                value=demangle_name_plus_char(value,NULL,&closed,'\'');
                value=demangle_name_plus_name(value,NULL,&qualified,&scope);
                demangle_name_assign(&scope,NULL,value);
            }
        } else {
            value=demangle_parse_name(&component);
            value=demangle_name_plus_name(value,NULL,&qualified,&scope);
            demangle_name_assign(&scope,NULL,value);
        }
    }
    if (!*demangle_input) {
        if (first) demangle_name_assign_status(&scope,NULL,2);
        else {
            value=demangle_name_status(&truncated,NULL,2);
            value=demangle_name_plus_text(value,NULL,&separator,"::");
            value=demangle_name_plus_name(value,NULL,&qualified,&scope);
            demangle_name_assign(&scope,NULL,value);
        }
    } else if (*demangle_input!='@') demangle_name_assign_status(&scope,NULL,1);
    return demangle_name_copy(result,NULL,&scope);
}

/* Native thiscall [00040370,000403f0); hidden result and source, RET8. */
DemangleName *__fastcall demangle_name_plus_name(const DemangleName *self,void *unused,
                                                 DemangleName *result,const DemangleName *source)
{
    DemangleName combined;
    demangle_name_copy(&combined,NULL,self);
    if (demangle_name_is_empty(&combined,NULL)) demangle_name_assign(&combined,NULL,source);
    else if (demangle_name_is_empty(source,NULL))
        demangle_name_append_status(&combined,NULL,(long)(source->flags<<28)>>28);
    else demangle_name_append_name(&combined,NULL,source);
    return demangle_name_copy(result,NULL,&combined);
}

/* Native thiscall [000406e0,00040770); RET4. */
DemangleName *__fastcall demangle_name_append_status(DemangleName *self,void *unused,int status)
{
    DemangleStatusNode *node;
    if (demangle_name_is_empty(self,NULL) || status==1 || status==3)
        return demangle_name_assign_status(self,NULL,status);
    node=demangle_allocate(&demangle_heap,NULL,sizeof(*node),0);
    if (node) node=demangle_status_node(node,NULL,status);
    if (node) {
        self->head=demangle_node_wrap(self->head,NULL);
        if (self->head) demangle_node_append(self->head,NULL,&node->base);
    } else self->head=NULL;
    if (!self->head) self->flags=(self->flags&~15UL)|3;
    return self;
}

/* Native thiscall [00040b00,00040b60); no stack arguments. */
DemangleNode *__fastcall demangle_node_wrap(DemangleNode *self,void *unused)
{
    DemangleReferenceNode *node;
    DemangleName *name;
    node=demangle_allocate(&demangle_heap,NULL,sizeof(*node),0);
    if (!node) return NULL;
    name=demangle_allocate(&demangle_heap,NULL,sizeof(*name),0);
    if (name) {
        name->head=self;
        name->flags&=~0x3fUL;
    }
    return (DemangleNode *)demangle_reference_node(node,NULL,name);
}

/* Native thiscall [00040d50,00040d90); RET4. */
DemangleReferenceNode *__fastcall demangle_reference_node(DemangleReferenceNode *self,
                                                           void *unused,DemangleName *name)
{
    demangle_node_initialize(&self->base,NULL,DEMANGLE_REFERENCE);
    if (name && ((name->flags&15)==1 || (name->flags&15)==3)) self->name=NULL;
    else self->name=name;
    return self;
}

/* Native thiscall [00040c10,00040c50); RET4. */
DemangleNode *__fastcall demangle_node_append(DemangleNode *self,void *unused,DemangleNode *next)
{
    DemangleNode *tail;
    if (next) {
        tail=self->next;
        if (tail) {
            while (tail->next) tail=tail->next;
            tail->next=next;
        } else self->next=next;
    }
    return self;
}

/* Native thiscall [00040810,00040860); RET4. */
DemangleName *__fastcall demangle_name_assign(DemangleName *self,void *unused,
                                              const DemangleName *source)
{
    unsigned status=self->flags&15;
    if (!status || status==2) {
        self->flags=(self->flags&~0x3fUL)|(source->flags&0x3f);
        self->head=source->head;
    }
    return self;
}

/* Native thiscall [00040320,00040370); hidden result/text, RET8. */
DemangleName *__fastcall demangle_name_plus_text(const DemangleName *self,void *unused,
                                                 DemangleName *result,const char *text)
{
    DemangleName combined;
    demangle_name_copy(&combined,NULL,self);
    if (demangle_name_is_empty(&combined,NULL)) demangle_name_assign_text(&combined,NULL,text);
    else demangle_name_append_text(&combined,NULL,text);
    return demangle_name_copy(result,NULL,&combined);
}

/* Native thiscall [00040510,000405a0); RET4. */
DemangleName *__fastcall demangle_name_append_text(DemangleName *self,void *unused,const char *text)
{
    DemangleTextNode *node;
    if (text && *text) {
        if (demangle_name_is_empty(self,NULL)) demangle_name_assign_text(self,NULL,text);
        else {
            self->head=demangle_node_wrap(self->head,NULL);
            if (self->head) {
                node=demangle_allocate(&demangle_heap,NULL,sizeof(*node),0);
                if (node) node=demangle_text_node(node,NULL,text,0);
                demangle_node_append(self->head,NULL,(DemangleNode *)node);
            } else self->flags=(self->flags&~15UL)|3;
        }
    }
    return self;
}

/* Original [00040e30,00040e50). */
int demangle_text_length(const char *text)
{
    int length=0;
    while (*text++) ++length;
    return length;
}

/* Native thiscall [000405a0,00040630); RET4. */
DemangleName *__fastcall demangle_name_append_name(DemangleName *self,void *unused,
                                                   const DemangleName *source)
{
    if (demangle_name_is_empty(source,NULL))
        demangle_name_append_status(self,NULL,(long)(source->flags<<28)>>28);
    else if (demangle_name_is_empty(self,NULL)) demangle_name_assign(self,NULL,source);
    else {
        self->head=demangle_node_wrap(self->head,NULL);
        if (self->head) demangle_node_append(self->head,NULL,source->head);
        else self->flags=(self->flags&~15UL)|3;
    }
    return self;
}

/* Decode declaration attributes and format the complete type.
 * Original [00040e80,00042540), including four adjacent switch tables. */
DemangleName *demangle_parse_type_declaration(DemangleName *result,const DemangleName *name)
{
    DemangleName declaration,temporary,second,third,component;
    DemangleName empty_type,empty_scope,this_qualifiers,return_type;
    DemangleName displacement,vtordisp,descriptor,table;
    DemangleName *value,*hole;
    unsigned attributes=0,code,group,modifier,access;
    int function,member,thunk,conversion,kind,first_keyword,second_keyword;
    signed char character;
    demangle_name_empty(&declaration,NULL);
    if (*demangle_input=='_') {
        attributes=0x4000;
        ++demangle_input;
    }
    character=(signed char)*demangle_input;
    if (character>='A' && character<='Z') {
        code=character-'A';
        ++demangle_input;
        attributes|=(code&1)?0xa000:0x8000;
        if (code<24) {
            attributes|=0x800;
            switch (code&0x18) {
            case 0: attributes|=0x40; break;
            case 8: attributes|=0x80; break;
            case 16: break;
            default: attributes=0xffff; break;
            }
            switch (code&6) {
            case 0: break;
            case 2: attributes|=0x200; break; /* static member */
            case 4: attributes|=0x100; break; /* virtual member */
            case 6: attributes|=0x400; break; /* adjustor thunk */
            }
        }
    } else if (character=='$') {
        ++demangle_input;
        character=(signed char)*demangle_input;
        if (!character) {
            attributes=0xfffe;
            ++demangle_input;
        } else if (character>='0' && character<='5') {
            code=character-'0';
            attributes|=(code&1)?0xad00:0x8d00;
            switch (code&6) {
            case 0: attributes|=0x40; break;
            case 2: attributes|=0x80; break;
            case 4: break;
            default: attributes=0xffff; break;
            }
            ++demangle_input;
        } else if (character=='A' || character=='B') {
            attributes|=character=='A'?0x9000:0x9800;
            ++demangle_input;
        } else attributes=0xffff;
    } else if (character>='0' && character<='8') {
        ++demangle_input;
        switch (character) {
        case '0': attributes=0x800; break;
        case '1': attributes=0x1000; break;
        case '2': attributes=0; break;
        case '3': attributes=0x4000; break;
        case '4': attributes=0x2000; break;
        case '5': attributes=0x6000; break;
        case '6': attributes=0x6800; break;
        case '7': attributes=0x7000; break;
        default: attributes=0xffff; break;
        }
    } else attributes=character?0xffff:0xfffe;
    conversion=!demangle_name_is_empty(name,NULL) && (name->flags&0x20);
    if (attributes==0xffff) return demangle_name_status(result,NULL,1);
    if (attributes==0xfffe) return demangle_prepend_status(result,2,name);
    function=(attributes&0x8000)!=0;
    group=attributes&0x1800;
    modifier=(attributes>>8)&7;
    member=function?(group==0x800):!(attributes&0x6000);
    thunk=member?(attributes&0x400):(attributes&0x1000);

    if (!function || (thunk && group==0x1000)) {
        demangle_name_append_name(&declaration,NULL,name);
        kind=attributes&0x7800;
        if (!function && (kind==0x6800 || kind==0x7000)) {
            demangle_name_copy(&table,NULL,&declaration);
            if (demangle_name_is_valid(&table,NULL) && *demangle_input) {
                if (kind==0x6800) {
                    demangle_name_empty(&empty_scope,NULL);
                    demangle_name_empty(&empty_type,NULL);
                    demangle_parse_data_qualifiers(&component,&empty_type,0,&empty_scope,0);
                } else demangle_parse_vbtable_type(&component);
                value=demangle_name_plus_char(&component,NULL,&temporary,' ');
                value=demangle_name_plus_name(value,NULL,&second,&table);
                demangle_name_assign(&table,NULL,value);
                if (demangle_name_is_valid(&table,NULL)) {
                    if (*demangle_input!='@') {
                        demangle_name_append_text(&table,NULL,"{for ");
                        while (demangle_name_is_valid(&table,NULL) &&
                               *demangle_input && *demangle_input!='@') {
                            value=demangle_parse_scope(&component);
                            value=demangle_prepend_char(&temporary,'`',value);
                            value=demangle_name_plus_char(value,NULL,&second,'\'');
                            demangle_name_append_name(&table,NULL,value);
                            if (*demangle_input=='@') ++demangle_input;
                            if (demangle_name_is_valid(&table,NULL) && *demangle_input!='@')
                                demangle_name_append_text(&table,NULL,"s ");
                        }
                        if (demangle_name_is_valid(&table,NULL)) {
                            if (!*demangle_input) demangle_name_append_status(&table,NULL,2);
                            demangle_name_append_char(&table,NULL,'}');
                        }
                    }
                    if (*demangle_input=='@') ++demangle_input;
                }
            } else if (demangle_name_is_valid(&table,NULL)) {
                value=demangle_prepend_status(&temporary,2,&table);
                demangle_name_assign(&table,NULL,value);
            }
            return demangle_name_copy(result,NULL,&table);
        }
        if (!function && kind==0x6000) {
            demangle_parse_dimension(&displacement);
            value=demangle_name_plus_char(&declaration,NULL,&temporary,'{');
            value=demangle_name_plus_name(value,NULL,&second,&displacement);
            return demangle_name_plus_text(value,NULL,result,"}'");
        }
        if (thunk && function && group==0x1000)
            demangle_name_append_text(&declaration,NULL,"`local static destructor helper'");
        else if (!function && kind==0x7800) return demangle_name_status(result,NULL,1);
        hole=demangle_allocate(&demangle_heap,NULL,sizeof(*hole),0);
        if (hole) demangle_name_empty(hole,NULL);
        demangle_parse_data_type(&return_type,hole);
        demangle_name_empty(&empty_scope,NULL);
        demangle_name_empty(&empty_type,NULL);
        demangle_parse_data_qualifiers(&component,&empty_type,0,&empty_scope,0);
        value=demangle_name_plus_char(&component,NULL,&temporary,' ');
        value=demangle_name_plus_name(value,NULL,&second,&declaration);
        demangle_name_assign(hole,NULL,value);
        demangle_name_copy(&temporary,NULL,&return_type);
        demangle_name_assign(&declaration,NULL,&temporary);
    } else {
        if (attributes&0x4000) {
            value=demangle_parse_based(&component);
            if (!(demangle_flags&10)) {
                value=demangle_prepend_char(&temporary,' ',value);
                demangle_name_assign(&declaration,NULL,value);
            } else demangle_name_merge_error(&declaration,NULL,value);
        }
        if (thunk && group==0x1800) {
            demangle_parse_dimension(&displacement);
            value=demangle_name_plus_char(name,NULL,&temporary,'{');
            value=demangle_name_plus_name(value,NULL,&second,&displacement);
            value=demangle_name_plus_char(value,NULL,&third,',');
            demangle_name_append_name(&declaration,NULL,value);
            demangle_name_character(&descriptor,NULL,'{');
            character=(signed char)*demangle_input;
            switch (character) {
            case 'A': case 'B': case 'E': case 'F': case 'I': case 'J': first_keyword=1; break;
            case 'C': case 'D': case 'G': case 'H': case 'K': case 'L': first_keyword=4; break;
            default: first_keyword=0; break;
            }
            if (!first_keyword) demangle_name_status(&component,NULL,character?1:2);
            else {
                demangle_name_append_text(&descriptor,NULL,demangle_keyword(first_keyword));
                demangle_name_append_text(&descriptor,NULL,"this, ");
                second_keyword=(character&1)?1:4;
                demangle_name_append_text(&descriptor,NULL,demangle_keyword(second_keyword));
                demangle_name_append_text(&descriptor,NULL,"call, ");
                ++demangle_input;
                if (character>='A' && character<='D')
                    demangle_name_append_text(&descriptor,NULL,demangle_keyword(1));
                else if (character>='E' && character<='H')
                    demangle_name_append_text(&descriptor,NULL,demangle_keyword(4));
                else {
                    value=demangle_parse_based(&temporary);
                    demangle_name_append_name(&descriptor,NULL,value);
                }
                demangle_name_append_text(&descriptor,NULL,"vfptr}");
                demangle_name_copy(&component,NULL,&descriptor);
            }
            value=demangle_name_plus_text(&component,NULL,&temporary,"}' ");
            demangle_name_append_name(&declaration,NULL,value);
        } else {
            demangle_name_empty(&vtordisp,NULL);
            demangle_name_empty(&displacement,NULL);
            demangle_name_empty(&this_qualifiers,NULL);
            if (thunk) {
                if (member && modifier==5) {
                    value=demangle_parse_dimension(&temporary);
                    demangle_name_assign(&vtordisp,NULL,value);
                }
                value=demangle_parse_dimension(&temporary);
                demangle_name_assign(&displacement,NULL,value);
            }
            if (member && modifier!=2) {
                demangle_name_empty(&empty_scope,NULL);
                demangle_name_empty(&empty_type,NULL);
                value=demangle_parse_data_qualifiers(&component,&empty_type,0,&empty_scope,1);
                if ((demangle_flags&0x60)==0x60)
                    demangle_name_merge_error(&this_qualifiers,NULL,value);
                else demangle_name_assign(&this_qualifiers,NULL,value);
            }
            value=demangle_parse_calling_convention(&component);
            if (!(demangle_flags&2)) {
                if (!(demangle_flags&0x10)) {
                    value=demangle_name_plus_name(value,NULL,&temporary,&declaration);
                    demangle_name_assign(&declaration,NULL,value);
                } else demangle_name_merge_error(&declaration,NULL,value);
                if (!(demangle_flags&8)) {
                    value=demangle_prepend_text(&temporary,
                        demangle_keyword((attributes&0x2000)?4:1),&declaration);
                    demangle_name_assign(&declaration,NULL,value);
                }
            } else demangle_name_merge_error(&declaration,NULL,value);
            if (!demangle_name_is_empty(name,NULL)) {
                if (!demangle_name_is_empty(&declaration,NULL)) {
                    value=demangle_prepend_char(&temporary,' ',name);
                    demangle_name_append_name(&declaration,NULL,value);
                } else demangle_name_assign(&declaration,NULL,name);
            }
            hole=NULL;
            demangle_name_empty(&return_type,NULL);
            if (conversion) {
                value=demangle_parse_return_type(&component,NULL);
                value=demangle_prepend_text(&temporary,"`",value);
                value=demangle_name_plus_text(value,NULL,&second,"' ");
                demangle_name_append_name(&declaration,NULL,value);
            } else {
                hole=demangle_allocate(&demangle_heap,NULL,sizeof(*hole),0);
                if (hole) demangle_name_empty(hole,NULL);
                value=demangle_parse_return_type(&component,hole);
                demangle_name_assign(&return_type,NULL,value);
            }
            if (thunk) {
                if (member && modifier==5) {
                    value=demangle_prepend_text(&temporary,"`vtordisp{",&vtordisp);
                    value=demangle_name_plus_char(value,NULL,&second,',');
                    demangle_name_append_name(&declaration,NULL,value);
                } else demangle_name_append_text(&declaration,NULL,"`adjustor{");
                value=demangle_name_plus_text(&displacement,NULL,&temporary,"}' ");
                demangle_name_append_name(&declaration,NULL,value);
            }
            value=demangle_parse_arguments(&component);
            value=demangle_prepend_char(&temporary,'(',value);
            value=demangle_name_plus_char(value,NULL,&second,')');
            demangle_name_append_name(&declaration,NULL,value);
            if (member && modifier!=2)
                demangle_name_append_name(&declaration,NULL,&this_qualifiers);
            value=demangle_parse_throw_types(&component);
            if (!(demangle_flags&0x100)) demangle_name_append_name(&declaration,NULL,value);
            else demangle_name_merge_error(&declaration,NULL,value);
            if (!(demangle_flags&4) && hole) {
                demangle_name_assign(hole,NULL,&declaration);
                demangle_name_assign(&declaration,NULL,&return_type);
            }
        }
    }
    if (member) {
        if (!(demangle_flags&0x200)) {
            if (!function || modifier==2) {
                value=demangle_prepend_text(&temporary,"static ",&declaration);
                demangle_name_assign(&declaration,NULL,value);
            }
            if ((function && modifier==1) || (thunk && (modifier==4 || modifier==5))) {
                value=demangle_prepend_text(&temporary,"virtual ",&declaration);
                demangle_name_assign(&declaration,NULL,value);
            }
        }
        if (!(demangle_flags&0x80)) {
            access=function?(attributes&0xc0):(attributes&0x1800);
            if (access==(function?0x40:0x800))
                value=demangle_prepend_text(&temporary,"private: ",&declaration);
            else if (access==(function?0x80:0x1000))
                value=demangle_prepend_text(&temporary,"protected: ",&declaration);
            else if (!access) value=demangle_prepend_text(&temporary,"public: ",&declaration);
            else value=NULL;
            if (value) demangle_name_assign(&declaration,NULL,value);
        }
    }
    if (thunk) {
        value=demangle_prepend_text(&temporary,"[thunk]:",&declaration);
        demangle_name_assign(&declaration,NULL,value);
    }
    return demangle_name_copy(result,NULL,&declaration);
}

/* Parse a data type around its declarator [0003e570,0003e6b0). */
DemangleName *demangle_parse_data_type(DemangleName *result,DemangleName *declarator)
{
    DemangleName reference,type,separated,combined;
    DemangleName *value;
    unsigned attributes;
    demangle_name_reference(&reference,NULL,declarator);
    if (!*demangle_input) return demangle_prepend_status(result,2,&reference);
    if (*demangle_input=='?') {
        ++demangle_input;
        if (!*demangle_input) attributes=0xfffe;
        else {
            attributes=(signed char)*demangle_input-'A';
            ++demangle_input;
            attributes=attributes<16?(attributes|0x8000):0xffff;
        }
        value=demangle_parse_indirect_type(&type,attributes);
        value=demangle_name_plus_char(value,NULL,&separated,' ');
        value=demangle_name_plus_name(value,NULL,&combined,&reference);
        demangle_name_assign(&reference,NULL,value);
        return demangle_name_copy(result,NULL,&reference);
    }
    if (*demangle_input!='X') return demangle_parse_primary_type(result,&reference);
    ++demangle_input;
    if (demangle_name_is_empty(&reference,NULL)) return demangle_name_text(result,NULL,"void");
    return demangle_prepend_text(result,"void ",&reference);
}

/* Native thiscall [0003ff10,0003ff80); RET4. */
DemangleName *__fastcall demangle_name_reference(DemangleName *self,void *unused,DemangleName *source)
{
    DemangleReferenceNode *node;
    if (source) {
        node=demangle_allocate(&demangle_heap,NULL,sizeof(*node),0);
        self->head=node?(DemangleNode *)demangle_reference_node(node,NULL,source):NULL;
        self->flags=(self->flags&~15UL)|(self->head?0:3);
    } else {
        self->head=NULL;
        self->flags&=~15UL;
    }
    self->flags&=~0x30UL;
    return self;
}

/* Parse primary/reference types [0003e6b0,0003e780). */
DemangleName *demangle_parse_primary_type(DemangleName *result,const DemangleName *declarator)
{
    DemangleName qualifier,reference;
    demangle_name_empty(&qualifier,NULL);
    if (!*demangle_input) return demangle_prepend_status(result,2,declarator);
    if (*demangle_input!='A' && *demangle_input!='B')
        return demangle_parse_basic_type(result,declarator);
    if (*demangle_input=='B') {
        demangle_name_assign_text(&qualifier,NULL,"volatile");
        if (!demangle_name_is_empty(declarator,NULL)) demangle_name_append_char(&qualifier,NULL,' ');
    }
    demangle_name_copy(&reference,NULL,declarator);
    reference.flags|=0x10;
    ++demangle_input;
    return demangle_parse_pointer_type(result,&qualifier,&reference,0);
}

/* Builtin types and pointer cv qualifiers [0003ea50,0003ecf0), with switch data. */
DemangleName *demangle_parse_basic_type(DemangleName *result,const DemangleName *declarator)
{
    DemangleName type,qualifier,pointer,temporary,separated;
    DemangleName *value;
    unsigned code,qualifiers=(unsigned)-1;
    if (!*demangle_input) return demangle_prepend_status(result,2,declarator);
    code=(signed char)*demangle_input++;
    demangle_name_empty(&type,NULL);
    switch (code) {
    case 'C': case 'D': case 'E': demangle_name_assign_text(&type,NULL,"char"); break;
    case 'F': case 'G': demangle_name_assign_text(&type,NULL,"short"); break;
    case 'H': case 'I': demangle_name_assign_text(&type,NULL,"int"); break;
    case 'J': case 'K': demangle_name_assign_text(&type,NULL,"long"); break;
    case 'L': demangle_name_assign_text(&type,NULL,demangle_keyword(18)); break;
    case 'M': demangle_name_assign_text(&type,NULL,"float"); break;
    case 'O': demangle_name_assign_text(&type,NULL,"long ");
    case 'N': demangle_name_append_text(&type,NULL,"double"); break;
    case 'P': case 'Q': case 'R': case 'S': qualifiers=code&3; break;
    default:
        --demangle_input;
        value=demangle_parse_indirect_type(&temporary,0);
        demangle_name_assign(&type,NULL,value);
        if (demangle_name_is_empty(&type,NULL)) return demangle_name_copy(result,NULL,&type);
        break;
    }
    if (qualifiers!=(unsigned)-1) {
        demangle_name_empty(&qualifier,NULL);
        demangle_name_copy(&pointer,NULL,declarator);
        if (qualifiers&1) {
            demangle_name_assign_text(&qualifier,NULL,"const");
            if (qualifiers&2) demangle_name_append_text(&qualifier,NULL," volatile");
        } else if (qualifiers&2) demangle_name_assign_text(&qualifier,NULL,"volatile");
        pointer.flags|=0x10;
        return demangle_parse_pointer_type(result,&qualifier,&pointer,1);
    }
    if (code=='C') {
        value=demangle_prepend_text(&temporary,"signed ",&type);
        demangle_name_assign(&type,NULL,value);
    } else if (code=='E' || code=='G' || code=='I' || code=='K') {
        value=demangle_prepend_text(&temporary,"unsigned ",&type);
        demangle_name_assign(&type,NULL,value);
    }
    if (!demangle_name_is_empty(declarator,NULL)) {
        value=demangle_prepend_char(&separated,' ',declarator);
        demangle_name_append_name(&type,NULL,value);
    }
    return demangle_name_copy(result,NULL,&type);
}

/* Native thiscall [000407e0,00040810); RET4. */
DemangleName *__fastcall demangle_name_assign_text(DemangleName *self,void *unused,const char *text)
{
    self->flags&=~0x30UL;
    demangle_name_set_range(self,NULL,text,demangle_text_length(text));
    return self;
}

/* Original [0003fdd0,0003fe00); explicit aggregate result. */
DemangleName *demangle_prepend_char(DemangleName *result,char character,const DemangleName *source)
{
    DemangleName prefix;
    demangle_name_character(&prefix,NULL,character);
    demangle_name_plus_name(&prefix,NULL,result,source);
    return result;
}

/* Native thiscall [0003fe80,0003fec0); RET4. */
DemangleName *__fastcall demangle_name_character(DemangleName *self,void *unused,char character)
{
    self->flags&=~0x3fUL;
    self->head=NULL;
    if (character) demangle_name_set_range(self,NULL,&character,1);
    return self;
}

/* Native thiscall [00040c50,00040c70); RET4. */
DemangleCharNode *__fastcall demangle_char_node(DemangleCharNode *self,void *unused,char character)
{
    demangle_node_initialize(&self->base,NULL,DEMANGLE_CHARACTER);
    self->character=character;
    return self;
}

/* Pointer/member cv and memory-model qualifiers [0003f860,0003fce0). */
DemangleName *demangle_parse_data_qualifiers(DemangleName *result,const DemangleName *type,
                                             char indirection,const DemangleName *scope,int member)
{
    DemangleName qualifier,component,temporary,second,third;
    DemangleName *value;
    const DemangleName *suffix;
    unsigned attributes,memory_model;
    signed char character=(signed char)*demangle_input;
    if (!character) {
        if (!member) {
            if (!demangle_name_is_empty(type,NULL)) {
                if (!(type->flags&0x10) && !demangle_name_is_empty(scope,NULL)) {
                    value=demangle_prepend_status(&component,2,scope);
                    value=demangle_name_plus_char(value,NULL,&temporary,' ');
                    return demangle_name_plus_name(value,NULL,result,type);
                }
                return demangle_prepend_status(result,2,type);
            }
            if (!demangle_name_is_empty(scope,NULL)) return demangle_prepend_status(result,2,scope);
        }
        return demangle_name_status(result,NULL,2);
    }
    attributes=character-(character<'A'?0x16:'A');
    ++demangle_input;
    if (attributes>31) return demangle_name_status(result,NULL,1);
    demangle_name_character(&qualifier,NULL,indirection);
    if (attributes&0x10) {
        if (member) return demangle_name_status(result,NULL,1);
        value=demangle_prepend_text(&temporary,"::",&qualifier);
        demangle_name_assign(&qualifier,NULL,value);
        if (!*demangle_input) value=demangle_prepend_status(&temporary,2,&qualifier);
        else {
            value=demangle_parse_scope(&component);
            value=demangle_prepend_char(&temporary,' ',value);
        }
        /* Native assignment replaces the earlier ::/indirection expression. */
        demangle_name_assign(&qualifier,NULL,value);
        if (!*demangle_input) demangle_name_append_status(&qualifier,NULL,2);
        else if (*demangle_input++!='@') return demangle_name_status(result,NULL,1);
    }
    memory_model=attributes&12;
    if (!(demangle_flags&2)) {
        if (memory_model==0) {
            if (!(demangle_flags&0x800)) {
                value=demangle_prepend_text(&temporary,demangle_keyword(0),&qualifier);
                demangle_name_assign(&qualifier,NULL,value);
            }
        } else if (memory_model==4 || memory_model==8) {
            value=demangle_prepend_text(&temporary,demangle_keyword(memory_model==4?3:6),&qualifier);
            demangle_name_assign(&qualifier,NULL,value);
        } else {
            if (member) return demangle_name_status(result,NULL,1);
            value=demangle_parse_based(&component);
            value=demangle_name_plus_name(value,NULL,&temporary,&qualifier);
            demangle_name_assign(&qualifier,NULL,value);
        }
    } else if (memory_model==12) {
        value=demangle_parse_based(&component);
        demangle_name_merge_error(&qualifier,NULL,value);
    }
    if (attributes&2) {
        value=demangle_prepend_text(&temporary,"volatile ",&qualifier);
        demangle_name_assign(&qualifier,NULL,value);
    }
    if (attributes&1) {
        value=demangle_prepend_text(&temporary,"const ",&qualifier);
        demangle_name_assign(&qualifier,NULL,value);
    }
    if (!member) {
        suffix=NULL;
        if (!demangle_name_is_empty(type,NULL)) {
            if (!(type->flags&0x10) && !demangle_name_is_empty(scope,NULL)) {
                value=demangle_prepend_char(&temporary,' ',scope);
                value=demangle_name_plus_char(value,NULL,&second,' ');
                value=demangle_name_plus_name(value,NULL,&third,type);
                demangle_name_append_name(&qualifier,NULL,value);
            } else suffix=type;
        } else if (!demangle_name_is_empty(scope,NULL)) suffix=scope;
        if (suffix) {
            value=demangle_prepend_char(&temporary,' ',suffix);
            demangle_name_append_name(&qualifier,NULL,value);
        }
    }
    return demangle_name_copy(result,NULL,&qualifier);
}

/* Native thiscall [000402d0,00040320); hidden result/character, RET8. */
DemangleName *__fastcall demangle_name_plus_char(const DemangleName *self,void *unused,
                                                 DemangleName *result,char character)
{
    DemangleName combined;
    demangle_name_copy(&combined,NULL,self);
    if (demangle_name_is_empty(&combined,NULL)) demangle_name_assign_char(&combined,NULL,character);
    else demangle_name_append_char(&combined,NULL,character);
    return demangle_name_copy(result,NULL,&combined);
}

/* Native thiscall [000407b0,000407e0); RET4. */
DemangleName *__fastcall demangle_name_assign_char(DemangleName *self,void *unused,char character)
{
    self->flags&=~0x30UL;
    demangle_name_set_range(self,NULL,&character,1);
    return self;
}


/* Original [0003fe30,0003fe60); explicit aggregate result. */
DemangleName *demangle_prepend_text(DemangleName *result,const char *text,const DemangleName *source)
{
    DemangleName prefix;
    demangle_name_text(&prefix,NULL,text);
    demangle_name_plus_name(&prefix,NULL,result,source);
    return result;
}

/* Native thiscall [0003ff80,0003ffc0); RET4. */
DemangleName *__fastcall demangle_name_text(DemangleName *self,void *unused,const char *text)
{
    self->flags&=~0x3fUL;
    self->head=NULL;
    if (text) demangle_name_set_range(self,NULL,text,demangle_text_length(text));
    return self;
}

/* Native thiscall [00040200,00040230); no stack arguments. */
int __fastcall demangle_name_length(DemangleName *self,void *unused)
{
    int length=0;
    DemangleNode *node;
    if (!demangle_name_is_empty(self,NULL))
        for (node=self->head;node;node=node->next) length+=demangle_node_length(node,NULL);
    return length;
}

/* Native thiscall [00040b60,00040ba0); no stack arguments. */
int __fastcall demangle_node_length(DemangleNode *self,void *unused)
{
    DemangleReferenceNode *reference;
    switch (self->kind) {
    case DEMANGLE_CHARACTER: return 1;
    case DEMANGLE_TEXT: return ((DemangleTextNode *)self)->length;
    case DEMANGLE_REFERENCE:
        reference=(DemangleReferenceNode *)self;
        return reference->name?demangle_name_length(reference->name,NULL):0;
    case DEMANGLE_STATUS: return ((DemangleStatusNode *)self)->length;
    default: return 0;
    }
}

/* Native thiscall [00040230,000402d0); output/capacity, RET8. */
char *__fastcall demangle_name_write(DemangleName *self,void *unused,char *output,int capacity)
{
    DemangleNode *node;
    char *cursor;
    int length;
    if (demangle_name_is_empty(self,NULL)) {
        if (output) *output=0;
        return output;
    }
    if (!output) {
        capacity=demangle_name_length(self,NULL)+1;
        output=demangle_allocate(&demangle_heap,NULL,capacity,0);
    }
    if (!output) return NULL;
    cursor=output;
    for (node=self->head;node && capacity>0;node=node->next) {
        length=demangle_node_length(node,NULL);
        if (!length) continue;
        if (capacity-length<0) length=capacity;
        if (demangle_node_write(node,NULL,cursor,length)) {
            capacity-=length;
            cursor+=length;
        }
    }
    *cursor=0;
    return output;
}

/* Native thiscall [00040ba0,00040c10); output/capacity, RET8. */
char *__fastcall demangle_node_write(DemangleNode *self,void *unused,char *output,int capacity)
{
    switch (self->kind) {
    case DEMANGLE_CHARACTER: return demangle_char_write((DemangleCharNode *)self,NULL,output,capacity);
    case DEMANGLE_TEXT: return demangle_text_write((DemangleTextNode *)self,NULL,output,capacity);
    case DEMANGLE_REFERENCE: return demangle_reference_write((DemangleReferenceNode *)self,NULL,output,capacity);
    case DEMANGLE_STATUS: return demangle_status_write((DemangleStatusNode *)self,NULL,output,capacity);
    default: return NULL;
    }
}

/* Native thiscall [00040d90,00040dc0); output/capacity, RET8. */
char *__fastcall demangle_reference_write(DemangleReferenceNode *self,void *unused,char *output,int capacity)
{
    if (self->name && output && capacity)
        return demangle_name_write(self->name,NULL,output,capacity);
    return NULL;
}

/* Native thiscall [00040d10,00040d50); output/capacity, RET8. */
char *__fastcall demangle_text_write(DemangleTextNode *self,void *unused,char *output,int capacity)
{
    if (self->length<capacity) capacity=self->length;
    if (self->text && output && capacity) return demangle_copy_text(output,self->text,capacity);
    return NULL;
}

/* Native thiscall [00040c70,00040c90); output/capacity, RET8. */
char *__fastcall demangle_char_write(DemangleCharNode *self,void *unused,char *output,int capacity)
{
    if (!output || !capacity) return NULL;
    *output=self->character;
    return output;
}

/* Operator and compiler-generated names [00042540,000427c0). */
DemangleName *demangle_parse_operator(DemangleName *result)
{
    DemangleName name,temporary,prefixed;
    DemangleName *value;
    const char *saved,*spelling;
    signed char code;
    int conversion=0;
    demangle_name_empty(&name,NULL);
    code=(signed char)*demangle_input++;
    if (!code) {
        --demangle_input;
        return demangle_name_status(result,NULL,2);
    }
    if (code=='0' || code=='1') {
        saved=demangle_input;
        value=demangle_parse_name(&temporary);
        demangle_name_assign(&name,NULL,value);
        demangle_input=saved;
        if (!demangle_name_is_empty(&name,NULL) && demangle_input[-1]=='1') {
            value=demangle_prepend_char(&prefixed,'~',&name);
            demangle_name_assign(&name,NULL,value);
        }
        return demangle_name_copy(result,NULL,&name);
    }
    if (code=='_') {
        code=(signed char)*demangle_input++;
        switch (code) {
        case 0:
            --demangle_input;
            return demangle_name_status(result,NULL,2);
        case '0': spelling="/="; break;
        case '1': spelling="%="; break;
        case '2': spelling=">>="; break;
        case '3': spelling="<<="; break;
        case '4': spelling="&="; break;
        case '5': spelling="|="; break;
        case '6': spelling="^="; break;
        case '7': return demangle_name_text(result,NULL,"`vftable'");
        case '8': return demangle_name_text(result,NULL,"`vbtable'");
        case '9': return demangle_name_text(result,NULL,"`vcall");
        case 'A': return demangle_name_text(result,NULL,"`typeof'");
        case 'B': return demangle_name_text(result,NULL,"`local static guard");
        case 'C': return demangle_name_text(result,NULL,"`reserved'");
        case 'D': return demangle_name_text(result,NULL,"`vbase destructor'");
        case 'E': return demangle_name_text(result,NULL,"`vector deleting destructor'");
        case 'F': return demangle_name_text(result,NULL,"`default constructor closure'");
        case 'G': return demangle_name_text(result,NULL,"`scalar deleting destructor'");
        case 'H': return demangle_name_text(result,NULL,"`vector constructor iterator'");
        case 'I': return demangle_name_text(result,NULL,"`vector destructor iterator'");
        case 'J': return demangle_name_text(result,NULL,"`vector vbase constructor iterator'");
        default: return demangle_name_status(result,NULL,1);
        }
    } else {
        switch (code) {
        case '2': spelling=" new"; break;
        case '3': spelling=" delete"; break;
        case '4': spelling="="; break;
        case '5': spelling=">>"; break;
        case '6': spelling="<<"; break;
        case '7': spelling="!"; break;
        case '8': spelling="=="; break;
        case '9': spelling="!="; break;
        case 'A': spelling="[]"; break;
        case 'B': spelling="operator"; conversion=1; break;
        case 'C': spelling="->"; break;
        case 'D': spelling="*"; break;
        case 'E': spelling="++"; break;
        case 'F': spelling="--"; break;
        case 'G': spelling="-"; break;
        case 'H': spelling="+"; break;
        case 'I': spelling="&"; break;
        case 'J': spelling="->*"; break;
        case 'K': spelling="/"; break;
        case 'L': spelling="%"; break;
        case 'M': spelling="<"; break;
        case 'N': spelling="<="; break;
        case 'O': spelling=">"; break;
        case 'P': spelling=">="; break;
        case 'Q': spelling=","; break;
        case 'R': spelling="()"; break;
        case 'S': spelling="~"; break;
        case 'T': spelling="^"; break;
        case 'U': spelling="|"; break;
        case 'V': spelling="&&"; break;
        case 'W': spelling="||"; break;
        case 'X': spelling="*="; break;
        case 'Y': spelling="+="; break;
        case 'Z': spelling="-="; break;
        default: return demangle_name_status(result,NULL,1);
        }
    }
    demangle_name_assign_text(&name,NULL,spelling);
    if (!demangle_name_is_empty(&name,NULL)) {
        if (conversion) name.flags|=0x20;
        else {
            value=demangle_prepend_text(&prefixed,"operator",&name);
            demangle_name_assign(&name,NULL,value);
        }
    }
    return demangle_name_copy(result,NULL,&name);
}

/* Calling conventions [0003e430,0003e530). */
DemangleName *demangle_parse_calling_convention(DemangleName *result)
{
    DemangleName convention;
    unsigned code,keyword;
    if (!*demangle_input) return demangle_name_status(result,NULL,2);
    code=(signed char)*demangle_input++-'A';
    if (code>10) return demangle_name_status(result,NULL,1);
    demangle_name_empty(&convention,NULL);
    if (!(demangle_flags&2)) {
        if (code==10) demangle_name_assign_text(&convention,NULL,demangle_keyword(15));
        else {
            switch (code&~1U) {
            case 0: keyword=10; break;
            case 2: keyword=11; break;
            case 4: keyword=13; break;
            case 6: keyword=12; break;
            case 8: keyword=14; break;
            default: keyword=0; break;
            }
            demangle_name_assign_text(&convention,NULL,demangle_keyword(keyword));
            if (code&1) {
                /* Native adds32 to keyword16(__saveregs), reaching __segname(\".
                 * Name that observed relationship instead of preserving literal padding. */
                demangle_keyword(16);
                demangle_name_append_text(&convention,NULL,
                    (demangle_flags&1)?"segname(\"":"__segname(\"");
            }
        }
    }
    return demangle_name_copy(result,NULL,&convention);
}

/* Native keyword lookup [0003fce0,0003fd20). Internal indices0..18. */
const char *demangle_keyword(int keyword)
{
    const char *spelling;
    switch (keyword) {
    case 0: spelling="__near"; break;
    case 1: spelling="__near "; break;
    case 2: spelling="__near*"; break;
    case 3: spelling="__far"; break;
    case 4: spelling="__far "; break;
    case 5: spelling="__far*"; break;
    case 6: spelling="__huge"; break;
    case 7: spelling="__huge "; break;
    case 8: spelling="__huge*"; break;
    case 9: spelling="__based("; break;
    case 10: spelling="__cdecl"; break;
    case 11: spelling="__pascal"; break;
    case 12: spelling="__stdcall"; break;
    case 13: spelling="__thiscall"; break;
    case 14: spelling="__fastcall"; break;
    case 15: spelling="__interrupt"; break;
    case 16: spelling="__saveregs"; break;
    case 17: spelling="__self"; break;
    case 18: spelling="__segment"; break;
    default: return NULL;
    }
    if ((keyword==1 || keyword==2) && (demangle_flags&0x800)) return spelling+6;
    return spelling+((demangle_flags&1)?2:0);
}

/* Return-type slot [0003e530,0003e570). */
DemangleName *demangle_parse_return_type(DemangleName *result,DemangleName *declarator)
{
    if (*demangle_input=='@') {
        ++demangle_input;
        return demangle_name_reference(result,NULL,declarator);
    }
    demangle_parse_data_type(result,declarator);
    return result;
}

/* Function argument-list wrapper [0003e780,0003e8a0). */
DemangleName *demangle_parse_arguments(DemangleName *result)
{
    DemangleName arguments,variadic;
    DemangleName *value;
    if (*demangle_input=='X') {
        ++demangle_input;
        return demangle_name_text(result,NULL,"void");
    }
    if (*demangle_input=='Z') {
        ++demangle_input;
        return demangle_name_text(result,NULL,"...");
    }
    demangle_parse_argument_list(&arguments);
    if (!(arguments.flags&15) && *demangle_input) {
        if (*demangle_input=='@') ++demangle_input;
        else if (*demangle_input=='Z') {
            ++demangle_input;
            value=demangle_name_plus_text(&arguments,NULL,&variadic,",...");
            return demangle_name_copy(result,NULL,value);
        } else return demangle_name_status(result,NULL,1);
    }
    return demangle_name_copy(result,NULL,&arguments);
}

/* Parse argument types and their backreferences [0003e8a0,0003e9b0). */
DemangleName *demangle_parse_argument_list(DemangleName *result)
{
    DemangleName arguments,type,empty;
    DemangleName *value;
    const char *start;
    int first=1,index;
    demangle_name_empty(&arguments,NULL);
    while (!(arguments.flags&15) && *demangle_input!='@' && *demangle_input!='Z') {
        if (first) first=0;
        else demangle_name_append_char(&arguments,NULL,',');
        start=demangle_input;
        if (!*start) {
            demangle_name_append_status(&arguments,NULL,2);
            break;
        }
        index=(signed char)*demangle_input-'0';
        if (index>=0 && index<=9) {
            ++demangle_input;
            value=demangle_replicator_get(demangle_type_replicator,NULL,index);
            demangle_name_append_name(&arguments,NULL,value);
        } else {
            demangle_name_empty(&empty,NULL);
            demangle_parse_primary_type(&type,&empty);
            if ((long)(demangle_input-start)>1 && demangle_type_replicator->index!=9)
                demangle_replicator_add(demangle_type_replicator,NULL,&type);
            demangle_name_append_name(&arguments,NULL,&type);
        }
    }
    return demangle_name_copy(result,NULL,&arguments);
}

/* Native thiscall [00040490,00040510); RET4. */
DemangleName *__fastcall demangle_name_append_char(DemangleName *self,void *unused,char character)
{
    DemangleCharNode *node;
    if (character) {
        if (demangle_name_is_empty(self,NULL)) demangle_name_assign_char(self,NULL,character);
        else {
            self->head=demangle_node_wrap(self->head,NULL);
            if (self->head) {
                node=demangle_allocate(&demangle_heap,NULL,sizeof(*node),0);
                if (node) node=demangle_char_node(node,NULL,character);
                demangle_node_append(self->head,NULL,(DemangleNode *)node);
            } else self->flags=(self->flags&~15UL)|3;
        }
    }
    return self;
}

/* Exception specification [0003e9b0,0003ea50). */
DemangleName *demangle_parse_throw_types(DemangleName *result)
{
    DemangleName arguments,prefix,incomplete;
    DemangleName *value;
    if (*demangle_input=='Z') {
        ++demangle_input;
        demangle_name_empty(&arguments,NULL);
        return demangle_name_copy(result,NULL,&arguments);
    }
    if (*demangle_input) {
        value=demangle_parse_arguments(&arguments);
        value=demangle_prepend_text(&prefix," throw(",value);
    } else {
        demangle_name_text(&arguments,NULL," throw(");
        value=demangle_name_plus_status(&arguments,NULL,&incomplete,2);
    }
    demangle_name_plus_char(value,NULL,result,')');
    return result;
}

/* Pointer/reference, arrays and function-pointer declarators [0003f010,0003f860). */
DemangleName *demangle_parse_pointer_type(DemangleName *result,const DemangleName *qualifier,
                                         const DemangleName *declarator,int pointer)
{
    DemangleName expression,qualified,this_qualifiers,empty_type,empty_scope;
    DemangleName component,temporary,second,third,return_type,array,dimensions,element_type;
    DemangleName *value,*hole,*array_type;
    unsigned kind,count;
    char indirection=pointer?'*':'&';
    signed char character=(signed char)*demangle_input;
    if (!character) {
        demangle_name_status(&expression,NULL,2);
        demangle_name_append_char(&expression,NULL,indirection);
        if (!demangle_name_is_empty(qualifier,NULL)) demangle_name_append_name(&expression,NULL,qualifier);
        if (!demangle_name_is_empty(declarator,NULL)) {
            if (!demangle_name_is_empty(qualifier,NULL)) demangle_name_append_char(&expression,NULL,' ');
            demangle_name_append_name(&expression,NULL,declarator);
        }
        return demangle_name_copy(result,NULL,&expression);
    }
    if ((character>'5' && character<':') || character=='_') {
        kind=character-'6';
        ++demangle_input;
        if (kind==0x29) {
            if (!*demangle_input) return demangle_prepend_status(result,2,declarator);
            kind=(signed char)*demangle_input-0x3d;
            ++demangle_input;
            if (kind<4 || kind>=8) kind=(unsigned)-1;
        } else if (kind>=4) kind=(unsigned)-1;
        if (kind==(unsigned)-1) return demangle_name_status(result,NULL,1);
        demangle_name_empty(&this_qualifiers,NULL);
        demangle_name_character(&expression,NULL,indirection);
        if (!demangle_name_is_empty(qualifier,NULL) &&
            (demangle_name_is_empty(declarator,NULL) || (declarator->flags&0x10)))
            demangle_name_append_name(&expression,NULL,qualifier);
        if (!demangle_name_is_empty(declarator,NULL)) demangle_name_append_name(&expression,NULL,declarator);
        if (kind&2) {
            value=demangle_prepend_text(&temporary,"::",&expression);
            demangle_name_assign(&expression,NULL,value);
            if (!*demangle_input) value=demangle_prepend_status(&temporary,2,&expression);
            else {
                value=demangle_parse_scope(&component);
                value=demangle_prepend_char(&temporary,' ',value);
            }
            demangle_name_assign(&expression,NULL,value);
            if (!*demangle_input) return demangle_prepend_status(result,2,&expression);
            if (*demangle_input!='@') return demangle_name_status(result,NULL,1);
            ++demangle_input;
            demangle_name_empty(&empty_scope,NULL);
            demangle_name_empty(&empty_type,NULL);
            value=demangle_parse_data_qualifiers(&component,&empty_type,0,&empty_scope,1);
            if ((demangle_flags&0x60)==0x60) demangle_name_merge_error(&this_qualifiers,NULL,value);
            else demangle_name_assign(&this_qualifiers,NULL,value);
        }
        if (kind&4) {
            value=demangle_parse_based(&component);
            if (!(demangle_flags&2)) {
                value=demangle_prepend_char(&temporary,' ',value);
                value=demangle_name_plus_name(value,NULL,&second,&expression);
                demangle_name_assign(&expression,NULL,value);
            } else demangle_name_merge_error(&expression,NULL,value);
        }
        value=demangle_parse_calling_convention(&component);
        if (!(demangle_flags&2)) {
            value=demangle_name_plus_name(value,NULL,&temporary,&expression);
            demangle_name_assign(&expression,NULL,value);
            value=demangle_prepend_text(&temporary,demangle_keyword((kind&1)?4:1),&expression);
            demangle_name_assign(&expression,NULL,value);
        } else demangle_name_merge_error(&expression,NULL,value);
        value=demangle_prepend_char(&temporary,'(',&expression);
        value=demangle_name_plus_char(value,NULL,&second,')');
        demangle_name_assign(&expression,NULL,value);
        hole=demangle_allocate(&demangle_heap,NULL,sizeof(*hole),0);
        if (hole) demangle_name_empty(hole,NULL);
        demangle_parse_return_type(&return_type,hole);
        value=demangle_parse_arguments(&component);
        value=demangle_prepend_char(&temporary,'(',value);
        value=demangle_name_plus_char(value,NULL,&second,')');
        demangle_name_append_name(&expression,NULL,value);
        if ((demangle_flags&0x60)!=0x60 && (kind&2))
            demangle_name_append_name(&expression,NULL,&this_qualifiers);
        value=demangle_parse_throw_types(&component);
        if (!(demangle_flags&0x100)) demangle_name_append_name(&expression,NULL,value);
        else demangle_name_merge_error(&expression,NULL,value);
        if (!hole) return demangle_name_status(result,NULL,3);
        demangle_name_assign(hole,NULL,&expression);
        return demangle_name_copy(result,NULL,&return_type);
    }
    demangle_parse_data_qualifiers(&qualified,declarator,indirection,qualifier,0);
    if (!*demangle_input) return demangle_prepend_status(result,2,&qualified);
    if (pointer && *demangle_input=='X') {
        ++demangle_input;
        if (demangle_name_is_empty(&qualified,NULL)) return demangle_name_text(result,NULL,"void");
        return demangle_prepend_text(result,"void ",&qualified);
    }
    if (*demangle_input!='Y') return demangle_parse_basic_type(result,&qualified);
    hole=demangle_allocate(&demangle_heap,NULL,sizeof(*hole),0);
    if (hole) demangle_name_empty(hole,NULL);
    if (!hole) return demangle_name_status(result,NULL,3);
    ++demangle_input;
    demangle_name_reference(&array,NULL,hole);
    if (!*demangle_input) {
        if (demangle_name_is_empty(&array,NULL))
            value=demangle_name_character(&component,NULL,'[');
        else {
            value=demangle_prepend_char(&component,'(',&array);
            value=demangle_name_plus_text(value,NULL,&temporary,")[");
        }
        value=demangle_name_plus_status(value,NULL,&second,2);
        array_type=demangle_name_plus_char(value,NULL,&third,']');
    } else {
        count=demangle_read_dimension();
        if (!count) {
            value=demangle_name_character(&component,NULL,'[');
            value=demangle_name_plus_status(value,NULL,&temporary,2);
            array_type=demangle_name_plus_char(value,NULL,&second,']');
        } else {
            demangle_name_empty(&dimensions,NULL);
            do {
                value=demangle_parse_dimension(&component);
                value=demangle_prepend_char(&temporary,'[',value);
                value=demangle_name_plus_char(value,NULL,&second,']');
                demangle_name_append_name(&dimensions,NULL,value);
            } while (--count);
            if (!demangle_name_is_empty(&array,NULL)) {
                value=demangle_prepend_char(&component,'(',&array);
                value=demangle_name_plus_char(value,NULL,&temporary,')');
                value=demangle_name_plus_name(value,NULL,&second,&dimensions);
                demangle_name_assign(&dimensions,NULL,value);
            }
            array_type=&dimensions;
        }
    }
    demangle_parse_basic_type(&element_type,array_type);
    if (!demangle_name_is_empty(&element_type,NULL)) demangle_name_assign(hole,NULL,&qualified);
    return demangle_name_copy(result,NULL,&element_type);
}

/* Enum/class/struct/union names and memory qualifiers [0003ecf0,0003f010),
 * including the native eight-entry enum switch table. */
DemangleName *demangle_parse_indirect_type(DemangleName *result,unsigned attributes)
{
    DemangleName type,underlying,enumeration,component,temporary,combined;
    DemangleName *value;
    unsigned memory_model;
    signed char code;
    const char *prefix;
    demangle_name_empty(&type,NULL);
    if (attributes) {
        if (attributes==0xffff) return demangle_name_status(result,NULL,1);
        if (attributes==0xfffe) demangle_name_assign_status(&type,NULL,2);
        else {
            memory_model=attributes&12;
            if (memory_model!=12) {
                if (!(demangle_flags&0x402))
                    demangle_name_assign_text(&type,NULL,demangle_keyword(
                        memory_model==0?1:(memory_model==4?4:7)));
            } else {
                value=demangle_parse_based(&component);
                if (!(demangle_flags&0x402)) demangle_name_assign(&type,NULL,value);
                else demangle_name_merge_error(&type,NULL,value);
            }
        }
    }
    code=(signed char)*demangle_input++;
    if (!code) {
        --demangle_input;
        value=demangle_prepend_text(&temporary,"`unknown ecsu'",&type);
        return demangle_name_plus_status(value,NULL,result,2);
    }
    switch (code) {
    case 'T': prefix="union "; break;
    case 'U': prefix="struct "; break;
    case 'V': prefix="class "; break;
    case 'W':
        demangle_name_empty(&underlying,NULL);
        if (!*demangle_input) demangle_name_status(&enumeration,NULL,2);
        else {
            switch (*demangle_input) {
            case '0': case '1': demangle_name_assign_text(&underlying,NULL,"char "); break;
            case '2': case '3': demangle_name_assign_text(&underlying,NULL,"short "); break;
            case '4': break;
            case '5': demangle_name_assign_text(&underlying,NULL,"int "); break;
            case '6': case '7': demangle_name_assign_text(&underlying,NULL,"long "); break;
            default:
                demangle_name_status(&enumeration,NULL,1);
                goto enum_name;
            }
            code=(signed char)*demangle_input++;
            if (code=='1' || code=='3' || code=='5' || code=='7') {
                value=demangle_prepend_text(&temporary,"unsigned ",&underlying);
                demangle_name_assign(&underlying,NULL,value);
            }
            value=demangle_parse_type_name(&component);
            demangle_name_plus_name(&underlying,NULL,&enumeration,value);
        }
enum_name:
        value=demangle_prepend_text(&temporary,"enum ",&type);
        value=demangle_name_plus_name(value,NULL,&combined,&enumeration);
        demangle_name_assign(&type,NULL,value);
        return demangle_name_copy(result,NULL,&type);
    default: return demangle_name_status(result,NULL,1);
    }
    value=demangle_prepend_text(&temporary,prefix,&type);
    demangle_name_assign(&type,NULL,value);
    value=demangle_parse_type_name(&component);
    demangle_name_append_name(&type,NULL,value);
    return demangle_name_copy(result,NULL,&type);
}

/* Qualified ECSU name [0003e310,0003e430). */
DemangleName *demangle_parse_type_name(DemangleName *result)
{
    DemangleName name,component,temporary,combined,truncated;
    DemangleName *value;
    demangle_name_empty(&name,NULL);
    if (*demangle_input=='?') {
        ++demangle_input;
        value=demangle_parse_template(&component);
    } else value=demangle_parse_name(&component);
    demangle_name_assign(&name,NULL,value);
    if (!(name.flags&15) && *demangle_input && *demangle_input!='@') {
        value=demangle_parse_scope(&component);
        value=demangle_name_plus_text(value,NULL,&temporary,"::");
        value=demangle_name_plus_name(value,NULL,&combined,&name);
        demangle_name_assign(&name,NULL,value);
    }
    if (*demangle_input=='@') ++demangle_input;
    else if (*demangle_input) demangle_name_assign_status(&name,NULL,1);
    else if (demangle_name_is_empty(&name,NULL)) demangle_name_assign_status(&name,NULL,2);
    else {
        value=demangle_name_status(&truncated,NULL,2);
        value=demangle_name_plus_text(value,NULL,&temporary,"::");
        value=demangle_name_plus_name(value,NULL,&combined,&name);
        demangle_name_assign(&name,NULL,value);
    }
    return demangle_name_copy(result,NULL,&name);
}

/* Native thiscall [00040ab0,00040ae0); RET4. */
DemangleName *__fastcall demangle_replicator_get(DemangleReplicator *self,void *unused,int index)
{
    if (index<0 || index>9) return &self->error;
    if (self->index==-1 || self->index<index) return &self->missing;
    return self->values[index];
}

/* Numeric dimension/count reader [0003dff0,0003e080). */
unsigned demangle_read_dimension(void)
{
    unsigned value=0;
    signed char character=(signed char)*demangle_input;
    if (!character) return 0;
    if (character>='0' && character<='9') {
        ++demangle_input;
        return character-'0'+1;
    }
    while (*demangle_input!='@') {
        character=(signed char)*demangle_input;
        if (!character) return 0;
        if (character<'A' || character>'P') return (unsigned)-1;
        value=(value<<4)+(character-'A');
        ++demangle_input;
    }
    if (*demangle_input++!='@') return (unsigned)-1;
    return value;
}

/* Dimension name decoder [0003df10,0003dff0). Separate native vertex from count reader. */
DemangleName *demangle_parse_dimension(DemangleName *result)
{
    unsigned value=0;
    signed char character=(signed char)*demangle_input;
    if (!character) return demangle_name_status(result,NULL,2);
    if (character>='0' && character<='9') {
        ++demangle_input;
        return demangle_name_unsigned(result,NULL,character-'0'+1);
    }
    while (*demangle_input!='@') {
        character=(signed char)*demangle_input;
        if (!character) return demangle_name_status(result,NULL,2);
        if (character<'A' || character>'P') return demangle_name_status(result,NULL,1);
        value=(value<<4)+(character-'A');
        ++demangle_input;
    }
    if (*demangle_input++!='@') return demangle_name_status(result,NULL,1);
    return demangle_name_unsigned(result,NULL,value);
}

/* Native thiscall [000400e0,00040150); RET4. */
DemangleName *__fastcall demangle_name_unsigned(DemangleName *self,void *unused,unsigned value)
{
    char digits[11],*end=digits+10,*cursor=end;
    self->flags&=~0x3fUL;
    self->head=NULL;
    *end=0;
    do {
        *--cursor=(char)('0'+value%10);
        value/=10;
    } while (value);
    demangle_name_set_range(self,NULL,cursor,end-cursor);
    return self;
}
