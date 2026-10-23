#pragma title("Configurable calibration table")
#pragma subtitle("Partial records and thread-local cache")
#pragma page(1)

static const unsigned coefficients[48] = {
    CAL00, CAL01, CAL02, CAL03, CAL04, CAL05, CAL06, CAL07,
    CAL08, CAL09, CAL10, CAL11, CAL12, CAL13, CAL14, CAL15,
    CAL16, CAL17, CAL18, CAL19, CAL20, CAL21, CAL22, CAL23,
    CAL24, CAL25, CAL26, CAL27, CAL28, CAL29, CAL30, CAL31,
    CAL32, CAL33, CAL34, CAL35, CAL36, CAL37, CAL38, CAL39,
    CAL40, CAL41, CAL42, CAL43, CAL44, CAL45, CAL46, CAL47
};
static double gains[3] = { -7, 2147483648U, 9 };
static float bias = 11;

#pragma pack(2)
struct CalibrationRow {
    unsigned short tag;
    unsigned low : 3;
    unsigned high : 5;
    double samples[3];
    union {
        unsigned integral;
        float real;
    };
};
#pragma pack()
static struct CalibrationRow records[2] = { { 3 }, { 5, 2 } };
struct Cache { unsigned sum; double last; };

static unsigned calibrate(unsigned input)
{
    static __declspec(thread) unsigned calls;
    static __declspec(thread) struct Cache cache;
    if (!calls && (cache.sum || cache.last != 0.0)) return 0xffffffffU;
    ++calls;
    cache.sum += input;
    cache.last = (double)input;
    return cache.sum + calls + (unsigned)cache.last;
}

int main(void)
{
    unsigned sum = 0, i, j;
    for (i = 0; i < 48; ++i) sum += coefficients[i];
    if (sum != 1176U) return 1;
    if (records[0].tag != 3 || records[0].low || records[0].high ||
        records[0].integral || records[1].tag != 5 || records[1].low != 2 ||
        records[1].high || records[1].integral) return 2;
    for (i = 0; i < 2; ++i)
        for (j = 0; j < 3; ++j)
            if (records[i].samples[j] != 0.0) return 3;
    if (gains[0] != -7.0 || gains[1] != 2147483648.0 || gains[2] != 9.0 ||
        bias != 11.0f) return 4;
    if (calibrate(3) != 7 || calibrate(5) != 15) return 5;
    return 0;
}
