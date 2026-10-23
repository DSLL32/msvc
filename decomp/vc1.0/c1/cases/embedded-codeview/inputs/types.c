typedef enum Shade { shade_negative = -3, shade_plain = 7, shade_large = 70000 } Shade;
typedef struct Node Node;
typedef int (__cdecl *Transform)(const Node *, int);
typedef union Number { int integer; unsigned long bits; double real; } Number;
struct Node {
    Node *next;
    const char *name;
    Shade shade;
    Number value;
    int coordinates[3];
    Transform transform;
};
struct Flags { unsigned first:3; unsigned second:5; signed direction:4; };
#pragma pack(2)
struct Packed { char kind; unsigned short count; Node *node; };
#pragma pack()
typedef struct Bundle { Node nodes[2]; struct Flags flags; struct Packed packed; } Bundle;
static int scale(const Node *node, int amount) { return node->coordinates[0] * amount; }
Node root = { 0, "root", shade_plain, { 5 }, { 2, 3, 4 }, scale };
volatile const Node *root_view = &root;
Bundle bundle;
int invoke(Transform transform, const Node *node, int amount) { return transform(node, amount); }
int main(void) {
    struct Packed local;
    local.kind = 1;
    local.count = 2;
    local.node = &root;
    bundle.flags.first = 3;
    return invoke(root.transform, local.node, 4) != 8;
}
