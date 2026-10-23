/* One original-compiler OMF case: typed data, fixups and lexical scopes. */
#pragma comment(lib, "omf_runtime.lib")
#pragma comment(user, "OMF typed debug case")
typedef union OmfValue {
    long integer;
    double real;
    unsigned char bytes[8];
} OmfValue;
typedef struct OmfItem {
    OmfValue value;
    short delta[3];
    char label[10];
} OmfItem;
typedef struct OmfTable {
    unsigned count;
    OmfItem item[3];
    struct OmfTable *next;
} OmfTable;
enum OmfMode { OmfSubtract = 3, OmfRotate = 7, OmfAdd = 11 };
extern int omf_external(OmfItem *, int, char *);
extern unsigned omf_unknown;
static char omf_text[] = "typed OMF relocation and debug scopes";
static OmfTable omf_table = {
    3,
    { { { 19 }, { 1, -2, 3 }, "first" },
      { { -23 }, { 4, 5, -6 }, "second" },
      { { 31 }, { -7, 8, 9 }, "third" } },
    &omf_table
};
OmfItem *omf_selected = &omf_table.item[1];
char *omf_message = omf_text + 6;
int (*omf_callback)(OmfItem *, int, char *) = omf_external;
static int omf_adjust(OmfItem *item, int amount)
{
    register int result = item->value.integer + amount;
    {
        int offset = item->delta[amount & 1];
        {
            int mixed = result ^ offset;
            if (mixed & 1) result -= mixed;
            else result += mixed;
            item->value.integer = result;
        }
    }
    return result;
}
__declspec(dllexport) int omf_typed_records(OmfTable *table, int limit)
{
    OmfItem copy;
    int i, total = omf_unknown;
    for (i = 0; i < limit; ++i) {
        copy = table->item[i % 3];
        total += omf_adjust(&copy, total & 15);
        total ^= omf_callback(&copy, i, omf_message);
        {
            enum OmfMode mode = (enum OmfMode)(i & 15);
            {
                int bias = mode == OmfSubtract ? -3 : 5;
                copy.delta[2] += bias;
                total += copy.delta[2];
            }
        }
        __asm {
            mov eax, total
            test eax, 1
            jz omf_even
            sub eax, i
            jmp omf_join
        omf_even:
            add eax, i
        omf_join:
            mov total, eax
        }
        table->item[i % 3] = copy;
    }
    return total + omf_external(omf_selected, limit, omf_text);
}
