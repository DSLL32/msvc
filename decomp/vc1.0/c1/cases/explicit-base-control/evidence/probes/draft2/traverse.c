/* Explicit byte bases and stored offset links describe the same arrays. */
char *arena_base;
typedef int __based(arena_base) *CellLink;
static int first[6] = { 2, 3, 6, 7, 10, 13 };
static int second[6] = { 17, 20, 23, 30, 31, 37 };

CellLink advance_link(CellLink link, int count)
{
    return link + count;
}
CellLink keep_link(CellLink link)
{
    return link;
}

int read_explicit(char *base)
{
    int *left = (base + sizeof(int)) :> (int *)0;
    int *right = (base + 3 * sizeof(int)) :> (CellLink)0;
    return *left + *right;
}

int visit_values(char *base)
{
    int count = 0, sum = 0, *current;
again:
    if (count == 4) goto done;
    current = (base + sizeof(int)) :> (int *)0;
    if (*current & 1) goto odd;
    sum += *current + 2;
    goto advance;
odd:
    sum += *current * 3;
advance:
    base += sizeof(int);
    ++count;
    goto again;
done:
    return sum;
}

int main(void)
{
    CellLink head, later;
    arena_base = (char *)first;
    head = (CellLink)sizeof(int);
    later = keep_link(advance_link(head, 2));
    if (*head != 3 || *later != 7 || later - head != 2) return 1;
    if (read_explicit((char *)first) != 10 || visit_values((char *)first) != 50) return 2;
    arena_base = (char *)second;
    later = keep_link(advance_link(head, 1));
    if (*head != 20 || *later != 23) return 3;
    if (read_explicit((char *)second) != 50 || visit_values((char *)second) != 216) return 4;
    return 0;
}
