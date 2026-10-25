#ifndef DEBUG_INFO_H
#define DEBUG_INFO_H
#include "frontend.h"
#include <recon.h>
typedef struct DebugDatabase DebugDatabase;
extern DebugDatabase *debug_database;
extern unsigned char *line_record_buffer, *line_record_leaf;
extern unsigned char *debug_record_buffer;
DebugDatabase *__cdecl open_debug_database(char *filename, char *mode, unsigned signature);
unsigned short __cdecl hash_debug_database_record(unsigned char *record);
unsigned short __cdecl debug_database_intern(DebugDatabase *database, unsigned char *record);
unsigned __cdecl debug_database_signature(DebugDatabase *database);
unsigned __cdecl debug_database_age(DebugDatabase *database);
unsigned short copy_line_record_name(unsigned char *destination, char *name);
unsigned short pad_line_record(unsigned char *destination, unsigned short length);
void initialize_line_record_buffer(void);
void emit_line_record_database_reference(void);
extern unsigned debug_stream_offset;
void write_debug_record(unsigned record, ...);
void begin_line_record_debug_info(void);
void finish_line_record_debug_info(void);
#endif
