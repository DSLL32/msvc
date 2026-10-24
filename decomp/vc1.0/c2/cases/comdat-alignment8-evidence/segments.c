typedef struct State { unsigned empty_name; unsigned records[1004]; } State;
#pragma bss_seg("OMFBSS")
State bss_state;
#pragma bss_seg()
#pragma data_seg("OMFDATA")
State data_state;
State zero_state = {0};
#pragma data_seg()
int main(void) { return ((unsigned)&bss_state | (unsigned)&data_state | (unsigned)&zero_state) & 7; }
