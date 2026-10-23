/* A compact record arena whose links remain offsets when its base changes. */
char *arena_base;
char *other_base;
typedef struct Node Node;
typedef Node __based(arena_base) *NodeLink;
typedef Node __based(other_base) *OtherLink;

struct Node {
    int value;
    NodeLink next;
    short coordinate[2];
    unsigned flags;
};
typedef struct WalkState {
    NodeLink head;
    NodeLink cursor;
    unsigned visited;
} WalkState;
typedef union Arena {
    long alignment;
    unsigned char bytes[256];
} Arena;

void fill_arena(unsigned char *bytes, int initial)
{
    Node *records = (Node *)(bytes + 16);
    int i;
    for (i = 0; i < 3; ++i) {
        records[i].value = initial + 7 * i;
        records[i].next = i == 2 ? 0 : (NodeLink)(16 + (i + 1) * sizeof(Node));
        records[i].coordinate[0] = (short)(i + 1);
        records[i].coordinate[1] = (short)(2 * (i + 1));
        records[i].flags = 3U << i;
    }
}

int walk_arena(WalkState *state)
{
    int sum = 0;
    state->visited = 0;
    for (state->cursor = state->head; state->cursor; state->cursor = state->cursor->next) {
        sum += state->cursor->value + 3 * state->cursor->coordinate[0] +
               state->cursor->coordinate[1] + state->cursor->flags;
        if (++state->visited > 3) return -1;
    }
    return sum;
}

int check_conversions(NodeLink head, int initial)
{
    NodeLink next = head + 1;
    NodeLink roundtrip;
    OtherLink foreign;
    Node *full = (Node *)head;
    if (next - head != 1 || next[-1].value != initial) return 1;
    if (next->value != initial + 7 || head[2].value != initial + 14) return 2;
    roundtrip = (NodeLink)full;
    if (roundtrip != head || roundtrip->value != initial) return 3;
    foreign = (OtherLink)head;
    if (foreign->value != initial || (Node *)foreign != full) return 4;
    foreign = 0;
    roundtrip = 0;
    if (foreign || roundtrip) return 5;
    return 0;
}

int main(void)
{
    Arena first, second;
    WalkState state;
    NodeLink head;
    int (*walker)(WalkState *) = walk_arena;
    if (sizeof(Node) > 64) return 10;
    fill_arena(first.bytes, 10);
    fill_arena(second.bytes, 20);
    arena_base = (char *)first.bytes;
    other_base = (char *)first.bytes + 8;
    head = (NodeLink)16;
    state.head = head;
    if (walker(&state) != 102 || state.visited != 3) return 11;
    if (check_conversions(head, 10)) return 12;
    arena_base = (char *)second.bytes;
    other_base = (char *)second.bytes + 8;
    state.head = head;
    if (walker(&state) != 132 || state.visited != 3) return 13;
    if (check_conversions(head, 20)) return 14;
    state.head = 0;
    if (walker(&state) != 0 || state.visited) return 15;
    return 0;
}
