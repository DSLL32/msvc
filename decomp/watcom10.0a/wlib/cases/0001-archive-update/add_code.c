extern int shared_common;
extern unsigned long data_words[];
extern int keep_value(int);
int added_entry(int x) { shared_common += x; return keep_value(x) + (int)data_words[x & 3]; }
