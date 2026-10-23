set pagination off
set confirm off
break *edit_section
run -edit /section:.data=.store,rw /section:.text=.code,er '/section:.reloc,!d' /section:.rdata,a4 edit.dll >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/edit-section-rename-edit_section-_t518n2n/native.stdout 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/edit-section-rename-edit_section-_t518n2n/native.stderr
printf "NATIVE_MATCH edit_section\n"
x/6wx $esp
bt 5
quit
