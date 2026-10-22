#ifndef PCH_H
#define PCH_H
#include "initialization.h"

/* Encountered PCH event prefix; payload bytes immediately follow it. */
typedef struct PchRecord {
    unsigned kind, payload_bytes, value;
} PchRecord;
extern PchRecord *pch_record_cursor;
extern short pch_include_count;
extern unsigned short pch_include_page_count;
extern int pch_macro_changed, pch_rebuild_on_macro_change;
void pch_record_string(unsigned kind, unsigned value, char *text);
int __stdcall pch_macro_defined(char *name);
int pch_replay_mismatch(void);
#endif
