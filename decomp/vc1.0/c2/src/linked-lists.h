#ifndef LINKED_LISTS_H
#define LINKED_LISTS_H
#include <recon.h>
/* Generic algorithms require only a next pointer at offset zero. */
typedef struct LinkedListLink {
    struct LinkedListLink *next;
} LinkedListLink;
typedef int (__fastcall *LinkedListComparison)(void *left, void *right);
void *__fastcall sort_linked_list(void *head, LinkedListComparison compare);
void *__fastcall merge_sorted_lists(void *left, void *right, LinkedListComparison compare);
#endif
