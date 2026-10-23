typedef union Pair { unsigned words[2]; void *pointer; double floating; unsigned char bytes[8]; } Pair;
Pair __stdcall return_pair(unsigned value);
