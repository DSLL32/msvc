/* Natural /Zi constructor context for the separate database-method tests. */
typedef struct RegistryRecord { int tag; long value; } RegistryRecord;
static RegistryRecord sample = { 3, 5 };
static int combine(RegistryRecord *entry) { return entry->tag + entry->value; }
int main(void) { return combine(&sample) == 8 ? 0 : 1; }
