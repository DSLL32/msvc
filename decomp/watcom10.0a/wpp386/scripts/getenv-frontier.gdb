set pagination off
set confirm off
handle SIGTRAP nostop noprint pass
# Stop on the real prologue, not the stub's literal INT3 instruction: GDB's
# software breakpoint on an existing INT3 can consume that frontier on resume.
break *GetEnvVar
commands
silent
printf "GetEnvVar length=%d Initialized=%d AllocSize=%u\n",length,Initialized,AllocSize
x/s name
if length != 9 || Initialized != 2 || AllocSize != 0 || name[0] != 'W' || name[1] != 'C' || name[2] != 'G' || name[3] != 'M' || name[4] != 'E' || name[5] != 'M' || name[6] != 'O' || name[7] != 'R' || name[8] != 'Y' || name[9] != 0
quit 2
end
set $output_buffer = buffer
up
printf "MemInit buffer extent=%u\n",sizeof(buff)
if sizeof(buff) != 80 || &buff[0] != $output_buffer
quit 2
end
down
disable 1
signal 0
end
continue
