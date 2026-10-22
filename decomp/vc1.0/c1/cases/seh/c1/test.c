int guarded(int x)
{
    int value = 0;
    __try {
        __try {
            if (x < 0) __leave;
            while (x > 0) {
                value += x;
                --x;
            }
        } __finally {
            value += 3;
        }
    } __except (1) {
        value = -1;
    }
    return value;
}

int main(void)
{
    return guarded(2) != 6;
}
