"""Read stopped native FILE/CRT/Win32 slots and Linux descriptor destinations."""
import json
import os
from pathlib import Path
import stat
import struct
import gdb


def capture(directory,phase):
    directory=Path(directory);inferior=gdb.selected_inferior();pid=inferior.pid
    def value(expression):return int(gdb.parse_and_eval(expression))&0xffffffff
    lowio=[value('recon_osfhnd[%d]'%i)for i in range(3)]
    flags=[value('recon_osfile[%d]'%i)for i in range(3)]
    win32=[value('(unsigned)standard_handles[%d]'%i)for i in range(3)]
    iob=value('(unsigned)recon_iob');raw=bytes(inferior.read_memory(iob,96))
    (directory/('streams-'+phase+'-iob.bin')).write_bytes(raw)
    outputs={name:os.stat(directory/(name+'.bin'))for name in ('stdout','stderr')}
    def query(handle):
        descriptor=handle-1;path=Path('/proc')/str(pid)/'fd'/str(descriptor);info=os.stat(path)
        position=int(next(row.split(':',1)[1]for row in (Path('/proc')/str(pid)/'fdinfo'/str(descriptor)).read_text().splitlines()if row.startswith('pos:')))
        if stat.S_ISREG(info.st_mode):kind=1
        elif stat.S_ISCHR(info.st_mode):kind=2
        else:raise AssertionError('unknown captured native descriptor kind')
        role='stdin' if kind==2 and info.st_rdev==os.stat('/dev/null').st_rdev else None
        for name,reference in outputs.items():
            if info.st_dev==reference.st_dev and info.st_ino==reference.st_ino:role=name
        assert role is not None,'unknown native stream destination'
        return {'handle':handle,'type':kind,'role':role,'position':position if kind==1 else None,
            'device':info.st_dev,'inode':info.st_ino}
    result={'pid':pid,'crt':[query(handle)for handle in lowio],
        'win32':[query(handle)for handle in win32],'osfile':flags,
        'errors':{'errno':value('(int)errno'),'doserrno':value('recon_doserrno'),
            'last_error':value('last_error'),'x87_control_word':value('$fctrl')}}
    config=json.loads((directory/'config.json').read_text())
    if '_prepare_jobs' in config['accepted']:result['mre_calls']=value('mre_call_count')
    (directory/('streams-'+phase+'.json')).write_text(json.dumps(result,indent=2)+'\n')
