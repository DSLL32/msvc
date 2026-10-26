#ifndef WCC_REGSAVE_H
#define WCC_REGSAVE_H
#include <recon.h>
typedef unsigned save_def;
#define SAVE_CLASS_COUNT 11
struct savings {
    save_def index_save;
    save_def load_cost[SAVE_CLASS_COUNT],store_cost[SAVE_CLASS_COUNT];
    save_def use_save[SAVE_CLASS_COUNT],def_save[SAVE_CLASS_COUNT];
    save_def push_cost[SAVE_CLASS_COUNT],pop_cost[SAVE_CLASS_COUNT];
    save_def loop_weight[6];
};
extern struct savings Save;
void AdjTimeSize(unsigned *time,unsigned *size);
void SetLoopCost(unsigned time);
void SetCost(save_def *cost,save_def value);
#endif
