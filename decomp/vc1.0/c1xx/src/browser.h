#ifndef BROWSER_H
#define BROWSER_H
#include "source-input.h"
#include <recon.h>
void begin_browser_output(void);
void __stdcall format_browser_temporary_name(char *filename);
void begin_browser_records(void);
void __stdcall queue_browser_record(unsigned char kind, void *subject);
#endif
