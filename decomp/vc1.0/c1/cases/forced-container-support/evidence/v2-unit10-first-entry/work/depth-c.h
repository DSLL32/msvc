#ifndef DEPTH_C_H
#define DEPTH_C_H
static int container_deep_score(const ContainerItem *item, int bias)
{
    return item->key + bias + (item->enabled ? (int)item->group : 0);
}
enum ContainerHeaderState { HEADER_IDLE, HEADER_READY, HEADER_COMPLETE };
struct ContainerHeaderStamp { unsigned short version; enum ContainerHeaderState state; };
#endif
