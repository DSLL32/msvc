/* Build the librarian response when exports or a definition file require it.
 * This owner has a real scoped 20-byte buffer and original C++ EH cleanup. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <sys/stat.h>
#include "link.h"
#include "../crt/crt-imports.h"
#include "options.h"
#include "pass-one.h"
#include "full-link-functions.h"
#include "sections.h"
#include "input.h"
#include "definition-buffer-construct.h"
#include "definition-buffer-cleanup.h"
#include "definition-buffer-append.h"
#include <recon.h>

extern "C" {
extern BYTE incremental_enabled,export_directive_changed,mac_target;
extern BYTE verbose_libraries,xoff;
extern InputArguments export_arguments;
extern const char *output_file_name,*import_library_name;
extern StringNode *entry_symbol;
extern char short_symbol_name[9];
DWORD mac_current_import_version;
LONG mac_old_code_version=-1,mac_old_api_version=-1;
DWORD definition_export_state;
void __stdcall collect_incremental_exports(LinkContext *,const char *,IncrementalDefinition *)
{ __asm { int 3 } }
void __stdcall warn_existing_export_section(const char *)
{ __asm { int 3 } }
unsigned __stdcall write_argument_file(int,const void *,unsigned)
{ __asm { int 3 } return 0; }
}

#pragma intrinsic(strlen,memcpy)
extern "C" BYTE __stdcall process_definition_file(LinkContext *context,const char *name)
{
#ifdef RECON_NATIVE_TEST
    /* Native exceptions are disabled. Express this same object's lifetime
     * explicitly so Clang retains the original two normal cleanup call roles. */
    union { void *alignment; BYTE bytes[sizeof(DefinitionBuffer)]; } storage;
    DefinitionBuffer &buffer=*new(storage.bytes) DefinitionBuffer;
#else
    DefinitionBuffer buffer;
#endif
    ByteBuffer command={NULL,0,0};
    InputArgument *argument;
    IncludeSymbol *include;
    DefaultLibrary *library;
    OutputModule *module;
    WORD index,name_kind;
    BYTE comma;
    const char *text,*library_name;
    unsigned length,offsets[3];
    const char *args[4];
    int file;
    char temp_path[260],response[260],drive[4],directory[256];
    char basename[256],extension[256],library_path[256],export_path[256];
    char number[12];
    struct _stat information;
    InputArgument export_argument;

    buffer.Grow(4096);
    if(incremental_enabled)
        collect_incremental_exports(context,name,&context->definition);
    if(!export_arguments.count && !export_directive_changed) {
        if(!name || !*name) {
#ifdef RECON_NATIVE_TEST
            [[clang::always_inline]] buffer.~DefinitionBuffer();
#endif
            return 0;
        }
    }
    if(export_group->first) {
        warn_existing_export_section(export_group->first->module->name);
#ifdef RECON_NATIVE_TEST
        [[clang::always_inline]] buffer.~DefinitionBuffer();
#endif
        return 0;
    }
    if(!GetTempPathA(sizeof(temp_path),temp_path) ||
       !GetTempFileNameA(temp_path,"lah",0,response)) link_fatal(NULL,1104,"TEMPFILE");
    file=open_argument_file(response,0x8002,0);
    if(file==-1) link_fatal(NULL,1104,"TEMPFILE");
    buffer.Write("/def:",5);
    if(!name || !*name) buffer.Write("\n",1);
    else {
        buffer.Write("\"",1);
        buffer.Write(name,strlen(name));
        buffer.Write("\"\n",2);
    }
    if(!output_file_name) link_fatal(NULL,1159);
    _splitpath(output_file_name,drive,directory,basename,extension);
    buffer.Write("/name:",6);
    buffer.Write("\"",1);
    buffer.Write(basename,strlen(basename));
    buffer.Write(extension,strlen(extension));
    buffer.Append("\"\n",2);
    library_name=import_library_name;
    if(!library_name) {
        _makepath(library_path,drive,directory,basename,".lib");
        library_name=library_path;
    }
    buffer.Append("/out:",5);
    buffer.Append("\"",1);
    buffer.AppendString(library_name);
    buffer.Append("\"\n",2);
    if(!context->options.legacy_import_library) buffer.Append("/vc6compat\n",11);
    argument=export_arguments.first;
    for(index=0;index<export_arguments.count;++index) {
        if(!argument->original_name) {
            buffer.Append("/export:",8);
            buffer.AppendString(argument->name);
            buffer.Append("\n",1);
        }
        argument=argument->next;
    }
    for(include=context->includes;include;include=include->next) {
        buffer.Append("/include:",9);
        text=get_symbol_name(include->symbol,context->symbols->hash.string_storage,&name_kind,0);
        if(name_kind==1) text=strncpy(short_symbol_name,text,8);
        buffer.AppendString(text);
        buffer.Append("\n",1);
    }
    argument=input_files.first;
    for(index=0;index<input_files.count;++index) {
        if(argument->switch_info->value_count) {
            buffer.Append("/libpath:",9);
            buffer.Append("\"",1);
            buffer.AppendString(argument->switch_info->values[0].text);
            buffer.Append("\"\n",2);
        }
        argument=argument->next;
    }
    if(entry_symbol && (entry_symbol->flag_byte & 1)) {
        text=get_symbol_name(entry_symbol,context->symbols->hash.string_storage,&name_kind,0);
        if(name_kind==1) text=strncpy(short_symbol_name,text,8);
        buffer.Append("/include:",9);
        buffer.AppendString(text);
        buffer.Append("\n",1);
    }
    if(context->pointer_list.suppress_default_libraries)
        buffer.Append("/nodefaultlib\n",14);
    else for(library=context->pointer_list.default_libraries;library;library=library->next) {
        if(library->flags & 4) {
            buffer.Append("/nodefaultlib:",14);
            buffer.AppendString(library->name);
            buffer.Append("\n",1);
        }
    }
    if(mac_target) {
        if(mac_current_import_version || mac_old_code_version!=-1 || mac_old_api_version!=-1) {
            comma=0;
            buffer.Append("/import:",8);
            if(mac_current_import_version) {
                comma=1;
                _itoa(mac_current_import_version,number,10);
                buffer.Append("currentver=",11);
                buffer.AppendString(number);
            }
            if(mac_old_code_version!=-1) {
                if(comma) buffer.Append(",",1);
                else comma=1;
                _itoa(mac_old_code_version,number,10);
                buffer.Append("oldcodever=",11);
                buffer.AppendString(number);
            }
            if(mac_old_api_version!=-1) {
                if(comma) buffer.Append(",",1);
                _itoa(mac_old_api_version,number,10);
                buffer.Append("oldapiver=",10);
                buffer.AppendString(number);
            }
            buffer.Append("\n",1);
        }
        if(context->specified_switches & 0x4000) {
            text=(const char *)context->mac_initializer_name;
            if(*text=='_') ++text;
            buffer.Append("/mac:init=",10);
            buffer.AppendString(text);
            buffer.Append("\n",1);
        }
        if(context->specified_switches & 0x10000) {
            text=(const char *)context->mac_terminator_name;
            if(*text=='_') ++text;
            buffer.Append("/mac:term=",10);
            buffer.AppendString(text);
            buffer.Append("\n",1);
        }
    }
    argument=link_files.first;
    for(index=0;index<link_files.count;++index) {
        buffer.Append("\"",1);
        buffer.AppendString(argument->name);
        buffer.Append("\"\n",2);
        argument=argument->next;
    }
    text=get_machine_name(context,context->file_header.Machine);
    buffer.Append("/machine:",9);
    buffer.AppendString(text);
    buffer.Append("\n",1);
    text=verbose_libraries?"/verbose\n":"/nologo\n";
    buffer.Append(text,strlen(text));
    if(xoff) buffer.Append("/xoff\n",6);
    if(context->options.debug_import_library && (context->options.debug_formats & 2))
        buffer.Append("/debugtype:cv\n",14);
    if(context->is_vxd==1) buffer.Copy((const BYTE *)"/vxd\n",5,NULL);
    write_argument_file(file,buffer.data,buffer.end-buffer.data);
    _splitpath(library_name,drive,directory,basename,NULL);
    _makepath(export_path,drive,directory,basename,"exp");
    offsets[0]=append_bytes(&command,"\"",1);
    length=strlen(_pgmptr);
    append_bytes(&command,_pgmptr,length);
    append_bytes(&command,"\"",2);
    offsets[1]=append_bytes(&command,"/lib",5);
    offsets[2]=append_bytes(&command,"@",1);
    append_bytes(&command,response,strlen(response)+1);
    for(index=0;index<3;++index) args[index]=(const char *)command.data+offsets[index];
    args[3]=NULL;
    close_argument_file(file,1);
    if(verbose_libraries) { fputc('\n',stdout); link_message(9004); }
    fflush(NULL);
    SetEnvironmentVariableA("LINK_REPRO_NAME","\\deflib.rsp");
    if(_spawnv(0,_pgmptr,args)) link_fatal(NULL,1141);
    if(verbose_libraries) { fputc('\n',stdout); link_message(9006); }
    if(!DeleteFileA(response)) link_fatal(NULL,1109,response);
    clear_buffer(&command);
    argument=export_arguments.first;
    for(index=0;index<export_arguments.count;++index) {
        if(argument->original_name) release_memory(argument->original_name);
        argument=argument->next;
    }
    export_argument.name=export_path;
    export_argument.original_name=export_path;
    context->LoadInputArgument(&export_argument,context->command_line_library);
    definition_export_state=0;
    if(incremental_enabled) {
        module=context->command_line_library->FindModule(export_path,NULL);
        if(_stat(module->name,&information)==-1) link_fatal(NULL,1104,module->name);
        module->timestamp=information.st_mtime;
        context->definition.export_module=module;
        context->definition.import_library_name=duplicate_link_string(library_name);
        if(_stat(library_name,&information)==-1) link_fatal(NULL,1104,library_name);
        context->definition.import_library_time=information.st_mtime;
    }
#ifdef RECON_NATIVE_TEST
    [[clang::noinline]] buffer.~DefinitionBuffer();
#else
    /* VC5 lexical inlining control preserves the actual success cleanup call;
     * the earlier return cleanups remain inline, and EH targets this outline. */
#pragma inline_depth(0)
#endif
    return 1;
}
#ifndef RECON_NATIVE_TEST
#pragma inline_depth()
#endif
