import sys
sys.path.insert(0,'/root/.ai-tmp/msvc2/vc5.10/cl/jobs-5-native')
from gdb_entry import capture
capture('/root/.ai-tmp/msvc2/vc5.10/cl/jobs-5-native')
from gdb_mktemp import install
install('/root/.ai-tmp/msvc2/vc5.10/cl/jobs-5-native')
from gdb_mre import install
install('/root/.ai-tmp/msvc2/vc5.10/cl/jobs-5-native')
import gdb
def completed(event):
    code=getattr(event,"exit_code",None)
    if code is not None:print("NATIVE_COMPLETION_STATUS="+str(code))
gdb.events.exited.connect(completed)
