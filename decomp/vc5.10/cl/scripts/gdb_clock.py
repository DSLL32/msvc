"""Read native _ftime inputs/effects without calls or inferior state changes."""
import json
from pathlib import Path
import struct
import gdb

def install(directory):
    directory=Path(directory);calls=[]
    if not(directory/'clock-response.json').exists():return
    inferior=gdb.selected_inferior
    def value(expression):return int(gdb.parse_and_eval(expression))&0xffffffff
    def observe(pointer):
        process=inferior();ranges=[]
        def read(address,length):
            raw=bytes(process.read_memory(address,length))
            ranges.append({'address':address,'data':raw.hex()});return raw
        symbols={name:value('(unsigned)&'+expression)for name,expression in{
            'errno':'errno','doserrno':'recon_doserrno','last_error':'last_error',
            'counter':'local_clock_calls','timezone':'recon_timezone',
            'daylight':'recon_daylight','started':'timer_started'}.items()}
        buffer=read(pointer,12)
        words={name:struct.unpack('<I',read(symbols[name],4))[0]
            for name in('errno','doserrno','last_error','timezone','daylight')}
        count=struct.unpack('<Q',read(symbols['counter'],8))[0]
        started=read(symbols['started'],12)
        # Read raw register representations. Preserve all of them for debugging;
        # same-process effect checks avoid equating PE and ELF code selectors/IPs.
        frame=gdb.newest_frame()
        registers={name:frame.read_register(name).bytes.hex()for name in
            ['fctrl','fstat','ftag','fiseg','fioff','foseg','fooff','fop']+
            ['st%d'%i for i in range(8)]}
        return {'pid':process.pid,'pointer':pointer,'symbols':symbols,'ranges':ranges,
            'timeb_hex':buffer.hex(),'started_hex':started.hex(),'scalars':words,
            'counter':count,'x87_registers':registers,'fctrl':value('$fctrl')}
    def save():
        (directory/'native-clock-gdb.json').write_text(json.dumps({'calls':calls},indent=2)+'\n')
    class Return(gdb.FinishBreakpoint):
        def __init__(self,frame,record,pointer):
            super().__init__(frame,internal=True);self.record=record;self.pointer=pointer
        def stop(self):
            self.record['after']=observe(self.pointer);save();return False
    class Entry(gdb.Breakpoint):
        def stop(self):
            frame=gdb.newest_frame();index=len(calls)
            assert index<2,'extra native driver _ftime call'
            caller=frame.older();assert caller.name()==('start_timer','stop_timer')[index]
            pointer=int(frame.read_var('result'))&0xffffffff
            slot=int(frame.read_var('result').address)&0xffffffff
            raw=bytes(inferior().read_memory(slot,4))
            assert struct.unpack('<I',raw)[0]==pointer
            record={'sequence':index+1,'caller':caller.name(),'argument_slot':slot,
                'argument_slot_hex':raw.hex(),'before':observe(pointer)}
            calls.append(record);save();Return(frame,record,pointer);return False
    Entry('recon_ftime',internal=True)
