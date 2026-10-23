/* Source-only seed for new 5.10 compiler/linker captures; no vc1.0 oracle. */
typedef struct DebugPoint {
    int x;
    int y;
} DebugPoint;

static int sum_point(const DebugPoint *point)
{
    return point->x + point->y;
}

__declspec(dllexport) int __stdcall hello_entry(void *module,
                                               unsigned long reason,
                                               void *reserved)
{
    DebugPoint point;
    (void)module;
    (void)reserved;
    point.x = (int)reason;
    point.y = 7;
    return sum_point(&point) != 0;
}
