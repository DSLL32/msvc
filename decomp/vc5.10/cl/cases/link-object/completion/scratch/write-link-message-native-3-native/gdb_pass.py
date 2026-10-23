"""Read the native pass table at the actual accepted help entry, without calls."""
import json
from pathlib import Path
import struct
import gdb

def install(directory):
    directory=Path(directory)
    if not(directory/'pass-state-observer.txt').exists():return
    count=[0]
    class Observe(gdb.Breakpoint):
        def stop(self):
            count[0]+=1;assert count[0]==1 and gdb.newest_frame().name()=='show_help'
            base=int(gdb.parse_and_eval('(unsigned)compiler_passes'));inferior=gdb.selected_inferior()
            table=bytes(inferior.read_memory(base,180));data=bytearray(struct.pack('<3I',0x31535043,base,180)+table)
            for i in range(5):
                words=struct.unpack_from('<9I',table,36*i)
                for slot in (0,1,2,8):
                    pointer=words[slot];text=bytearray()
                    if pointer:
                        for offset in range(1024):
                            byte=bytes(inferior.read_memory(pointer+offset,1));text+=byte
                            if byte==b'\0':break
                        else:raise AssertionError('unbounded pass string')
                    data+=struct.pack('<2I',pointer,len(text))+text
            (directory/'pass-state.bin').write_bytes(data)
            (directory/'pass-state-gdb.json').write_text(json.dumps({'symbol':'show_help','count':count[0],'base':base,'bytes':len(data)},indent=2)+'\n')
            return False
    Observe('show_help',internal=True)
