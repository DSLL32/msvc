/* Real section/declaration owners for external legacy debug support. */
int memcmp(const void *, const void *, unsigned);
void *memcpy(void *, const void *, unsigned);
void *memset(void *, int, unsigned);
#pragma data_seg("SUPPORT_DATA")
int support_segment_value = 7;
#pragma data_seg()
char *base_pointer;
int object_base;
struct SupportRecord { short key; unsigned flags; double value; };
struct SupportRecord support_records[3];
const float support_single = 1.25f;
const double support_double = -2.5;
int support_callback(struct SupportRecord *record, int bias)
{
    return record->key + bias + record->flags;
}
int main(void)
{
    memcpy(support_records + 1, support_records, sizeof(struct SupportRecord));
    memset(support_records + 2, 0, sizeof(struct SupportRecord));
    return support_callback(support_records, object_base) +
        memcmp(support_records, support_records + 1, sizeof(struct SupportRecord));
}
