int shared_common;
unsigned long data_words[4] = { 0x12345678UL, 7UL, 0xabcdefUL, 0UL };
extern int keep_value(int);
int (*saved_entry)(int) = keep_value;
