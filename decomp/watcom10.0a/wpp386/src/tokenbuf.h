#ifndef WPP_TOKENBUF_H
#define WPP_TOKENBUF_H
#include <recon.h>
void MacroTokenBufferInit(void);
void MacroTokenBufferAddString(const char *name);
unsigned MacroTokenBufferSize(void);
char *MacroTokenBufferStart(void);
void MacroTokenBufferAddChar(int token);
void MacroTokenBufferAddConstant(void);
void MacroTokenBufferAddValue(unsigned long value);
void MacroTokenBufferGet(char *target);
void MacroTokenBufferRestore(char *source, unsigned size);
void MacroTokenBufferRemoveWhiteSpace(void);
#endif
