int shared_common;
extern int replacement(int);
char keep_bytes[6] = {1,3,5,7,9,11};
int keep_value(int x) { return replacement(x) + shared_common + keep_bytes[x & 3]; }
