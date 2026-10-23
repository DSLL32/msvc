#include <stdio.h>
#include "pair.h"
int main(void){ Pair v;v=return_pair(0x11223344);printf("C %08x %08x\n",v.words[0],v.words[1]);return v.words[0]!=0x11223344||v.words[1]!=0x55667788;}
