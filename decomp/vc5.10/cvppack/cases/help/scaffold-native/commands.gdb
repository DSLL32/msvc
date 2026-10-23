set pagination off
set confirm off
set disable-randomization off
handle SIGTRAP stop print nopass
run /help >/root/msvc2/decomp/vc5.10/cvppack/cases/help/scaffold-native/stdout 2>/root/msvc2/decomp/vc5.10/cvppack/cases/help/scaffold-native/stderr
python
import gdb,json
f=gdb.newest_frame()
state={"function":f.name(),"pc":int(gdb.parse_and_eval("$pc")),"signal":gdb.execute("info program",to_string=True)}
open('/root/msvc2/decomp/vc5.10/cvppack/cases/help/scaffold-native/state.json',"w").write(json.dumps(state,indent=2))
end
quit
