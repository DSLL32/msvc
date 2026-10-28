#include "functions.h"
#include "code-records.h"
#include "function-reader.h"
#include "output-callbacks.h"
#include <recon.h>

extern int c2_assembly_enabled, c2_mixed_assembly;
int assembly_output_active;
unsigned char assembly_comment_character;

void __fastcall initialize_source_output(char *source)
{
    assembly_output_active = c2_assembly_enabled || c2_mixed_assembly ? 1 : 0;
    if (assembly_output_active) {
        assembly_comment_character = get_assembly_comment_character();
        write_assembly_header(source);
    }
}

void write_assembly_symbols(void)
{
    if (assembly_output_active) {
        visit_symbols(0, assembly_callback_4894ca);
        visit_symbols(0, assembly_callback_489596);
        visit_symbols(1, assembly_callback_489596);
        visit_symbols(0, assembly_callback_489693);
        visit_symbols(1, assembly_callback_489693);
        visit_symbols(0, assembly_callback_48a251);
    }
}

void __fastcall write_assembly_records(C2DebugLink *records)
{
    if (assembly_output_active) {
        for (; records; records = records->next)
            write_assembly_initializer(lookup_initializer(records->symbol->serial));
    }
}

extern int generate_prologue, generate_frame, input_scope_active;
extern unsigned diagnostic_position;
extern char *input_current_filename;
extern C2Section *function_debug_section;
/* These scalar controls are distinct from the ordinary listing-enabled flag.
 * The actual MASM output initializer owns the alternate-syntax producer. */
int assembly_alternate_syntax; /*49e28c: initialized0*/
unsigned assembly_column; /*49dde8: initialized0*/
/*49e6ac is in the zero-filled virtual tail of.data, not file-backed data.
 * Its current-section identity is returned by the actual section callbacks. */
C2Section *assembly_current_section;
void __fastcall initialize_assembly_function(C2Function *function) { __asm { int 3 } }
void __fastcall begin_assembly_function(C2Function *function) { __asm { int 3 } }
void __fastcall end_assembly_function(C2Function *function) { __asm { int 3 } }
int update_assembly_source(void) { __asm { int 3 } return 0; }
void __fastcall write_assembly_source_line(unsigned line) { __asm { int 3 } }
void __fastcall write_assembly_declaration(C2InputSymbol *declaration) { __asm { int 3 } }
C2Expression *__fastcall write_assembly_instruction(C2Expression *record, unsigned *offset)
{ __asm { int 3 } return NULL; }
C2Section *__fastcall begin_assembly_section(C2Section *section, int attributes)
{ __asm { int 3 } return NULL; }
C2Section *__fastcall end_assembly_section(C2Section *section)
{ __asm { int 3 } return NULL; }
void __fastcall finish_function_output_state(C2Function *function)
{
    C2Expression *record, *next;
    C2InputSymbol *declaration;
    C2Section *section;
    unsigned offset, previous_line;
    int alternate_section = 0;
    int saved_prologue = generate_prologue, saved_frame = generate_frame;
    /* The original caches the printf IAT pointer for the whole record walk. */
    int (__cdecl *print)(const char *, ...);
    if (!assembly_output_active) return;
    if (assembly_alternate_syntax) initialize_assembly_function(function);
    offset = (unsigned)-1;
    input_scope_active = 0;
    previous_line = diagnostic_position = 0;
    print = printf;
    for (record = (C2Expression *)function->flow->last->records; record; record = next) {
        if (record->flags & 1) {
            if (record->location.line) {
                diagnostic_position = resolve_diagnostic_location(
                    function->source_line_base + record->location.line, &input_current_filename);
                if (update_assembly_source()) previous_line = 0;
            }
            if (previous_line != diagnostic_position) {
                previous_line = diagnostic_position;
                if (assembly_alternate_syntax) write_assembly_source_line(diagnostic_position);
                else {
                    print("; Line %ld", diagnostic_position);
                    assembly_column = 0;
                    print("\n");
                }
            }
            next = write_assembly_instruction(record, &offset)->next;
        } else {
            next = record->next;
            switch (record->kind) {
            case 22:
                begin_assembly_function(function);
                offset = current_symbol->parameter_offset;
                if (record->location.line) {
                    diagnostic_position = resolve_diagnostic_location(
                        function->source_line_base + record->location.line, &input_current_filename);
                    if (update_assembly_source()) previous_line = 0;
                }
                break;
            case 23:
                if (alternate_section)
                    assembly_current_section = end_assembly_section(assembly_current_section);
                end_assembly_function(function);
                break;
            case 25:
                declaration = record->declaration;
                if ((declaration->symbol.source_kind == 0x54 ||
                     declaration->symbol.source_kind == 0x55) && !alternate_section) {
                    generate_prologue = generate_frame = 0;
                    alternate_section = 1;
                    section = function_debug_section;
                    if (current_symbol->flags & 0x20)
                        section = select_symbol_section((C2FunctionSymbol *)current_symbol,
                            (C2Symbol *)section);
                    assembly_current_section = begin_assembly_section(section, 0);
                }
                offset = declaration->symbol.parameter_offset;
                if (record->location.line) {
                    diagnostic_position = resolve_diagnostic_location(
                        function->source_line_base + record->location.line, &input_current_filename);
                    if (update_assembly_source()) previous_line = 0;
                }
                if (previous_line != diagnostic_position) {
                    previous_line = diagnostic_position;
                    if (assembly_alternate_syntax) write_assembly_source_line(diagnostic_position);
                    else {
                        print("; Line %ld", diagnostic_position);
                        assembly_column = 0;
                        print("\n");
                    }
                }
                write_assembly_declaration(record->declaration);
                print(":");
                assembly_column = 0;
                print("\n");
                break;
            }
        }
    }
    input_scope_active = 1;
    generate_prologue = saved_prologue;
    generate_frame = saved_frame;
    reset_input_pool(4);
}

/* The MASM source-file cache at49e6c0 holds at most five pointers: its
 * producer evicts entry0 when count reaches5 before appending. Only the
 * FILE member at+8 is used here; the preceding payload remains opaque.
 * The count and five pointer cells lie in the original .data virtual
 * zero-fill tail [49e400,49e700), independently of any live cache state. */
typedef struct C2AssemblySourceFile {
    unsigned char opaque[8];
    FILE *file;
} C2AssemblySourceFile;
int assembly_source_file_count; /*49e4b0*/
C2AssemblySourceFile *assembly_source_files[5]; /*49e6c0..49e6d4*/
extern int c2_assembly_redirected; /* existing49e2f4 option/cleanup owner */
void finish_assembly_output(void) { __asm { int 3 } }

void c2_finalize_output(void)
{
    int i;
    int (__cdecl *close_file)(FILE *);
    if (!assembly_output_active)
        return;
    finish_assembly_output();
    if (assembly_alternate_syntax && assembly_source_file_count > 0) {
        /* No wider cache domain has been reconstructed. */
        if (assembly_source_file_count > 5) { __asm { int 3 } }
        close_file = fclose;
        for (i = 0; i < assembly_source_file_count; ++i)
            close_file(assembly_source_files[i]->file);
    }
    c2_assembly_redirected = 0;
}
