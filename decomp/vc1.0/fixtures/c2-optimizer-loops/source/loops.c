/* Exercise loop-carried values, nested induction and conditional calls. */
extern int observe(int);

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
        if (subtotal & 1)
            total += observe(subtotal);
        else
            total += subtotal;
    }
    return total + carry;
}
