/* Resolve undecorated exports against linked definitions and archive indexes. */
#include "import-library.h"
#pragma intrinsic(strlen,strcmp,strcpy,strcat,memcpy)
extern char *def_filename;

void merge_export_definitions(SymbolTable *table,SymbolTable *linked,
                              unsigned char search,Library *first)
{
    External **ordered,**snapshot,*external,*replacement;
    LibraryEnumerator libraries;
    Library *library;
    char short_name[9]={0};
    /* Original name/context scratch ranges overlap by thirty pointer slots. */
    char *scratch[90],**names=scratch,**contexts=scratch+30;
    char *name,*candidate,*base,*suffix;
    unsigned long count,index,length,size,member;
    unsigned short found,match,preferred;
    unsigned char created,imports,compatible;
    ordered=get_externals_by_name(table);
    count=count_externals(table);
    snapshot=malloc(count*sizeof(*snapshot));
    if (!snapshot) out_of_memory();
    memcpy(snapshot,ordered,count*sizeof(*snapshot));
    invalidate_symbol_sorts(table);
    for (index=0;index<count;++index) {
        external=snapshot[index];
        if (!(external->flags&EXTERNAL_DEFINED) || (external->flags&0x10)) continue;
        name=external->export_target;
        if (name) {
            if (external->flags&0x200) {
                name=strchr(name,'.')+1;
                if (*name=='#') name=NULL;
            }
        }
        if (!name) {
            if (external->symbol.N.Name.Short)
                name=strncpy(short_name,(char *)&external->symbol,8);
            else name=table->strings.data+external->symbol.N.Name.Long;
        }
        if (strchr(name,'@')) continue;
        found=0;
        find_export_matches(linked,name,&found,names,contexts);
        compatible=*name=='?';
        for (match=0;match<found;++match)
            if (*names[match]!='?') compatible=1;
        if ((!found || !compatible) && search) {
            length=strlen(name);
            initialize_library_enumerator(&libraries,first);
            while (next_library(&libraries)) {
                library=libraries.current;
                for (member=0;member<library->symbol_count;++member) {
                    candidate=library->symbol_names[member];
                    base=candidate;
                    if (*base=='?' || *base=='_') ++base;
                    if (!strncmp(name,base,length) && (!base[length] || base[length]=='@') && found<60) {
                        if (found && !strcmp(names[0],candidate)) continue;
                        names[found]=candidate;
                        contexts[found]=library->name;
                        ++found;
                    }
                }
            }
        }
        if (!found) {
            set_external_defined(external,0,table);
            if (external->owner.contribution)
                record_undefined_reference(external,external->owner.contribution->module);
            continue;
        }
        if (found!=1) {
            preferred=0xffff;
            for (match=0;match<found;++match) {
                if (!strcmp(name,names[match])) break;
                if (*name!='?' && *names[match]!='?')
                    preferred=preferred==0xffff?match:found;
            }
            if (match>=found) match=preferred;
            if (match<found) { names[0]=names[match]; found=1; }
            if (found!=1) {
                export_error=1;
                warning_message(def_filename,0x5d,name,"");
                for (match=0;match<found && match<60;++match)
                    warning_message(def_filename,0x4d,names[match],contexts[match]);
                continue;
            }
        }
        if (external->export_target) {
            free(external->export_target);
            external->export_target=duplicate_string(names[0]);
            if (external->symbol.N.Name.Short)
                base=strncpy(short_name,(char *)&external->symbol,8);
            else base=table->strings.data+external->symbol.N.Name.Long;
            size=strlen(base);
            if (*external->export_target=='?' || *external->export_target=='_') ++size;
            suffix=strchr(external->export_target,'@');
            if (suffix) size+=strlen(suffix);
            names[0]=calloc(size+1,1);
            if (!names[0]) out_of_memory();
            if (*external->export_target=='?' || *external->export_target=='_')
                strncat(names[0],external->export_target,1);
            strcat(names[0],base);
            if (suffix) strcat(names[0],suffix);
        }
        replacement=lookup_external(table,(unsigned short)(strlen(names[0])<=8),names[0],&created,&imports);
        if (replacement!=external) {
            replacement->owner.contribution=external->owner.contribution;
            replacement->export_target=external->export_target;
            set_external_defined(replacement,1,table);
            replacement->flags=external->flags|0x10;
            replacement->value=external->value;
            replacement->archive_member=external->archive_member;
            replacement->symbol.Value=external->symbol.Value;
            set_external_defined(external,0,table);
            if (external->owner.contribution)
                record_undefined_reference(external,external->owner.contribution->module);
            external->flags |= EXTERNAL_IGNORED;
        }
    }
    free(snapshot);
}


unsigned long count_export_symbols(SymbolTable *table,unsigned long *data_exports)
{
    External *external;
    unsigned long total=0,data=0;
    begin_external_enumeration(table);
    while ((external=next_external(table))!=NULL) {
        if (external->flags&0x4001) {
            ++total;
            if (external->flags&0x2000) ++data;
        }
    }
    end_external_enumeration(table);
    if (data_exports) *data_exports=data;
    return total;
}


ExternalLink *create_import_aliases(SymbolTable *table)
{
    ExternalLink *head=NULL,*node;
    External *external,*alias;
    char *name,*import_name;
    unsigned char created,imports;
    begin_external_enumeration(table);
    while ((external=next_external(table))!=NULL) {
        if (external->flags&EXTERNAL_DEFINED) {
            node=malloc(sizeof(*node));
            node->external=external;
            node->next=head;
            head=node;
        }
    }
    end_external_enumeration(table);
    for (node=head;node;node=node->next) {
        external=node->external;
        if (external->symbol.N.Name.Short)
            name=strncpy(short_symbol_name,(char *)&external->symbol,8);
        else name=table->strings.data+external->symbol.N.Name.Long;
        import_name=malloc(strlen(name)+7);
        strcpy(import_name,"__imp_");
        strcat(import_name,name);
        alias=lookup_external(table,0,import_name,&created,&imports);
        set_external_defined(alias,1,table);
        alias->value.final_value=0;
        alias->flags |= 0x800;
        alias->archive_member=external->archive_member;
        alias->owner.contribution=external->owner.contribution;
        node->external=alias;
    }
    return head;
}


void mark_explicit_export_ordinals(SymbolTable *table,unsigned char *ordinals)
{
    External **ordered=get_externals_by_name(table),*external;
    unsigned long count=count_externals(table),index,slot;
    unsigned short ordinal;
    for (index=0;index<count;++index) {
        external=ordered[index];
        if ((external->flags&EXTERNAL_DEFINED) && (ordinal=(unsigned short)external->symbol.Value)!=0) {
            slot=ordinal-first_export_ordinal;
            if (ordinals[slot]) fatal_message(def_filename,0x18,external->symbol.Value);
            ordinals[slot]=1;
        }
    }
}

void intern_export_targets(SymbolTable *table)
{
    External **ordered=get_externals_by_name(table),*external;
    unsigned long count=count_externals(table),index;
    for (index=0;index<count;++index) {
        external=ordered[index];
        if ((external->flags&EXTERNAL_DEFINED) && !(external->flags&EXTERNAL_EMITTED) &&
            external->export_target && !(external->flags&0x200) && strlen(external->export_target)>8)
            intern_symbol_name(table,external->export_target);
    }
}
