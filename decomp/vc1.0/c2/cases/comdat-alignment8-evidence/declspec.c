typedef struct State { unsigned empty_name; unsigned records[1004]; } State;
__declspec(align(8)) State state;
int main(void) { return (unsigned)&state & 7; }
