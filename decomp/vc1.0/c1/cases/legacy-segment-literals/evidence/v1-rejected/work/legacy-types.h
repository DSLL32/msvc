#ifndef LEGACY_TYPES_H
#define LEGACY_TYPES_H
#define LEGACY_COUNT 4
#if LEGACY_COUNT > 2
#define LEGACY_MASK 7U
#else
#define LEGACY_MASK 3U
#endif
typedef struct LegacyRecord LegacyRecord;
typedef LegacyRecord __based((__segment)__self) *SelfLink;
#pragma pack(2)
struct LegacyRecord {
    unsigned short key;
    SelfLink next;
    unsigned flags : 3;
    unsigned state : 5;
    union { long integer; float scale; } value;
};
#pragma pack()
typedef int (__far __pascal *LegacyCallback)(LegacyRecord __far *, int);
#endif
