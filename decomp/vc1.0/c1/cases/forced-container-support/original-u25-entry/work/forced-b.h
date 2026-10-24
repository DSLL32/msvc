#ifndef FORCED_B_H
#define FORCED_B_H
#define CONTAINER_APPLY(fn,item,bias) ((fn)((item),(bias)))
#define CONTAINER_SELECT(value) ((value) ? CONTAINER_BIAS : -CONTAINER_BIAS)
typedef struct ContainerSummary {
    unsigned count;
    long sum;
    ContainerCallback callback;
} ContainerSummary;
#endif
