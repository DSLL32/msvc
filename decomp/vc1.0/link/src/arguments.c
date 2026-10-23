/* Command-line, response-file, and library-search argument collection. */
#include "arguments.h"
#include <io.h>
#include <recon.h>

Argument *first_argument;
ArgumentList search_paths, input_files, library_files, other_inputs;
char *response_cursor;
extern ArgumentList switches;
extern TextBuffer command_trace;
extern char *option_delimiters, *output_name;

char *duplicate_string(const char *text)
{
    char *copy=_strdup(text);
    if (!copy) out_of_memory();
    return copy;
}

void append_argument_node(ArgumentList *list,char *text,char *original)
{
    Argument *node=malloc(sizeof(*node));
    if (!node) out_of_memory();
    node->text=text;
    node->original=original;
    node->next=NULL;
    if (!list->first) list->first=node;
    else list->last->next=node;
    ++list->count;
    list->last=node;
    if (!first_argument) first_argument=node;
}

void append_argument(ArgumentList *list,char *text)
{
    append_argument_node(list,text,text);
}

void collect_search_path(const char *environment,ArgumentList *list)
{
    char *value=getenv(environment), *copy, *token;
    unsigned length, i;
    if (!value) return;
    for (length=0;value[length];++length) ;
    copy=calloc(length+1,1);
    if (!copy) out_of_memory();
    else for (i=0;i<=length;++i) copy[i]=value[i];
    token=strtok(copy,";");
    while (token) {
        for (length=0;token[length];++length) ;
        if (length && token[length-1]=='\\') token[length-1]=0;
        append_argument(list,token);
        token=strtok(NULL,";");
    }
}

char *tokenize_response(char *text,const char *delimiters,unsigned char *quoted)
{
    char *start, *closing, *scan;
    unsigned length;
    if (text) response_cursor=text;
next_token:
    if (quoted) *quoted=0;
    while (*response_cursor && strchr(delimiters,(unsigned char)*response_cursor))
        ++response_cursor;
    if (!*response_cursor) { response_cursor=NULL; return NULL; }
    start=response_cursor;
    while (*response_cursor && !strchr(delimiters,(unsigned char)*response_cursor)) {
        if (*response_cursor=='"') {
            scan=response_cursor+1;
            for (length=0;scan[length];++length) ;
            memmove(response_cursor,scan,length+1);
            closing=strchr(response_cursor,'"');
            if (closing) {
                scan=closing+1;
                for (length=0;scan[length];++length) ;
                memmove(closing,scan,length+1);
                response_cursor=closing;
                if (quoted) *quoted=1;
            }
        } else ++response_cursor;
    }
    if (*response_cursor) *response_cursor++=0;
    if (!*start) goto next_token;
    return start;
}

void parse_response_line(char *line)
{
    unsigned char quoted;
    unsigned length;
    char *token=tokenize_response(line,option_delimiters,&quoted);
    if (!token) return;
    do {
        if (quoted) append_text(&command_trace,"\"",1);
        for (length=0;token[length];++length) ;
        append_text(&command_trace,token,length);
        if (quoted) append_text(&command_trace,"\"",1);
        append_text(&command_trace," ",1);
        classify_argument(token,1);
        token=tokenize_response(NULL,option_delimiters,&quoted);
    } while (token);
    append_text(&command_trace,"\n",1);
}

void collect_arguments(int argc,char **argv,const char *environment)
{
    char *value, *copy, *line, *comment;
    FILE *response;
    unsigned length;
    int i;
    if (tool_kind==1) collect_search_path("LIB",&search_paths);
    if (environment && (value=getenv(environment))!=NULL) {
        copy=duplicate_string(value);
        parse_response_line(copy);
        free(copy);
    }
    first_argument=NULL;
    for (i=1;i<argc;++i) {
        if (argv[i][0]!='@') { classify_argument(argv[i],0); continue; }
        response=fopen(argv[i]+1,"rt");
        if (!response) fatal_message(program_name,7,argv[i]+1);
        line=malloc(16384);
        if (!line) out_of_memory();
        while (fgets(line,16384,response)) {
            for (length=0;line[length];++length) ;
            line[length-1]=0;
            comment=strchr(line,';');
            if (comment) *comment=0;
            parse_response_line(line);
        }
        free(line);
        fflush(stdout);
        fclose(response);
    }
}

void classify_argument(char *text,unsigned char copy)
{
    char drive[_MAX_DRIVE], directory[_MAX_DIR], name[_MAX_FNAME];
    char extension[_MAX_EXT], path[_MAX_PATH], resolved[_MAX_PATH];
    Argument *search;
    ArgumentList *list;
    const char *source;
    char *destination, *dot;
    unsigned i;
    if (text[0]=='/' || text[0]=='-') {
        ++text;
        list=&switches;
        if (text[0]=='?' && text[1]) ++text;
    } else {
        if (strpbrk(text,"?*")) { expand_wildcards(text); return; }
        if (_access(text,0)==-1 && text[0]!='.' && text[0]!='\\') {
            search=search_paths.first;
            for (i=0;i<search_paths.count;++i,search=search->next) {
                destination=resolved;
                source=search->text;
                while ((*destination++=*source++)!=0) ;
                destination[-1]='\\';
                source=text;
                while ((*destination++=*source++)!=0) ;
                if (!_access(resolved,0)) { text=duplicate_string(resolved); break; }
            }
        }
        if (tool_kind==1) {
            _splitpath(text,drive,directory,name,extension);
            if (!extension[0]) {
                source=".obj";
                destination=extension;
                while ((*destination++=*source++)!=0) ;
                _makepath(path,drive,directory,name,extension);
                text=duplicate_string(path);
            }
            list=&input_files;
        } else {
            input_file=open_file(text,0x8000,0);
            if (is_library_file(text,input_file)) list=&other_inputs;
            else {
                list=&library_files;
                if (!output_name[0]) {
                    output_name=duplicate_string(text);
                    dot=strrchr(output_name,'.');
                    if (dot) *dot=0;
                }
            }
            close_file(input_file,0);
        }
    }
    if (copy) text=duplicate_string(text);
    append_argument(list,text);
}

#pragma intrinsic(strcmp)
void remove_temporary_inputs(void)
{
    ArgumentList *list=tool_kind==1?&input_files:&library_files;
    Argument *argument=list->first;
    unsigned short index;
    for (index=0;index<list->count;++index) {
        if (strcmp(argument->text,argument->original)) remove(argument->original);
        argument=argument->next;
    }
}

extern unsigned long archive_data_offset;
unsigned char is_library_file(const char *path,int file)
{
    char signature[8];
    if ((unsigned short)read_file(file,signature,8)!=8)
        fatal_message(path,0x27,"");
    if (!strncmp(signature,"!<arch>\n",8)) {
        archive_data_offset=8;
        return 1;
    }
    return 0;
}


void copy_argument_node(ArgumentList *list,char *text,char *original)
{
    append_argument_node(list,text,original);
}


void clear_argument_list(ArgumentList *list)
{
    Argument *argument,*next;
    if (list->count) {
        argument=list->first;
        while (argument) {
            next=argument->next;
            free(argument);
            argument=next;
        }
        list->count=0;
    }
}


/* CRT startup file enumerator retained as a distinct library vertex. */
extern char *__cdecl _find(const char *pattern);
#pragma intrinsic(strcpy)
/* Expand one relative/qualified wildcard operand [00020630,000206e0). */
void expand_wildcards(const char *pattern)
{
    char path[_MAX_PATH],*end=path,*filename=path;
    const char *scan=pattern;
    char *found;
    while (*scan) {
        *end++=*scan;
        if (*scan=='\\' || *scan=='/') filename=end;
        ++scan;
    }
    *end=0;
    printf("[ ");
    found=_find(pattern);
    while (found) {
        strcpy(filename,found);
        printf("%s ",found);
        classify_argument(path,1);
        found=_find(NULL);
    }
    printf("] ");
}
#pragma function(strcpy)
