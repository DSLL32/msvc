/* Named function frontiers; remove each body as its function is recovered. */
#include "macros.h"
int rc_exception_filter(unsigned code, EXCEPTION_POINTERS *info) { __asm { int 3 } return 0; }

void rc_runtime_abort(int error) { __asm int 3 }

char *__stdcall rc_message(unsigned code) { __asm { int 3 } return 0; }
void __stdcall rc_fatal(unsigned code) { __asm int 3 }
BOOL WINAPI rc_control_handler(DWORD type) { __asm { int 3 } return 0; }

int __stdcall rc_match_suffix(const char *pattern, const char *value) { __asm { int 3 } return 0; }

unsigned __stdcall rc_parse_option_number(char *value) { __asm { int 3 } return 0; }

void __stdcall rc_error(unsigned code) { __asm int 3 }
void __stdcall rc_warning(unsigned code) { __asm int 3 }

void rc_skip_line(void) { __asm int 3 }

void rc_parse_pragma(void) { __asm int 3 }

unsigned char rc_parse_character(void) { __asm { int 3 } return 0; }
unsigned char __stdcall rc_read_floating(char *out) { __asm { int 3 } return 0; }

unsigned __stdcall rc_escape_value(int ch) { __asm { int 3 } return 0; }
