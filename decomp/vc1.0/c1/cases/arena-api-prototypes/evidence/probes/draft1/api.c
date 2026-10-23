/* A legacy arena interface exports offset links and ordinary row views. */
char *entry_base;
typedef int Row[3];
typedef struct Entry Entry;
typedef Entry __based(entry_base) *EntryLink;
typedef int __cdecl ReadEntry(EntryLink);
typedef ReadEntry *Reader;
struct Entry {
    int value;
    Row samples;
    EntryLink next;
};
static Entry records[3];

int __far __cdecl inspect_entry(EntryLink entry)
{
    return entry->value + entry->samples[0];
}

int __loadds __cdecl total_entry(EntryLink entry)
{
    return entry->value + entry->samples[0] + entry->samples[1] + entry->samples[2];
}

int (__cdecl *select_reader(int all))(EntryLink)
{
    return all ? total_entry : inspect_entry;
}

Row *choose_row(EntryLink entry)
{
    return &entry->samples;
}

int old_checksum(void *);
int old_checksum(data)
Entry *data;
{
    return data->value + data->samples[2];
}

int main(void)
{
    EntryLink first, second;
    Reader reader;
    int (__cdecl __loadds *declared_reader)(EntryLink) = total_entry;
    Row *row;
    entry_base = (char *)records;
    records[1].value = 10;
    records[1].samples[0] = 3;
    records[1].samples[1] = 5;
    records[1].samples[2] = 7;
    records[2].value = 20;
    records[2].samples[0] = 11;
    records[2].samples[1] = 13;
    records[2].samples[2] = 17;
    first = (EntryLink)sizeof(Entry);
    second = first + 1;
    records[1].next = second;
    records[2].next = 0;
    reader = select_reader(0);
    if ((*reader)(first) != 13) return 1;
    reader = select_reader(1);
    if ((*reader)(second) != 61) return 2;
    if ((*declared_reader)(first) != 25) return 3;
    row = choose_row(first);
    if ((*row)[0] + (*row)[1] + (*row)[2] != 15) return 4;
    if (old_checksum(&records[1]) != 17) return 5;
    if (first->next != second || second->next) return 6;
    return 0;
}
