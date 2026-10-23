typedef union DebugValue {
    long signed_value;
    unsigned bits;
    unsigned char bytes[4];
} DebugValue;

typedef struct DebugInner {
    DebugValue value;
    short lanes[3];
} DebugInner;

typedef struct DebugRecord {
    DebugInner inner;
    double weight;
    char label[9];
} DebugRecord;

typedef int (__cdecl *DebugCallback)(DebugRecord *, int);
extern int __cdecl observe_debug(DebugRecord *, const char *, int);
static const short adjustments[5] = { -7, 3, 19, -11, 5 };
static char description[] = "debug record listing";

int __cdecl transform_debug_record(DebugRecord *record, int seed)
{
    unsigned mixed;
    int i;
    __asm {
        mov eax, seed
        rol eax, 3
        test eax, eax
        jnz mixed_nonzero
        mov eax, 1357h
mixed_nonzero:
        xor eax, 5a5ah
        mov mixed, eax
    }
    record->inner.value.bits ^= mixed;
    for (i = 0; i < 3; ++i)
        record->inner.lanes[i] = (short)(record->inner.lanes[i] + adjustments[(seed + i) & 3]);
    record->weight = record->weight * 1.25 + (double)(mixed & 255);
    return observe_debug(record, description, seed);
}

int __cdecl debug_listing_records(DebugRecord *records, int count, int seed,
                                 DebugCallback callback)
{
    DebugRecord local;
    int history[4];
    int i, total = seed;
    for (i = 0; i < count; ++i) {
        local = records[i];
        history[i & 3] = transform_debug_record(&local, seed + i);
        total += callback(&local, history[i & 3]);
        local.inner.value.signed_value += total;
        local.label[0] = (char)('A' + (i & 15));
        records[i] = local;
    }
    return observe_debug(records, description, total);
}
