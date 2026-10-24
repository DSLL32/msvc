#ifndef CV510_SOURCE_INDEX_H
#define CV510_SOURCE_INDEX_H
#include "pack.h"
#include <recon.h>

typedef struct SourceModuleIndex {
    unsigned short module, files;
    struct SourceModuleIndex *next;
} SourceModuleIndex;
typedef struct SourceNameIndex {
    const unsigned char *name;
    unsigned offset;
    struct SourceNameIndex *next;
    unsigned reserved;
} SourceNameIndex;
SourceNameIndex *append_source_name(const unsigned char *name, SourceNameIndex *tail, unsigned offset);
void build_source_file_index(void);
#endif
