#ifndef BROWSER_H
#define BROWSER_H
#include "source-input.h"
#include <recon.h>
/* Original ring entries are 24 bytes. The common header is also used by
   queue coalescing; bytes 1 and 23 are ordinary structure padding. */
typedef struct BrowserRecordHeader {
    unsigned char kind;
    unsigned short flags;
} BrowserRecordHeader;
typedef struct BrowserRecord {
    BrowserRecordHeader header;
    unsigned line;
    unsigned symbol_index;
    char *name;
    char *name_identity;
    unsigned short attributes;
    unsigned char category;
} BrowserRecord;
typedef char BrowserRecordLayoutMustMatch[
    (sizeof(BrowserRecord) == 24 &&
     offsetof(BrowserRecordHeader, flags) == 2 &&
     offsetof(BrowserRecord, line) == 4 &&
     offsetof(BrowserRecord, symbol_index) == 8 &&
     offsetof(BrowserRecord, name) == 12 &&
     offsetof(BrowserRecord, name_identity) == 16 &&
     offsetof(BrowserRecord, attributes) == 20 &&
     offsetof(BrowserRecord, category) == 22) ? 1 : -1];
extern BrowserRecord browser_records[32];
extern int browser_record_head;
extern int browser_records_suppressed;
void __stdcall fill_browser_record(BrowserRecord *record, void *subject);
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
