/* Module-definition language used while building import libraries. */
#include "definition-file.h"
#include <recon.h>
#pragma intrinsic(strlen,strcpy,strcat,strcmp)
FILE *definition_stream;
char *definition_cursor;
char definition_line[261];
unsigned long definition_version;
unsigned char definition_name_seen;
/* Shared by keyword recognition, dispatch diagnostics and section handlers. */
const char *definition_keywords[]={"NAME","LIBRARY","DESCRIPTION","HEAPSIZE",
    "STACKSIZE","PROTMODE","CODE","DATA","SEGMENTS","EXPORTS","IMPORTS","STUB",
    "OLD","APPLOADER","EXETYPE","REALMODE","FUNCTIONS","INCLUDE","SECTIONS",
    "OBJECTS","VERSION",""};

/* Original [00029520,00029b90), including the native keyword jump table. */
void read_definition_file(Image *image,const char *path)
{
    unsigned short keyword=0xffff;
    unsigned long reserve,commit;
    char number[28],directive[128],*token,*dot;
    definition_stream=fopen(path,"rt");
    if (!definition_stream) fatal_message(program_name,7,path);
    while ((definition_cursor=read_definition_line())!=NULL) {
        if (!*definition_cursor) continue;
        if ((unsigned char)*definition_cursor==0x7f) break;
        token=strtok(definition_cursor," \t");
        if (!token) continue;
        keyword=find_definition_keyword(token);
        if (keyword!=0xffff) break;
    }
    while (keyword!=0xffff) {
        switch (keyword) {
        case DEF_NAME:
        case DEF_LIBRARY:
            if (definition_cursor) while (*definition_cursor++);
            keyword=read_definition_name((unsigned char)(keyword==DEF_LIBRARY));
            break;
        case DEF_DESCRIPTION:
            if (definition_cursor) while (*definition_cursor++);
            keyword=read_definition_description(definition_cursor);
            break;
        case DEF_HEAPSIZE:
            if (definition_cursor) while (*definition_cursor++);
            reserve=commit=0;
            keyword=read_definition_sizes("HEAP",definition_cursor,&reserve,&commit);
            if (reserve) {
                strcpy(directive,"-HEAP:0x");
                _ultoa(reserve,number,16);
                strcat(directive,number);
                if (commit) {
                    strcat(directive,",0x");
                    _ultoa(commit,number,16);
                    strcat(directive,number);
                }
                append_export_directive(directive);
            }
            break;
        case DEF_STACKSIZE:
            if (definition_cursor) while (*definition_cursor++);
            reserve=commit=0;
            keyword=read_definition_sizes("STACK",definition_cursor,&reserve,&commit);
            if (reserve) {
                strcpy(directive,"-STACK:0x");
                _ultoa(reserve,number,16);
                strcat(directive,number);
                if (commit) {
                    strcat(directive,",0x");
                    _ultoa(commit,number,16);
                    strcat(directive,number);
                }
                append_export_directive(directive);
            }
            break;
        case DEF_CODE: keyword=skip_definition_code(); break;
        case DEF_DATA: keyword=skip_definition_data(); break;
        case DEF_SEGMENTS:
        case DEF_SECTIONS:
        case DEF_OBJECTS:
            if (definition_cursor) while (*definition_cursor++);
            keyword=read_definition_sections(definition_keywords[keyword],definition_cursor);
            break;
        case DEF_EXPORTS:
            if (definition_cursor) while (*definition_cursor++);
            keyword=read_definition_exports(image,definition_cursor);
            break;
        case DEF_IMPORTS:
            if (definition_cursor) while (*definition_cursor++);
            keyword=read_definition_imports(definition_cursor);
            break;
        case DEF_STUB:
        case DEF_OLD:
        case DEF_APPLOADER:
        case DEF_REALMODE:
            warning_message(def_filename,0x58,definition_keywords[keyword],"");
            keyword=next_definition_keyword();
            break;
        case DEF_EXETYPE:
            if (definition_cursor) while (*definition_cursor++);
            if (_strcmpi(definition_cursor,"WINDOWS"))
                warning_message(def_filename,0x58,definition_keywords[keyword],"");
            keyword=next_definition_keyword();
            break;
        case DEF_VERSION:
            if (definition_cursor) while (*definition_cursor++);
            reserve=commit=0;
            dot=strchr(definition_cursor,'.');
            if (dot && (sscanf(dot+1,"%li",&commit)!=1 || commit>0x7fff))
                fatal_message(def_filename,0x15,"VERSION");
            if (sscanf(definition_cursor,"%li",&reserve)!=1 || reserve>0x7fff)
                fatal_message(def_filename,0x15,"VERSION");
            definition_version=(reserve<<16)+commit;
            if (definition_version) {
                strcpy(directive,"-VERSION:0x");
                _ultoa(reserve,number,16);
                strcat(directive,number);
                if (commit) {
                    strcat(directive,".0x");
                    _ultoa(commit,number,16);
                    strcat(directive,number);
                }
                append_export_directive(directive);
            }
            keyword=next_definition_keyword();
            break;
        case DEF_PROTMODE:
        case DEF_FUNCTIONS:
        case DEF_INCLUDE:
            keyword=next_definition_keyword();
            break;
        }
    }
    fclose(definition_stream);
}

/* Original [00029b90,00029c20). Comments start at the first semicolon. */
char *read_definition_line(void)
{
    char *comment,*text;
    long last;
    if (!fgets(definition_line,261,definition_stream)) return "\x7f";
    last=strlen(definition_line)-1;
    if (definition_line[last]=='\n') definition_line[last]=0;
    if (definition_line[last-1]=='\r') definition_line[last-1]=0;
    comment=strchr(definition_line,';');
    if (comment) *comment=0;
    text=definition_line;
    while (*text==' ' || *text=='\t') ++text;
    return text;
}

/* Original [0002a910,0002a970). DEF keywords are case sensitive. */
unsigned short find_definition_keyword(const char *text)
{
    unsigned short index=0;
    while (*definition_keywords[index]) {
        if (!strcmp(definition_keywords[index],text)) return index;
        ++index;
    }
    return 0xffff;
}

/* Original [00029c70,00029fa0), including the seven-entry attribute table. */
unsigned short read_definition_name(unsigned char library)
{
    static const char *attributes[]={"BASE","WINDOWAPI","WINDOWCOMPAT","NOTWINDOWCOMPAT",
        "NEWFILES","LONGNAMES","INITINSTANCE",""};
    char directive[128],number[36],*name,*extension;
    unsigned short attribute;
    unsigned long base;
    if (definition_name_seen)
        warning_message(def_filename,0x58,library?"LIBRARY":"NAME","");
    else {
        definition_name_seen=1;
        name=strtok(definition_cursor," \t\"");
        if (name) {
            export_image->options.library_image_name=duplicate_string(name);
            extension=strchr(export_image->options.library_image_name,'.');
            if (extension) {
                export_image->options.default_extension=duplicate_string(extension);
                *extension=0;
            } else if (!library) export_image->options.default_extension=".exe";
            if (library) append_export_directive("-DLL");
            strcpy(directive,"-OUT:");
            strcat(directive,export_image->options.library_image_name);
            strcat(directive,export_image->options.default_extension);
            append_export_directive(directive);
            while ((name=strtok(NULL," \t"))!=NULL) {
                attribute=0;
                if (_strnicmp(name,"BASE=",5))
                    while (*attributes[attribute] && _strcmpi(attributes[attribute],name))
                        ++attribute;
                switch (attribute) {
                case 0:
                    if (sscanf(name+5,"%li",&base)!=1) fatal_message(def_filename,0x15,"NAME");
                    strcpy(directive,"-BASE:0x");
                    _ultoa(base,number,16);
                    strcat(directive,number);
                    append_export_directive(directive);
                    break;
                case 1: append_export_directive("-subsystem:windows"); break;
                case 2: append_export_directive("-subsystem:console"); break;
                case 3: warning_message(def_filename,0x58,name,""); break;
                case 4:
                case 5:
                case 6: break;
                default: fatal_message(def_filename,0x15,library?"LIBRARY":"NAME"); break;
                }
            }
        }
    }
    return next_definition_keyword();
}

/* Original [00029c20,00029c70). Skip blank and unrecognized lines. */
unsigned short next_definition_keyword(void)
{
    char *token;
    unsigned short keyword;
    while ((definition_cursor=read_definition_line())!=NULL) {
        if (!*definition_cursor) continue;
        if ((unsigned char)*definition_cursor==0x7f) return 0xffff;
        token=strtok(definition_cursor," \t");
        if (!token) continue;
        keyword=find_definition_keyword(token);
        if (keyword!=0xffff) return keyword;
    }
    return 0;
}

/* Original [00029fa0,0002a000). Double quotes remain part of DESCRIPTION. */
unsigned short read_definition_description(char *text)
{
    char *description,*quote;
    if (text && *text) {
        description=text;
        while (*description==' ' || *description=='\t') ++description;
        if (*description=='\'') {
            ++description;
            if (*description) {
                quote=strrchr(text,'\'');
                if (quote) *quote=0;
            }
        }
        export_description=duplicate_string(description);
    }
    return next_definition_keyword();
}

/* Original [0002a7a0,0002a810). */
unsigned short read_definition_exports(Image *image,char *text)
{
    unsigned short keyword;
    if (!text) definition_cursor=read_definition_line();
    else {
        while (*text==' ' || *text=='\t') ++text;
        definition_cursor=text;
    }
    for (;;) {
        if ((unsigned char)*definition_cursor==0x7f) return 0xffff;
        keyword=parse_definition_export(image,definition_cursor,def_filename);
        if (keyword) return keyword;
        definition_cursor=read_definition_line();
        if (!definition_cursor) return 0;
    }
}

/* Original [0002a530,0002a7a0). A definition line or the next section keyword. */
unsigned short parse_definition_export(Image *image,char *text,const char *context)
{
    char delimiter=*text,*cursor=text,*token,*internal=NULL,*forwarder=NULL;
    unsigned char noname=0;
    unsigned short keyword;
    unsigned long ordinal=0;
    int kind=0;
    if (!delimiter) return 0;
    while (*cursor) {
        if (*cursor=='\t' || *cursor==' ' || *cursor=='=') {
            delimiter=*cursor;
            *cursor=0;
        } else ++cursor;
    }
    ++cursor;
    if (*cursor) {
        while (*cursor==' ' || *cursor=='\t') ++cursor;
        if (delimiter==' ' || delimiter=='\t') {
            delimiter=*cursor;
            if (delimiter=='=' || delimiter=='@') ++cursor;
        }
    }
    if (delimiter) {
        keyword=find_definition_keyword(text);
        if (keyword!=0xffff) return keyword;
    }
    token=strtok(cursor," \t");
    if (!token) { token=""; delimiter=0; }
    if (delimiter=='=') {
        if (strchr(token,'.')) forwarder=token;
        else internal=token;
        token=strtok(NULL," \t");
        if (!token) token="";
        delimiter=*token;
        if (delimiter=='@') ++token;
    }
    if (delimiter=='@')
        token=read_definition_ordinal(token,"",context,&delimiter,&ordinal,&noname);
    if (!_strcmpi(token,"CONSTANT")) {
        kind=1;
        token=strtok(NULL," \t");
        if (!token) { token=""; delimiter=0; }
    } else if (!_strcmpi(token,"DATA")) {
        kind=2;
        token=strtok(NULL," \t");
        if (!token) { token=""; delimiter=0; }
    }
    if (!_strcmpi(token,"NODATA") || !_strcmpi(token,"IOPL")) {
        token=strtok(NULL," \t");
        if (!token) { token=""; delimiter=0; }
    }
    if (strcmp(token,"")) fatal_message(context,0x19,token);
    /* This native caller consumes NONAME but does not pass that local flag. */
    register_export(image->symbols,text,internal,forwarder,kind,ordinal,context,0,
                    image,&image->optional);
    return 0;
}

/* Original [0002a460,0002a530). */
char *read_definition_ordinal(char *text,char *fallback,const char *context,
                              char *delimiter,unsigned long *ordinal,unsigned char *noname)
{
    char *token;
    sscanf(text,"%li",ordinal);
    if (!*ordinal) fatal_message(context,0x16,text);
    else record_export_ordinal(*ordinal);
    token=strtok(NULL," \t");
    if (!token) { token=fallback; *delimiter=*fallback; }
    if (!_strcmpi(token,"NONAME")) {
        *noname=1;
        token=strtok(NULL," \t");
        if (!token) { token=fallback; *delimiter=*fallback; }
    }
    if (!_strcmpi(token,"RESIDENTNAME")) {
        token=strtok(NULL," \t");
        if (!token) { token=fallback; *delimiter=*fallback; }
    }
    return token;
}
