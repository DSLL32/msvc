/* Real parsed declarations for external legacy support calls. */
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
    return support_callback(support_records, object_base);
}
