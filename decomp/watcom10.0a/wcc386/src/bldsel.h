#ifndef WCC_BLDSEL_H
#define WCC_BLDSEL_H
#include <stddef.h>
#include <recon.h>
struct code_lbl;
struct address_name;
#pragma pack(4)
typedef struct select_list {
    struct select_list *next;
    long low,high;
    unsigned count;
    struct code_lbl *label;
} select_list;
typedef struct select_node {
    select_list *list;
    struct code_lbl *other_wise;
    long lower,upper;
    unsigned num_cases;
} select_node;
#pragma pack()
typedef char old_select_list_size[(sizeof(select_list)==20)?1:-1];
typedef char old_select_node_size[(sizeof(select_node)==20)?1:-1];
typedef char old_select_node_count[(offsetof(select_node,num_cases)==16)?1:-1];
select_node *BGSelInit(void);
void BGSelect(select_node *node,struct address_name *value);
void BGSelOther(select_node *node,struct code_lbl *label);
void BGSelCase(select_node *node,struct code_lbl *label,long value);
void BGSelRange(select_node *node,long low,long high,struct code_lbl *label);
#endif
