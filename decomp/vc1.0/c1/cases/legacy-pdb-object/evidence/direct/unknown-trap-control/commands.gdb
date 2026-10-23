set pagination off
set confirm off
set disable-randomization off
break *open_program_database_object
run 0 /root/.ai-tmp/msvc2/vc1.0/c1/validation/pdb-unknown-trap-control/report >/root/.ai-tmp/msvc2/vc1.0/c1/validation/pdb-unknown-trap-control/stdout 2>&1
python
assert int(gdb.parse_and_eval('$eip'))==int(gdb.parse_and_eval('(unsigned)create_program_database'))+1
end
signal SIGTRAP
delete 1
signal SIGTRAP
python
assert int(gdb.parse_and_eval('$_exitcode'))==88
end
