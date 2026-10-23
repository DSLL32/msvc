struct Sample {
    unsigned char tag;
    unsigned short count;
    int values[4];
    double weight;
};

static struct Sample sample = { 3, 12, { 5, 9, 13, 17 }, 2.5 };
static int table[8] = { 2, 4, 6, 8, 10, 12, 14, 16 };
static double scale = 4.0;

int indexed_sum(struct Sample *record, int index)
{
    int result;
    __asm {
        mov ebx, record
        mov ecx, index
        mov eax, [ebx].values[ecx*4]
        lea edx, table
        add eax, [edx+ecx*4+4]
        movzx edx, WORD PTR [ebx].count
        add eax, edx
        movzx edx, BYTE PTR [ebx].tag
        add eax, edx
        mov result, eax
    }
    return result;
}

int constants_and_offsets(void)
{
    int result;
    __asm {
        mov esi, OFFSET table
        mov edi, OFFSET sample
        mov eax, DWORD PTR [esi+2*4]
        add eax, DWORD PTR [edi].values[3*4]
        add eax, TYPE sample
        add eax, LENGTH table
        add eax, SIZE table
        mov result, eax
    }
    return result;
}

double floating_mix(double value)
{
    double result;
    __asm {
        fld QWORD PTR value
        fld QWORD PTR scale
        fmul st(0), st(1)
        fld1
        faddp st(1), st(0)
        fxch st(1)
        fsubp st(1), st(0)
        fstp QWORD PTR result
    }
    return result;
}

int main(void)
{
    int sum = indexed_sum(&sample, 2);
    int constants = constants_and_offsets();
    double value = floating_mix(3.0);
    return sum != 36 || constants != 95 || value != 10.0;
}
