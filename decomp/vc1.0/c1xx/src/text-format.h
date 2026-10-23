#ifndef TEXT_FORMAT_H
#define TEXT_FORMAT_H
#include <stdarg.h>
#include <recon.h>
void format_text(char *destination, char *format, ...);
void vformat_text(char *destination, char *format, va_list arguments);
#endif
