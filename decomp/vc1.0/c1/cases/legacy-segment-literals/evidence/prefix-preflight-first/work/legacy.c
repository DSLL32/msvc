/* Standalone C1 legacy declaration and literal compatibility workflow. */
#include "legacy-types.h"

char *record_base;
LegacyRecord records[LEGACY_COUNT];
NodeLink offset_head;
typedef int __based(record_base) RecordRelativeInt;
RecordRelativeInt record_total;
static unsigned char counted_name[] = "\pREC" "ORDS";
static unsigned char counted_label[] = "\pLegacy packet";
static unsigned char packet_bytes[] = "A\x42\103\n";
static const char *plain_label = "segment record traversal";
volatile int legacy_sink;
int record_counter;

static int __far __pascal score_record(LegacyRecord *record, int bias)
{
    int value = record->key + bias;
    if (record->flags & 1) value += record->value.integer;
    else value -= record->state;
    return value;
}

static unsigned checksum_counted(const unsigned char *text)
{
    unsigned i, result = 0;
    for (i = 1; i <= text[0]; ++i)
        result = (result << 1) ^ text[i];
    return result;
}

static void fill_records(void)
{
    int i;
    for (i = 0; i < LEGACY_COUNT; ++i) {
        records[i].key = (unsigned short)(11 + i);
        records[i].flags = i & LEGACY_MASK;
        records[i].state = i + 2;
        records[i].value.integer = 3L * i;
        records[i].next = i + 1 < LEGACY_COUNT ? (NodeLink)((i + 1) * sizeof(LegacyRecord)) : 0;
    }
    record_base = (char *)records;
    offset_head = (NodeLink)0;
}

int legacy_workflow(LegacyCallback callback, unsigned limit)
{
    LegacyRecord *record;
    unsigned i, mixed = checksum_counted(counted_name);
    int total = 0;
    fill_records();
    for (i = 0; i < limit && i < LEGACY_COUNT; ++i) {
        record = records + i;
        total += callback(record, (int)i);
        if (record->next) mixed ^= (unsigned)record->next;
        if (i & 1) mixed += packet_bytes[i];
        else mixed += counted_label[i + 1];
    }
    record_total = total;
    record_counter = (int)mixed;
    legacy_sink = plain_label[0] + (int)checksum_counted(counted_label);
    return record_total + record_counter + legacy_sink;
}

int main(void)
{
    LegacyCallback callback = score_record;
    return legacy_workflow(callback, LEGACY_COUNT) == 0;
}
