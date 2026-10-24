#ifndef METADATA_INTERFACE_H
#define METADATA_INTERFACE_H
typedef struct MetadataRecord {
    struct MetadataRecord *next;
    unsigned kind : 5;
    unsigned flags : 3;
    long value;
    const char *name;
    double weight;
} MetadataRecord;
typedef int (__cdecl *MetadataVisit)(const MetadataRecord *, unsigned);
extern int metadata_sink(const MetadataRecord *, unsigned, double);
extern const MetadataRecord metadata_seed;
extern const volatile unsigned metadata_qualified;
extern unsigned metadata_mutable;
#endif
