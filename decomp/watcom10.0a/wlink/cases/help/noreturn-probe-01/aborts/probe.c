extern void exit(int);
#pragma aux exit aborts;
int main(void) { exit(7); return 0; }
