/* Backend stress: arithmetic, loops, aliasing, calls and x87 conversions. */
extern int observe(int);
extern double floating_call(double);

unsigned mix_unsigned(unsigned value, unsigned divisor, unsigned shift)
{
    unsigned quotient = value / (divisor | 1);
    unsigned remainder = value % (divisor | 1);
    return (quotient << (shift & 15)) ^ (remainder >> (shift & 7)) ^ value;
}

long mix_signed(long value, long divisor)
{
    long quotient, remainder;
    divisor = (divisor & 255) + 1;
    quotient = value / divisor;
    remainder = value % divisor;
    return quotient * 17 + remainder - (value >> 3);
}

int nested_loops(int *values, int rows, int columns, int threshold)
{
    int row, column, total = 0, carry = 3;
    for (row = 0; row < rows; ++row) {
        int subtotal = 0;
        for (column = 0; column < columns; ++column) {
            int value = values[row * columns + column];
            if (value > threshold)
                subtotal += value * (column + 1);
            else
                subtotal -= value ^ carry;
            carry = (carry + value) & 1023;
        }
        total += (subtotal & 1) ? observe(subtotal) : subtotal;
    }
    return total + carry;
}

struct AliasBlock {
    int first, second, third;
    unsigned char bytes[12];
};

int pointer_aliasing(struct AliasBlock *destination, struct AliasBlock *source,
                    int *left, int *right, int count)
{
    struct AliasBlock saved = *source;
    int before = *left, index;
    destination->first = before + source->second;
    *right = source->third + before;
    destination->second = *left + *right;
    *destination = saved;
    destination->bytes[1] = source->bytes[0];
    for (index = 0; index < count; ++index) {
        int value = left[index];
        right[index] = value + destination->second;
        destination->bytes[index & 7] ^= (unsigned char)*left;
    }
    return before + *left + *right + destination->third;
}

double floating_point(double *destination, const float *source, int count, int bias)
{
    double sum = (double)bias + 0.25, scale = 1.5;
    int index;
    for (index = 0; index < count; ++index) {
        double value = (double)source[index] * scale + (double)(index - bias);
        if (value < 0.0)
            value = -value / (scale + 0.5);
        else
            value = value * value - 0.125;
        destination[index] = value;
        sum += value;
        scale += 0.0625;
    }
    return floating_call(sum) + (double)(long)(sum * 0.5);
}
