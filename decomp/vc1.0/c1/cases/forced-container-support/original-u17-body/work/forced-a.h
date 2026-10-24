#ifndef FORCED_A_H
#define FORCED_A_H
#define CONTAINER_COUNT 4
#define CONTAINER_BIAS 3
typedef struct ContainerItem ContainerItem;
struct ContainerItem {
    unsigned short key;
    unsigned enabled : 1;
    unsigned group : 3;
    const char *name;
    double weight;
};
typedef int (*ContainerCallback)(const ContainerItem *, int);
#include "depth-a.h"
#endif
