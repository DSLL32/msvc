#ifndef TEXT_FORMAT_H
#define TEXT_FORMAT_H
#include <stdarg.h>
#include <stdio.h>
#include <recon.h>
void format_text(char *destination, char *format, ...);
void vformat_text(char *destination, char *format, va_list arguments);
unsigned write_formatted_text(FILE *stream, char *format, ...);
#endif
