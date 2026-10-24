#ifndef MEMBER_REGISTRY_H
#define MEMBER_REGISTRY_H

#define REGISTRY_VERSION 3
#define REGISTRY_FEATURES 7
#define JOIN_RAW(a,b) a ## b
#define JOIN(a,b) JOIN_RAW(a,b)
#define TEXT_RAW(a) #a
#define TEXT(a) TEXT_RAW(a)
#define ACTION_TYPE(prefix,owner) typedef int (owner::*JOIN(prefix,Action))(int)
#define CHECK(code,condition) do { if (!(condition)) return code; } while (0)

#ifdef REGISTRY_FEATURES
#if defined(REGISTRY_VERSION) && ((REGISTRY_FEATURES & 3) == 3)
#define BASE_SEED 5
#else
#error Missing member registry features
#endif
#else
#error Missing registry configuration
#endif

#pragma pack(2)
struct RegistryHeader {
    char version;
    unsigned long bytes;
};
#pragma pack()

#endif
