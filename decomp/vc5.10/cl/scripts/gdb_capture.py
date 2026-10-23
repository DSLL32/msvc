"""Read actual native frame/CRT state, or terminal status, under Linux GDB."""
import json
from pathlib import Path
import struct
import gdb


def capture(directory):
    directory=Path(directory);inferior=gdb.selected_inferior()
    try:frame=gdb.newest_frame()
    except gdb.error:frame=None
    if frame is None:
        print('NATIVE_COMPLETION_STATUS='+str(int(gdb.parse_and_eval('$_exitcode'))))
        return
    print('NATIVE_FRONTIER_SYMBOL='+frame.name())
    if frame.name()!='main':
        from frontier_state import capture_native
        capture_native(directory,gdb)
        return
    def scalar(expression):return int(gdb.parse_and_eval(expression))
    def cstring(pointer):
        data=bytearray()
        for i in range(32768):
            byte=bytes(inferior.read_memory(pointer+i,1));data+=byte
            if byte==b'\0':return bytes(data)
        raise AssertionError('unterminated native string')
    def vector(name,pointer):
        records=[]
        for count in range(4096):
            address=struct.unpack('<I',bytes(inferior.read_memory(pointer+4*count,4)))[0]
            if not address:break
            value=cstring(address);records.append(struct.pack('<II',address,len(value))+value)
        else:raise AssertionError('unterminated native vector')
        (directory/name).write_bytes(struct.pack('<II',pointer,len(records))+b''.join(records))
    raw={name:scalar(expression)for name,expression in {
        'argc':'argc','argv_address':'(unsigned)argv','envp_address':'(unsigned)envp',
        'environ_address':'(unsigned)environ','fmode':'recon_fmode','commode':'recon_commode',
        'mb_cur_max':'recon_mb_cur_max','x87_control_word':'$fctrl'}.items()}
    raw['osfile']=[scalar('recon_osfile[%d]'%i)for i in range(3)]
    (directory/'native-crt.json').write_text(json.dumps(raw,indent=2)+'\n')
    vector('argv.bin',raw['argv_address']);vector('environment.bin',raw['envp_address'])
    (directory/'pgmptr.bin').write_bytes(cstring(scalar('(unsigned)recon_pgmptr')))
