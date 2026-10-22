/* Parse mutable .drectve export specifications. */
#include "import-library.h"

void parse_export(char *text,Image *image,unsigned char directives,const char *context)
{
    char delimiter=0,*cursor,*internal=NULL,*ordinal_text;
    unsigned long ordinal=0;
    int kind=0;
    cursor=take_export_token(text,"=,",context,&delimiter);
    if (delimiter=='=') {
        internal=cursor;
        cursor=take_export_token(cursor,",",context,&delimiter);
    }
    if (delimiter=='@') {
        ordinal_text=cursor+1;
        cursor=take_export_token(ordinal_text,",",context,&delimiter);
        sscanf(ordinal_text,"%li",&ordinal);
        if (!ordinal) fatal_message(context,0x16,ordinal_text);
        if (!_strnicmp(cursor,"NONAME",6)) {
            cursor+=6;
            if (*cursor==',') ++cursor;
        }
    }
    if (!_strnicmp(cursor,"CONSTANT",8)) { kind=1; cursor+=8; }
    else if (!_strnicmp(cursor,"DATA",4)) { kind=2; cursor+=4; }
    if (*cursor) fatal_message(context,4,"BAD export directive");
    register_export(image->symbols,text,internal,NULL,kind,ordinal,context,directives,
                    image,&image->optional);
}


char *take_export_token(char *text,const char *delimiters,const char *context,char *delimiter)
{
    char *cursor=text+strcspn(text,delimiters);
    if (*cursor) {
        if (!cursor[1]) warning_message(context,0x7a,"","");
        *delimiter=*cursor;
        *cursor++=0;
    }
    if (*delimiter==',') *delimiter=*cursor;
    return cursor;
}


#pragma intrinsic(strlen,strcpy)
Library *export_library;
Module *export_module;
Contribution *export_contribution;
SectionList export_sections={NULL,&export_sections.first};
unsigned long export_forwarder_bytes,export_alias_bytes;

void register_export(SymbolTable *table,char *name,char *internal,char *forwarder,
                     int kind,unsigned long ordinal,const char *context,
                     unsigned char directives,Image *image,IMAGE_OPTIONAL_HEADER *header)
{
    LibraryList libraries;
    External *external;
    unsigned char created,imports;
    char *decorated,*target,*cursor;
    if (!export_library) {
        libraries.first=NULL;
        libraries.tail=&libraries.first;
        libraries.exclude_defaults=0;
        libraries.default_libraries=NULL;
        export_library=create_library(NULL,0,1,&libraries);
        export_library->search_flags |= 4;
        if (!context) context="(commandline)";
        export_module=create_module(context,context,0,image->file.Machine,0,0,0,0,0,export_library,NULL);
        export_contribution=create_contribution("",0,0,0,0,0,0,0,0,0,
                                                export_module,1,&export_sections,header);
    }
    decorated=malloc(strlen(name)+2);
    if (!decorated) out_of_memory();
    cursor=decorated;
    if (import_only_export && *name!='?' && *name!='@') *cursor++='_';
    strcpy(cursor,name);
    external=lookup_external(export_symbol_table,(unsigned short)(strlen(decorated)<=8),
                             decorated,&created,&imports);
    if (external->flags&EXTERNAL_DEFINED) {
        if (external->flags&0x400) return;
        warning_message(context,0x4d,decorated,context);
    }
    external->flags |= 0x400;
    set_external_defined(external,1,export_symbol_table);
    if (kind==1) external->flags |= 8;
    else if (kind==2) external->flags |= 0x2000;
    external->value.final_value=0;
    external->archive_member=++import_member_base;
    external->symbol.Value=ordinal;
    external->owner.contribution=export_contribution;
    export_contribution->module->path=duplicate_string(context);
    if (ordinal) record_export_ordinal(ordinal);
    if (directives && !strchr(decorated,'@')) external->flags |= 0x10;
    if (internal) {
        target=malloc(strlen(internal)+2);
        if (!target) out_of_memory();
        cursor=target;
        if (import_only_export && *internal!='?' && *internal!='@') *cursor++='_';
        strcpy(cursor,internal);
        external->export_target=target;
        export_alias_bytes+=strlen(target);
        return;
    }
    if (forwarder) {
        external->flags |= 0x200;
        target=malloc(strlen(forwarder)+1);
        if (!target) out_of_memory();
        strcpy(target,forwarder);
        external->export_target=target;
        export_forwarder_bytes+=strlen(target);
    }
}
