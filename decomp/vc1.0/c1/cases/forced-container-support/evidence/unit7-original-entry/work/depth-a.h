#ifndef DEPTH_A_H
#define DEPTH_A_H
#define CONTAINER_JOIN_RAW(a,b) a##b
#define CONTAINER_JOIN(a,b) CONTAINER_JOIN_RAW(a,b)
#include "depth-b.h"
#endif
