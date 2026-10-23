/* Standalone C1 QuickC-query mode; no CL or backend compatibility claim. */
typedef struct Sample {
    int tag;
    float scale;
    double bias;
    short limits[3];
} Sample;

volatile int qc_sink;
static const unsigned char masks[8] = { 1, 3, 7, 15, 31, 63, 127, 255 };

static __inline int clipped(int value, int upper)
{
    if (value < 0) return 0;
    if (value > upper) return upper;
    return value;
}

static __inline int combine(int left, int right)
{
    return (left * 3) ^ (right + 7);
}

static __inline double adjusted(double value, float scale, double bias)
{
    if (value < 0.0) return bias;
    return value * scale + bias;
}

static int escaped_parameter(int value)
{
    int *address = &value;
    int (*operation)(int, int) = combine;
    *address += 2;
    return operation(*address, value >> 1);
}

int quickc_workflow(unsigned limit, Sample *destination)
{
    Sample local = { 3, 0.5f, 1.25, { 2, 5, 9 } };
    int values[4] = { 3, 5, 7, 11 };
    float weights[3] = { 0.25f, 0.5f, 1.5f };
    double offsets[2] = { 1.25, 2.5 };
    char label[8] = "regions";
    int i, j, total = 0, n;
    int *alias = &total;
    double accumulated = 0.0;

    n = clipped((int)limit, 8);
    for (i = 0; i < n; ++i) {
        for (j = 0; j < 3; ++j) {
            if (i == j) continue;
            total += combine(values[(i + j) & 3], j);
            if (total > 600) break;
        }
        accumulated += adjusted((double)i, weights[i % 3], offsets[i & 1]);
        if (i & 1) total ^= masks[i & 7];
        else total += local.limits[i % 3];
    }

    i = n;
    while (i > 0) {
        --i;
        *alias += escaped_parameter(i);
        if (i == 2) break;
    }
    j = 0;
    do {
        total += label[j];
        ++j;
    } while (j < 3);

    switch (total & 3) {
    case 0: total += values[0]; break;
    case 1: total += values[1];
    case 2: total += values[2]; break;
    default: goto finished;
    }
    total += (int)accumulated;
finished:
    local.tag = total;
    local.bias = accumulated;
    *destination = local;
    return total;
}

int main(void)
{
    Sample output;
    qc_sink = quickc_workflow(7, &output);
    return output.tag != qc_sink;
}
