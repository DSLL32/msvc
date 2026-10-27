#include <windows.h>
#include <new.h>
#include <string.h>
#include "input.h"
#include "options.h"
#include "sections.h"
#include "full-link-functions.h"
#include "symbol-buffer.h"
#include <recon.h>

/* Ordinary lifetime with a separately qualified App destructor. The native
 * build retains the outline for DIRECT and inserts tracing after inlining. */
RECON_RETAIN inline ScopedBuffer::~ScopedBuffer()
{
    if(data) operator delete(data);
}

extern "C" {
extern char *current_module_strings,short_symbol_name[9];
extern StringNode **current_module_symbols;
extern BYTE incremental_enabled,incremental_reuse,mac_target,verbose_link;
BYTE undefined_list_changed;
extern DWORD weak_external_count,local_symbol_count;
extern int selected_tool_kind;
IMAGE_SYMBOL *__stdcall next_coff_symbol(IMAGE_SYMBOL **);
BOOL __stdcall mark_symbol_bit(DWORD *,DWORD);
void __stdcall collect_weak_external(StringTable *,IMAGE_SYMBOL **,IMAGE_SYMBOL *,StringNode *,OutputModule *,BYTE *,BYTE,DWORD);
void __stdcall report_duplicate_symbol(LinkOptions *,const char *,const char *,const char *);
void __stdcall record_undefined_comdat(StringNode *,const char *,OutputModule *);
int __stdcall has_incremental_symbol(void *,StringNode *);
int __stdcall update_incremental_symbol(const char *,IMAGE_SYMBOL *,StringNode *,BYTE);
void __stdcall process_mac_common(LinkContext *,StringNode *,IMAGE_SYMBOL *,const char *,BYTE,OutputModule *);
void __stdcall remember_common_symbol(CommonSymbol **,StringNode *);
void __stdcall clear_incremental_undefined(StringNode *,LinkContext *);
void __stdcall process_mac_undefined(LinkContext *,StringNode *,IMAGE_SYMBOL *,const char *,BYTE,OutputModule *);
void __stdcall define_input_symbol(StringNode *,Contribution *,DWORD,SHORT,WORD,DWORD,OutputModule *,StringTable *);
void __stdcall prepare_module_imports(OutputModule *);
char *__stdcall format_contribution_module(Contribution *,char *);

#pragma intrinsic(memcmp,strcpy)
void __stdcall process_module_symbols(LinkContext *context,OutputModule *module,
    void *parent,IMAGE_SYMBOL *symbols,DWORD flags,BYTE first_pass)
{
    char module_name[520],previous_name[520],previous_absolute_name[520];
    IMAGE_SYMBOL *cursor=symbols,*raw;
    StringNode *symbol;
    Contribution *contribution;
    const char *name;
    WORD kind,display_kind;
    DWORD index=0,value;
    SHORT section;
    BYTE created,is_import,any_import=0,define,aux_used;
    ScopedBuffer buffer;
    format_module_name(module,module_name);
    while(index!=module->symbol_count) {
        raw=next_coff_symbol(&cursor);
        aux_used=0;
        if(raw->StorageClass!=IMAGE_SYM_CLASS_EXTERNAL && raw->StorageClass!=0x69 &&
            raw->StorageClass!=0x44) {
            if(selected_tool_kind==1 && raw->SectionNumber==-1 &&
                raw->StorageClass==IMAGE_SYM_CLASS_STATIC &&
                !memcmp(raw->N.ShortName,"@comp.id",8))
                module->compiler_id=raw->Value;
            goto next_symbol;
        }
        define=0;
        created=0;
        name=get_symbol_name((StringNode *)raw,current_module_strings,&kind,&is_import);
        symbol=context->symbols->LookupSymbol(kind,name,&created);
        any_import |= is_import;
        /* Actual494578 gates Mac symbol bookkeeping (45f539/45f587/45f600).
         * The separate47f01c owner gates the incremental hooks below. */
        if(mac_target) {
            if(first_pass) {
                if(created) symbol->attributes |= 0x800;
            } else if(symbol->attributes & 0x800) {
                created=1;
                symbol->attributes &= ~0x800;
            }
        }
        current_module_symbols[index]=symbol;
        if(mac_target && selected_tool_kind==1) {
            module->symbol_indices[index]=(DWORD)symbol;
            mark_symbol_bit(module->symbol_bits[0],index);
        }
        if(incremental_reuse && !created && (symbol->flags & 0x100)) {
            created=1;
            symbol->reserved28=0;
            symbol->undefined_previous=NULL;
            symbol->undefined_next=NULL;
            symbol->flags=1;
            set_symbol_defined(symbol,0,context->symbols);
            if(mac_target) symbol->attributes=(symbol->attributes & 0xffbe)|0x800;
        }
        if(!created && (symbol->flags & 0xc0) && raw->StorageClass!=0x69) {
            symbol->flags &= ~0xc0U;
            --weak_external_count;
            undefined_list_changed=1;
        }
        if(created) {
            symbol->symbol.Type=raw->Type;
            if(parent) *(BYTE *)parent=1;
        }
        name=get_symbol_name(symbol,context->symbols->hash.string_storage,&display_kind,NULL);
        if(display_kind==1) name=strncpy(short_symbol_name,name,8);
        if(first_pass) {
            if(!(symbol->flags & 1) && !raw->Value && raw->StorageClass==0x69) {
                ++index;
                collect_weak_external(context->symbols,&cursor,raw,symbol,module,
                    (BYTE *)parent,created,flags);
                aux_used=1;
            }
            goto import_symbol;
        }
        section=raw->SectionNumber;
        if(section>0) {
            contribution=(Contribution *)module->contributions+section-1;
            if((selected_tool_kind==1 ||
                (selected_tool_kind==2 && context->options.librarian_skip_directive_symbols)) &&
                (contribution->characteristics & 0x200)) goto next_symbol;
            define=1;
            if(symbol->flags & 1) {
                if(symbol->flags & 2) {
                    if(incremental_enabled) record_undefined_comdat(symbol,name,module);
                    symbol->flags &= ~2U;
                } else {
                    if(symbol->contribution &&
                        (symbol->contribution->characteristics & 0x1000) &&
                        (contribution->characteristics & 0x1000)) {
                        if(incremental_enabled) {
                            record_symbol_reference(symbol,module);
                            record_incremental_include(module,symbol);
                        }
                    } else if(!mac_target || *name!='.') {
                        if(symbol->contribution)
                            format_contribution_module(symbol->contribution,previous_name);
                        else strcpy(previous_name,"a previous module");
                        if(incremental_reuse && (module->library->flags & 16)) {
                            if((symbol->contribution &&
                                !(symbol->contribution->module->library->flags & 16)) ||
                                has_incremental_symbol(context->incremental_symbols,symbol))
                                goto duplicate_incremental;
                        }
                        report_duplicate_symbol(&context->options,module_name,name,previous_name);
                    }
                    contribution=symbol->contribution;
                    define=0;
                }
            }
            value=raw->Value;
            if(incremental_reuse) {
                if(symbol->flags & 2) { incremental_status_code=8; return; }
                if(contribution->characteristics & 0x800) define=0;
                else if((contribution->module->library->flags & 16) &&
                    update_incremental_symbol(name,raw,symbol,created)) return;
            }
        } else {
            contribution=NULL;
            if(section==0) {
                if(raw->Value) {
                    if(incremental_enabled) {
                        process_mac_common(context,symbol,raw,name,created,module);
                        if(incremental_status_code) return;
                    }
                    if((symbol->flags & 1) && !(symbol->flags & 2)) goto define_symbol;
                    if(!(symbol->flags & 2)) {
                        remember_common_symbol(&module->common_symbols,symbol);
                        symbol->flags |= 2;
                    }
                    value=symbol->contribution ?
                        symbol->contribution->raw_size-symbol->contribution->padding :
                        symbol->symbol.Value;
                    if(raw->Value>value) { value=raw->Value; define=1; }
                } else {
                    if(incremental_reuse && (symbol->flags & 1))
                        clear_incremental_undefined(symbol,context);
                    record_symbol_reference(symbol,module);
                    if(incremental_enabled) record_incremental_include(module,symbol);
                    if(symbol->flags & 1) goto define_symbol;
                    if(incremental_reuse && created)
                        symbol->flags |= (raw->Type & 0x30)==0x20 ? 0x20000 : 0x40000;
                    if(raw->StorageClass==0x69) {
                        ++index;
                        collect_weak_external(context->symbols,&cursor,raw,symbol,module,
                            (BYTE *)parent,created,flags);
                        if(incremental_reuse && incremental_status_code) return;
                        aux_used=1;
                    }
                }
            } else if(section==-1) {
                value=raw->Value;
                define=1;
                if((symbol->flags & 1) && value!=symbol->symbol.Value) {
                    if(symbol->contribution)
                        format_contribution_module(symbol->contribution,previous_absolute_name);
                    else strcpy(previous_absolute_name,"a previous module");
                    report_duplicate_symbol(&context->options,module_name,name,previous_absolute_name);
                    define=0;
                }
                if(incremental_enabled) {
                    process_mac_undefined(context,symbol,raw,name,created,module);
                    if(incremental_status_code) return;
                }
            }
        }
    define_symbol:
        if(define) define_input_symbol(symbol,contribution,value,section,raw->Type,
            flags,module,context->symbols);
    import_symbol:
        if(is_import && contribution) contribution->rva=1;
    next_symbol:
        ++index;
        if(!first_pass && emit_symbol_class(raw->StorageClass,&context->options))
            local_symbol_count+=1+raw->NumberOfAuxSymbols;
        while(aux_used<raw->NumberOfAuxSymbols) {
            next_coff_symbol(&cursor);
            ++index;
            ++aux_used;
        }
    }
    if(selected_tool_kind==1 && any_import) prepare_module_imports(module);
    return;
 duplicate_incremental:
    if(verbose_link) link_error(NULL,6018,name);
    incremental_status_code=25;
}

}
