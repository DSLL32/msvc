#ifndef OPTIMIZER_CACHE_H
#define OPTIMIZER_CACHE_H
#include "work-optimizer.h"
/* The canonical tree cache is the work-value table. Its 24-byte records,
   hash heads and reverse creation chain retain one owner across phases. */
void clear_optimizer_trees(void);
#endif
