"""Observe actual shim entry/return state with read-only native GDB queries."""
import json
from pathlib import Path
import gdb


def errors():
    return {name:int(gdb.parse_and_eval(expression))for name,expression in {
        'errno':'(int)errno','doserrno':'recon_doserrno','last_error':'last_error'}.items()}


def string(pointer):
    data=bytearray()
    for i in range(260):
        byte=bytes(gdb.selected_inferior().read_memory(pointer+i,1))
        if byte==b'\0':return data.decode('ascii')
        data+=byte
    raise AssertionError('unbounded native temporary-name buffer')


def install(directory):
    directory=Path(directory)
    if not (directory/'mktemp-expected.json').exists():return
    records=[]
    def save():
        (directory/'native-mktemp-gdb.json').write_text(json.dumps({'calls':records},indent=2)+'\n')
    class Return(gdb.FinishBreakpoint):
        def __init__(self,frame,record):
            super().__init__(frame,internal=True);self.record=record
        def stop(self):
            try:
                value=int(self.return_value)&0xffffffff
                self.record.update(result_address=value,
                    result_alias=value==self.record['argument_address'],
                    result=string(self.record['argument_address']),after=errors())
                save();return False
            except Exception as error:
                self.record['error']=repr(error);save();raise
    class Entry(gdb.Breakpoint):
        def stop(self):
            assert not records,'extra native temporary-name call'
            frame=gdb.newest_frame();pointer=int(frame.read_var('pattern'))&0xffffffff
            record={'argument_address':pointer,'template':string(pointer),'before':errors()}
            records.append(record);save();Return(frame,record);return False
    Entry('recon_mktemp',internal=True)
