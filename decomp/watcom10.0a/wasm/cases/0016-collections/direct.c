#include <string.h>
#include <recon-tool.h>
#include "wasm.h"
#include <recon.h>
#define CHECK(x) do { if (!(x)) return 0; } while (0)
int collection_smoke(void) { return FixDup(NULL) == NULL; }
struct walk_state { unsigned count,sum,stop; };
static int sum_element(void *element,void *parameter)
{
    struct walk_state *state=parameter;
    ++state->count;state->sum+=*(unsigned *)element;
    return state->stop && state->count == state->stop ? -1 : 0;
}
static int array_owners(void)
{
    unsigned def=7,*first,*middle,*last;
    array_hdr *array;
    struct walk_state state;
    array=ArrCreate(sizeof(def),&def);CHECK(array != NULL);
    CHECK(array->def == &def && array->block == NULL && array->num_elms == 0);
    memset(&state,0,sizeof(state));CHECK(ArrWalk(array,&state,sum_element) == 0 && state.count == 0);
    last=ArrNewElm(array,130);CHECK(last != NULL && *last == 7);
    first=ArrAccess(array,0);middle=ArrAccess(array,64);
    CHECK(*first == 7 && *middle == 7 && array->num_elms == 131);
    CHECK(array->block != NULL && array->block->next != NULL && array->block->next->next != NULL);
    CHECK(array->block->next->next->next == NULL && array->elm_size == sizeof(def));
    *first=11;*middle=22;*last=33;
    CHECK(ArrNewElm(array,64) == middle && array->num_elms == 131);
    memset(&state,0,sizeof(state));CHECK(ArrWalk(array,&state,sum_element) == 0);
    CHECK(state.count == 131 && state.sum == 962);
    memset(&state,0,sizeof(state));state.stop=65;
    CHECK(ArrWalk(array,&state,sum_element) == -1 && state.count == 65 && state.sum == 474);
    CHECK(*first == 11 && *middle == 22 && *last == 33 && def == 7);
    CHECK(ArrAccess(array,130) == last && array->def == &def);
    ArrDestroy(array);CHECK(def == 7);
    array=ArrCreate(sizeof(def),NULL);CHECK(array != NULL);
    first=ArrNewElm(array,0);CHECK(first != NULL);*first=99;
    memset(&state,0,sizeof(state));CHECK(ArrWalk(array,&state,sum_element) == 0);
    CHECK(state.count == 1 && state.sum == 99 && array->def == NULL);
    ArrDestroy(array);return 1;
}
static int queue_owners(void)
{
    struct node { struct node *next; unsigned value; } a={NULL,1},b={NULL,2},c={NULL,3};
    qdesc dest,source,empty;
    QInit(&dest);QInit(&source);QInit(&empty);
    QEnqueue(&source,&a);QEnqueue(&source,&b);QJoinQueue(&dest,&source);
    CHECK(dest.head == &a && dest.tail == &b && source.head == &a && source.tail == &b);
    CHECK(a.next == &b && b.next == NULL);
    QJoinQueue(&dest,&empty);CHECK(dest.head == &a && dest.tail == &b);
    QInit(&source);QEnqueue(&source,&c);QJoinQueue(&dest,&source);
    CHECK(dest.head == &a && dest.tail == &c && source.head == &c && source.tail == &c);
    CHECK(a.next == &b && b.next == &c && c.next == NULL);
    CHECK(QDequeue(&dest) == &a && QDequeue(&dest) == &b && QDequeue(&dest) == &c);
    CHECK(QDequeue(&dest) == NULL && dest.head == NULL && dest.tail == NULL);
    CHECK(a.value == 1 && b.value == 2 && c.value == 3 && source.head == &c);
    return 1;
}
static int fixup_owners(void)
{
    fixup anchor,*source,*copy,before;
    memset(&anchor,0,sizeof(anchor));FixInit();source=FixNew();CHECK(source != NULL);
    memset(source,0,sizeof(*source));source->next=&anchor;source->loc_method=FIX_OFFSET;
    source->loc_offset=123;source->lr.frame=F_SEG;source->lr.frame_datum=2;
    source->lr.target=T_EXTWD;source->lr.target_datum=3;source->lr.target_offset=17;
    before=*source;copy=FixDup(source);CHECK(copy != NULL && copy != source);
    CHECK(memcmp(copy,&before,sizeof(before)) == 0 && copy->next == &anchor);
    copy->loc_offset=456;CHECK(memcmp(source,&before,sizeof(before)) == 0);
    CHECK(source->next == &anchor && anchor.next == NULL);
    FixKill(copy);FixKill(source);FixFini();CHECK(anchor.next == NULL);
    return 1;
}
static int successful_realloc(void)
{
    unsigned char *memory,*next,expected[24];
    unsigned i;
    memory=AsmAlloc(24);CHECK(memory != NULL);
    for(i=0;i<24;++i) memory[i]=expected[i]=(unsigned char)(i+1);
    next=AsmRealloc(memory,64);CHECK(next != NULL);memory=next;
    CHECK(memcmp(memory,expected,24) == 0);
    memset(memory+24,0x5a,40);
    next=MemRealloc(memory,8);CHECK(next != NULL);memory=next;
    CHECK(memcmp(memory,expected,8) == 0);
    MemFree(memory);return 1;
}
const ReconTest collection_tests[]={
    {"three-array-blocks-default-owner-and-callback-stop",1,array_owners},
    {"queue-splice-borrowed-aliases-and-dequeue",1,queue_owners},
    {"carved-fixup-shallow-copy-and-borrowed-next",1,fixup_owners},
    {"successful-nonNULL-realloc-prefix-and-final-owner",1,successful_realloc}
};
