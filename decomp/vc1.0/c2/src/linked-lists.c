/* Stable ordering of compiler-owned linked records. */
#include "linked-lists.h"
#include <recon.h>

/* Original [00059350,00059440). Bottom-up stable merge sort for
   records whose first field is their next pointer. Runs double each pass. */
void *__fastcall sort_linked_list(void *list, LinkedListComparison compare)
{
    LinkedListLink *head = list, *right = 0, *last = 0;
    LinkedListLink *run, *left_tail, *right_tail;
    LinkedListLink **link;
    int width = 1, i;
    if (!head) return head;
    do {
        if (head == right) break;
        run = head;
        link = &head;
        while (run) {
            last = run;
            left_tail = run;
            for (i = 1; left_tail->next && i < width; ++i)
                left_tail = left_tail->next;
            right = left_tail->next;
            if (!right) {
                *link = last;
                right = 0;
                break;
            }
            left_tail->next = 0;
            right_tail = right;
            for (i = 1; right_tail->next && i < width; ++i)
                right_tail = right_tail->next;
            run = right_tail->next;
            right_tail->next = 0;
            *link = merge_sorted_lists(last, right, compare);
            if (!run) break;
            for (i = width * 2; i > 0; --i)
                link = &(*link)->next;
        }
        width += width;
    } while (head != last);
    return head;
}

/* Original [00059440,000594a0). The comparison callback uses ECX/EDX;
   this helper receives that callback as its one callee-popped stack argument. */
void *__fastcall merge_sorted_lists(void *left_list, void *right_list,
    LinkedListComparison compare)
{
    LinkedListLink *left = left_list, *right = right_list, *head;
    LinkedListLink **tail = &head;
    while (left && right) {
        if (compare(left, right) <= 0) {
            *tail = left;
            tail = &left->next;
            left = left->next;
        } else {
            *tail = right;
            tail = &right->next;
            right = right->next;
        }
    }
    *tail = right ? right : left;
    return head;
}
