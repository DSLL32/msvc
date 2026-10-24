#ifndef BROWSER_H
#define BROWSER_H
#include "source-input.h"
#include <recon.h>
/* First encountered common header only. No payload size/ownership is
   claimed here; append/fill recovery will define the complete ring record. */
typedef struct BrowserRecordHeader {
    unsigned char kind;
    unsigned short flags;
} BrowserRecordHeader;
typedef char BrowserRecordFlagsOffsetMustBe2[
    (offsetof(BrowserRecordHeader, flags) == 2) ? 1 : -1];
extern int browser_record_count;
void __stdcall append_browser_record(unsigned char kind, void *subject, unsigned line);
BrowserRecordHeader *first_browser_record(void);
BrowserRecordHeader *second_browser_record(void);
BrowserRecordHeader *__stdcall browser_record_from_end(int index);
BrowserRecordHeader *pop_last_browser_record(void);
BrowserRecordHeader *pop_first_browser_record(void);
void __stdcall emit_browser_record(BrowserRecordHeader *record);
void begin_browser_output(void);
void __stdcall format_browser_temporary_name(char *filename);
void begin_browser_records(void);
void __stdcall queue_browser_record(unsigned char kind, void *subject);
#endif
