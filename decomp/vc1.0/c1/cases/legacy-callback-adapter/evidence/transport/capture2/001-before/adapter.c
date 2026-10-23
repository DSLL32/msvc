/* A legacy callback interface and its typed adapter share one implementation. */
typedef int (__cdecl *UnaryCallback)(int);
typedef int (__cdecl *OldCallback)();
typedef int (__cdecl *ShortCallback)(short);
typedef UnaryCallback (__cdecl *CallbackFactory)(int);

struct Adapter {
    UnaryCallback invoke;
    CallbackFactory select;
    int bias;
    int (*rows)[3];
};

int promoted_pair(int, double);
int adapter_run(OldCallback, int [3]);
int adapter_run(UnaryCallback, int *);

int promoted_pair(value, scale)
short value;
float scale;
{
    return value + (int)scale;
}

static int add_bias(register int value)
{
    return value + 3;
}
static int identity(int value)
{
    return value;
}
static UnaryCallback choose_callback(int mode)
{
    if (mode) return add_bias;
    return identity;
}
int adapter_run(UnaryCallback callback, int values[])
{
    int i, sum = 0;
    for (i = 0; i < 3; ++i) sum += (*callback)(values[i]);
    return sum;
}
static int invoke_adapter(struct Adapter *adapter, register int index)
{
    UnaryCallback chosen = (*adapter->select)(index);
    return (*chosen)(adapter->rows[0][index]) + adapter->bias;
}

int main(void)
{
    int samples[2][3] = { { 1, 2, 3 }, { 4, 5, 6 } };
    struct Adapter adapter;
    UnaryCallback restored;
    OldCallback old_view = add_bias;
    ShortCallback narrow_view = add_bias;
    void *opaque = add_bias;
    restored = opaque;
    adapter.invoke = restored;
    adapter.select = choose_callback;
    adapter.bias = 5;
    adapter.rows = samples;
    if (promoted_pair(7, 2.5) != 9) return 1;
    if (adapter_run(adapter.invoke, samples[0]) != 15) return 2;
    if (invoke_adapter(&adapter, 1) != 10) return 3;
    if ((*old_view)(6) != 9 || (*restored)(4) != 7) return 4;
    if (!narrow_view) return 5; /* Inspect the alternate prototype; never call it. */
    return 0;
}
