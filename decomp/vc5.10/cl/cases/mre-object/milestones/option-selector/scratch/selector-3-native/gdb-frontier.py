from gdb_mre import observe
import gdb
assert gdb.newest_frame().name()=='compare_option_strings'
print("NATIVE_FRONTIER_SYMBOL="+gdb.newest_frame().name())
observe('/root/.ai-tmp/msvc2/vc5.10/cl/selector-3-native',"mre-frontier",{})
