extern int pressure_call(int);

int register_pressure(int *values, int bias)
{
    int a = values[0] + bias;
    int b = values[1] - bias;
    int c = values[2] ^ bias;
    int d = values[3] + 3 * bias;
    int e = values[4] - 5 * bias;
    int f = values[5] ^ (bias + 7);
    int g = values[6] + 11 * bias;
    int h = values[7] - 13 * bias;
    int i = values[8] ^ (bias + 17);
    int j = values[9] + 19 * bias;
    int k = values[10] - 23 * bias;
    int l = values[11] ^ (bias + 29);
    int first = pressure_call(bias);
    int second = pressure_call(first + bias);
    if (second & 1)
        return (a ^ first) + b * 3 + c * 5 + d * 7 + e * 11 + f * 13 +
            g * 17 + h * 19 + i * 23 + j * 29 + k * 31 + l * 37 + second;
    return (a + second) ^ (b - first) ^ (c + d) ^ (e - f) ^
        (g + h) ^ (i - j) ^ (k + l);
}
