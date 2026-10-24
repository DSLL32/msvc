from gdb_mre import observe
import gdb
assert gdb.newest_frame().name()=='jobs_from_deferred'
print("NATIVE_FRONTIER_SYMBOL="+gdb.newest_frame().name())
observe('/root/.ai-tmp/msvc2/vc5.10/cl/comparator-2-native',"mre-frontier",{})
