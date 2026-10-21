/* Named function frontiers; remove each body as its function is recovered. */
#include "lexer.h"
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





int rc_skip_comment(void) { __asm { int 3 } return 0; }
int rc_splice_line(void) { __asm { int 3 } return 0; }
void rc_parse_directive(void) { __asm int 3 }
