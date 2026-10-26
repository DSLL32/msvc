/* Two real C translation units carry types, locals, static functions and
 * cross-module data references into the original compiler's /Z7 records. */
typedef enum ItemKind { ITEM_SCALAR=1,ITEM_VECTOR=2,ITEM_PAIR=3 } ItemKind;
typedef struct Point { int x,y; } Point;
typedef union ItemValue { long scalar; Point pair; unsigned char bytes[8]; } ItemValue;
typedef struct Item {
    ItemKind kind;
    ItemValue value;
    char label[12];
    struct Item *next;
} Item;

#ifdef DEBUG_LEFT
extern Item right_item;
extern int __cdecl EvaluateItem(const Item *,int);
static int __cdecl AddPoint(Point *point,int amount)
{
    int old=point->x+point->y;
    point->x+=amount;
    point->y-=amount;
    return old;
}
Item left_item={ITEM_PAIR,{17},"left",&right_item};
static int history[4];
int __stdcall TypedEntry(void)
{
    Point point={3,7};
    int result=AddPoint(&point,2);
    history[0]=EvaluateItem(&left_item,result);
    history[1]=EvaluateItem(left_item.next,point.x);
    return history[0]+history[1];
}
#else
Item right_item={ITEM_SCALAR,{29},"right",0};
static int __cdecl LabelLength(const char *name)
{
    int size=0;
    while(name[size])++size;
    return size;
}
int __cdecl EvaluateItem(const Item *item,int scale)
{
    int length=LabelLength(item->label);
    switch(item->kind) {
    case ITEM_SCALAR:return item->value.scalar*scale+length;
    case ITEM_VECTOR:return item->value.bytes[0]+scale;
    case ITEM_PAIR:return item->value.pair.x+item->value.pair.y+length;
    default:return -length;
    }
}
#endif
