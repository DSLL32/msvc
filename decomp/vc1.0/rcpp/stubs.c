/* Named function frontiers; remove each body as its function is recovered. */
#include "numbers.h"
void rc_exit(int result) { __asm int 3 }
void rc_cexit(int result) { __asm int 3 }
int rc_exception_filter(unsigned code, EXCEPTION_POINTERS *info) { __asm { int 3 } return 0; }

void rc_runtime_abort(int error) { __asm int 3 }


char *__stdcall rc_message(unsigned code) { __asm { int 3 } return 0; }
void __stdcall rc_fatal(unsigned code) { __asm int 3 }
BOOL WINAPI rc_control_handler(DWORD type) { __asm { int 3 } return 0; }
void rc_emit_tokens(void) { __asm int 3 }

int __stdcall rc_match_suffix(const char *pattern, const char *value) { __asm { int 3 } return 0; }

unsigned __stdcall rc_parse_option_number(char *value) { __asm { int 3 } return 0; }


int __stdcall rc_compare_macro_text(char *a, char *b, unsigned count) { __asm { int 3 } return 0; }
void __stdcall rc_error(unsigned code) { __asm int 3 }
void __stdcall rc_warning(unsigned code) { __asm int 3 }





int rc_splice_line(void) { __asm { int 3 } return 0; }


void rc_skip_line(void) { __asm int 3 }
int rc_evaluate_condition(void) { __asm { int 3 } return 0; }
int __stdcall rc_skip_inactive(int mode) { __asm { int 3 } return 0; }
int rc_push_condition(void) { __asm { int 3 } return 0; }
void rc_parse_define(void) { __asm int 3 }
void rc_parse_pragma(void) { __asm int 3 }
void rc_remove_macro(void) { __asm int 3 }

unsigned rc_resume_input(void) { __asm { int 3 } return 0; }

unsigned char rc_close_include(void) { __asm { int 3 } return 0; }
int __stdcall rc_expand_macro(RcMacro *macro) { __asm { int 3 } return 0; }
void __stdcall rc_copy_quoted_text(unsigned char quote) { __asm int 3 }
unsigned char rc_parse_character(void) { __asm { int 3 } return 0; }
unsigned char __stdcall rc_read_floating(char *out) { __asm { int 3 } return 0; }
int __stdcall rc_read_expanded_identifier(unsigned char first) { __asm { int 3 } return 0; }





unsigned __stdcall rc_escape_value(int ch) { __asm { int 3 } return 0; }
