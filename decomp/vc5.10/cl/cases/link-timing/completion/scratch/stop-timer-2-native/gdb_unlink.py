"""Read-only native observations of the actually executed removal calls."""
import json
from pathlib import Path
import re
import gdb
from gdb_mktemp import errors,string


def install(directory):
    directory=Path(directory)
    config=json.loads((directory/'config.json').read_text())
    response=directory/'dependency-response.json'
    if '_delete_temporary'not in config['accepted']or not response.exists():return
    if 'deletions'not in json.loads(response.read_text()):return
    records=[]
    def save():
        (directory/'native-unlink-gdb.json').write_text(json.dumps({
            'pid':gdb.selected_inferior().pid,'calls':records},indent=2)+'\n')
    class Return(gdb.FinishBreakpoint):
        def __init__(self,frame,record,path):
            super().__init__(frame,internal=True);self.record=record;self.path=path
        def stop(self):
            self.record.update(result=int(self.return_value),after=errors(),present_after=self.path.exists())
            save();return False
    class Entry(gdb.Breakpoint):
        def stop(self):
            frame=gdb.newest_frame();pointer=int(frame.read_var('name'))&0xffffffff
            filename=string(pointer)
            assert re.fullmatch(r'\.\\a0[0-9]{4}(ex|sy|gl|in|st|db|lk)',filename),'unknown removal path'
            caller=frame.older()
            assert caller.name()=='delete_temporary'
            local=int(caller.read_var('filename').address)&0xffffffff
            path=directory/filename[2:]
            record={'filename':filename,'present_before':path.exists(),'before':errors(),
                'argument_is_local_buffer':pointer==local}
            if not records and '_driver_exit'in config['accepted']:
                from completion_state import observe
                observe(directory,gdb,'postchild')
            records.append(record);save();Return(frame,record,path);return False
    Entry('recon_unlink',internal=True)
