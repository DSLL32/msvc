/* Sorted external-symbol diagnostics and COFF symbol emission. */
#include "link-pass.h"

void emit_externals(SymbolTable *table,unsigned char defined,unsigned char emit,
                    unsigned char name_order,ImageOptions *options)
{
    External **entries,*external;
    unsigned long remaining;
    unsigned char is_defined,referenced;
    IMAGE_SYMBOL symbol;
    ReferenceEnumerator references;
    const char *name;
    char *display,module_name[MAX_PATH];

    entries=name_order ? get_externals_by_name(table) : get_externals_by_address(table);
    remaining=count_externals(table);
    while (remaining--) {
        external=*entries++;
        if (external->flags&(EXTERNAL_IGNORED|EXTERNAL_NO_OUTPUT)) continue;
        is_defined=(external->flags&EXTERNAL_DEFINED)!=0;
        if (emit) {
            if (is_defined && external->owner.contribution &&
                options->eliminate_unreferenced &&
                contribution_discarded(external->owner.contribution)) continue;
            if (is_defined!=defined || (external->flags&EXTERNAL_EMITTED)) continue;
            if (!emit_symbol_class(external->symbol.StorageClass,options)) continue;
            symbol=external->symbol;
            symbol.Value=external->value.final_value;
            if (external->owner.contribution)
                symbol.SectionNumber=external->owner.contribution->group->section->index;
            write_coff_symbol(output_file,&symbol);
            if (external->flags&EXTERNAL_DEFINED)
                external->state.symbol_index=++symbol_count;
            external->flags |= EXTERNAL_EMITTED;
        } else {
            if (is_defined!=defined || (external->flags&EXTERNAL_EMITTED)) continue;
            if (external->symbol.N.Name.Short)
                name=strncpy(short_symbol_name,(const char *)&external->symbol,8);
            else name=table->strings.data+external->symbol.N.Name.Long;
            if (!defined && !external->symbol.Value) {
                display=undecorate_symbol_name(name,1);
                referenced=0;
                initialize_reference_enumerator(&references,external);
                while (next_reference(&references)) {
                    referenced=1;
                    warning_message(format_module_name(references.current,module_name),
                                    0x57,display,"");
                }
                if (!referenced) warning_message(program_name,0x57,display,"");
                if (external->symbol.N.Name.Short)
                    name=strncpy(short_symbol_name,(const char *)&external->symbol,8);
                else name=table->strings.data+external->symbol.N.Name.Long;
                if (display!=name) free(display);
                ++undefined_symbols;
            } else printf("%8lX %s\n",external->symbol.Value,name);
        }
    }
}

void emit_string_table(SymbolTable *table)
{
    unsigned long empty=0;
    if (table->strings.data) {
        *(unsigned long *)table->strings.data=table->strings.length;
        write_file(output_file,table->strings.data,table->strings.length);
    } else write_file(output_file,&empty,sizeof(empty));
}
