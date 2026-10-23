#include <stdio.h>
extern int _atoldbl(void *, const char *);
int main(void) { char input[] = "1.5"; unsigned char bytes[10]; int result, i; result = _atoldbl(bytes, input); printf("result %d\noutput ", result); for (i = 0; i < 10; ++i) printf("%02x", bytes[i]); printf("\nafter_input "); for (i = 0; i < sizeof(input); ++i) printf("%02x", (unsigned char)input[i]); printf("\n"); return 0; }
