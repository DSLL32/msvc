#ifndef RECONSTRUCTION_PCH_H
#define RECONSTRUCTION_PCH_H
#define PCH_SCALE 7
#define PCH_VALUE(x) ((x) * PCH_SCALE + 3)
typedef enum PchKind { PCH_SMALL = 2, PCH_LARGE = 5 } PchKind;
typedef struct PchItem {
    int value;
    PchKind kind;
    const char *name;
} PchItem;
typedef int (__cdecl *PchTransform)(const PchItem *item);
extern int __cdecl pch_transform(const PchItem *item);
#endif
