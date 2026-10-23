__declspec(dllimport) int local_fn(int);
int duplicate_value=7;
int main(void) { return local_fn(5)-12; }
