#ifndef LEGACY_TYPES_H
#define LEGACY_TYPES_H
#define LEGACY_COUNT 4
#if LEGACY_COUNT > 2
#define LEGACY_MASK 7U
#else
#define LEGACY_MASK 3U
#endif
typedef struct LegacyRecord LegacyRecord;
extern char *record_base;
typedef LegacyRecord __based(record_base) *NodeLink;
#pragma pack(2)
struct LegacyRecord {
    unsigned short key;
    NodeLink next;
    unsigned flags : 3;
    unsigned state : 5;
    union { long integer; float scale; } value;
};
#pragma pack()
typedef int (__far __pascal *LegacyCallback)(LegacyRecord *, int);
#endif
