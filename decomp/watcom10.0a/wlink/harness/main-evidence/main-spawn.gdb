break main
continue
set $entry_argv = argv
printf "main argc=%d\n", argc
break Spawn
continue
python
import gdb
checks = {
 'callback': int(gdb.parse_and_eval('callback')) == int(gdb.parse_and_eval('&DoLinkFromCommandLine')),
 'Argv_alias': int(gdb.parse_and_eval('Argv')) == int(gdb.parse_and_eval('$entry_argv')),
 'MapFile_zero': int(gdb.parse_and_eval('MapFile')) == 0,
 'MapFName_null': int(gdb.parse_and_eval('MapFName')) == 0,
 'OutFiles_null': int(gdb.parse_and_eval('OutFiles')) == 0,
 'LibPath_null': int(gdb.parse_and_eval('LibPath')) == 0,
 'LinkState_zero': int(gdb.parse_and_eval('LinkState')) == 0,
}
print('MAIN_SPAWN_NATIVE ' + repr(checks))
assert all(checks.values())
end
detach
quit
