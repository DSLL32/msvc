static __inline int clamp(int value) {
    if (value < 0) return 0;
    if (value > 20) return 20;
    return value;
}
static __inline int twice(int value) { return value + value; }
static int bias(int value, int add) {
    if (!value) return add;
    return value + add;
}
int evaluate(int value) {
    int limited = clamp(value);
    if (!limited) return bias(0, 3);
    return bias(twice(limited), 1);
}
int main(void) {
    return evaluate(-1) != 3 || evaluate(4) != 9 || evaluate(25) != 41;
}
