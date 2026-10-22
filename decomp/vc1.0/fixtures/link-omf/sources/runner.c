int omf_value(void);
int omf_helper(void) { return 20; }
int main(void) { return omf_value()==42 ? 0 : 1; }
