#include "parser-state.h"
#include "lexer.h"
#include <recon.h>

/* Genuine pending lexer dependencies, no body/extent credit. */
extern "C" unsigned __cdecl read_unspliced_character(void) { __asm { int 3 } return (unsigned)0; }
extern "C" int __cdecl skip_line_comment(void) { __asm { int 3 } return (int)0; }
extern "C" int __cdecl skip_block_comment(void) { __asm { int 3 } return (int)0; }
extern "C" unsigned __cdecl process_special_character(void) { __asm { int 3 } return (unsigned)0; }
extern "C" unsigned __cdecl process_trigraph(void) { __asm { int 3 } return (unsigned)0; }
extern "C" void __cdecl discard_input_character(void) { __asm { int 3 } }
extern "C" int __cdecl splice_input_line(void) { __asm { int 3 } return (int)0; }
extern "C" unsigned __cdecl resolve_special_character(unsigned) { __asm { int 3 } return (unsigned)0; }
extern "C" void __cdecl copy_quoted_literal(unsigned, int) { __asm { int 3 } }
extern "C" ParserTokenView * __cdecl make_character_constant(unsigned, const void *, unsigned) { __asm { int 3 } return (ParserTokenView *)0; }
extern "C" int __cdecl expand_macro(struct CompilerMacro *) { __asm { int 3 } return (int)0; }
extern "C" unsigned __cdecl collect_floating_token(char *) { __asm { int 3 } return (unsigned)0; }
extern "C" ParserTokenView * __cdecl scan_floating_token(char *) { __asm { int 3 } return (ParserTokenView *)0; }
extern "C" int __cdecl finish_forced_include(void) { __asm { int 3 } return (int)0; }
extern "C" unsigned __cdecl make_arena_token(unsigned) { __asm { int 3 } return (unsigned)0; }
unsigned RECON_THISCALL CompilerRecordedLexer::Peek(void) { __asm { int 3 } return 0; }

extern "C" int __cdecl skip_newline_comment(void) { __asm { int 3 } return 0; }

extern "C" void __cdecl finish_directive_line(const char *) { __asm { int 3 } }
extern "C" unsigned __cdecl read_directive_character(unsigned) { __asm { int 3 } return 0; }
extern "C" int __cdecl skip_conditional_group(int) { __asm { int 3 } return 0; }
extern "C" int __cdecl push_conditional_state(void) { __asm { int 3 } return 0; }
extern "C" __int64 __cdecl evaluate_preprocessor_condition(void) { __asm { int 3 } return 0; }
extern "C" void __cdecl begin_conditional_replay(void) { __asm { int 3 } }
extern "C" int __cdecl read_conditional_replay(void) { __asm { int 3 } return 0; }
extern "C" int __cdecl record_conditional_value(short) { __asm { int 3 } return 0; }
extern "C" void __cdecl process_define_directive(int) { __asm { int 3 } }
extern "C" ParserTokenView *__cdecl process_line_directive(void) { __asm { int 3 } return NULL; }
extern "C" char *__cdecl parse_include_directive(int, unsigned *) { __asm { int 3 } return NULL; }
extern "C" ParserTokenView *__cdecl include_directive_file(const char *, unsigned) { __asm { int 3 } return NULL; }
extern "C" char *__cdecl parse_import_directive(char **, unsigned *) { __asm { int 3 } return NULL; }
extern "C" char *__cdecl resolve_import_directive(const char *, const char *, unsigned) { __asm { int 3 } return NULL; }
extern "C" void __cdecl process_undef_directive(void) { __asm { int 3 } }
extern "C" BYTE __cdecl advance_multibyte_source(void) { __asm { int 3 } return 0; }

extern "C" int __cdecl scan_identifier_span(const BYTE *, unsigned, unsigned) { __asm { int 3 } return 0; }

extern "C" ParserTokenView *__cdecl preprocess_pragma_directive(void) { __asm { int 3 } return NULL; }
extern "C" ParserTokenView *__cdecl process_implementation_key_pragma(void) { __asm { int 3 } return NULL; }
extern "C" void __cdecl process_start_map_region_pragma(void) { __asm { int 3 } }
extern "C" void __cdecl process_include_alias_pragma(void) { __asm { int 3 } }

extern "C" void __cdecl skip_to_directive_newline(ParserTokenView *) { __asm { int 3 } }




extern "C" CompilerType * __cdecl select_based_literal_type(unsigned __int64 *) { __asm { int 3 } return 0; }

extern "C" CompilerType * __cdecl select_signed_wide_literal_type(unsigned __int64 *) { __asm { int 3 } return 0; }

extern "C" CompilerType * __cdecl select_unsigned_literal_type(unsigned __int64 *) { __asm { int 3 } return 0; }

extern "C" CompilerType * __cdecl select_unsigned_wide_literal_type(unsigned __int64 *) { __asm { int 3 } return 0; }

extern "C" CompilerType * __cdecl select_sized_literal_type(int) { __asm { int 3 } return 0; }



extern "C" CompilerType *__cdecl select_extended_decimal_type(unsigned __int64 *) { __asm { int 3 } return 0; }


/* Wider pragma helpers retain genuine entry traps. */
extern "C" ParserTokenView * __cdecl read_optional_pragma_value(unsigned) { __asm { int 3 } return NULL; }
extern "C" ParserTokenView * __cdecl read_required_pragma_value(unsigned) { __asm { int 3 } return NULL; }
extern "C" ParserTokenView * __cdecl read_pragma_string(void) { __asm { int 3 } return NULL; }
extern "C" ParserTokenView * __cdecl read_pragma_switch(void) { __asm { int 3 } return NULL; }
extern "C" CompilerSymbol * __cdecl find_or_create_segment(CompilerNameEntry *, CompilerNameEntry *, unsigned, unsigned) { __asm { int 3 } return NULL; }
extern "C" void __cdecl select_code_segment(unsigned) { __asm { int 3 }  }
extern "C" void __cdecl prepare_code_segment(unsigned) { __asm { int 3 }  }
extern "C" void __cdecl enable_minimal_rebuild_component(void) { __asm { int 3 }  }
extern "C" void __cdecl enable_minimum_type_info_component(void) { __asm { int 3 }  }
extern "C" void __cdecl enable_browser_component(void) { __asm { int 3 }  }
extern "C" void __cdecl set_member_pointer_generality(unsigned) { __asm { int 3 }  }
extern "C" void __cdecl set_member_pointer_inheritance(unsigned) { __asm { int 3 }  }
extern "C" void __cdecl set_init_segment_function(CompilerNameEntry *) { __asm { int 3 }  }
extern "C" void __cdecl mark_precompiled_header_stop(void **) { __asm { int 3 }  }
extern "C" void __cdecl apply_option_flags(unsigned) { __asm { int 3 }  }
extern "C" void __cdecl print_pragma_message(const char *) { __asm { int 3 }  }
extern "C" void __cdecl read_pragma_identifier_list(void (__cdecl *)(CompilerNameEntry *)) { __asm { int 3 }  }
extern "C" void __cdecl mark_function_nonintrinsic(CompilerNameEntry *) { __asm { int 3 }  }
extern "C" void __cdecl mark_function_intrinsic(CompilerNameEntry *) { __asm { int 3 }  }
extern "C" void __cdecl assign_allocated_text_segment(CompilerNameEntry *) { __asm { int 3 }  }
