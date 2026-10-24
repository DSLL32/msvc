from gdb_mre import observe
import gdb
assert gdb.newest_frame().name()=='option_matching_selector'
print("NATIVE_FRONTIER_SYMBOL="+gdb.newest_frame().name())
observe('/root/.ai-tmp/msvc2/vc5.10/cl/signature-2-native',"mre-frontier",{})
