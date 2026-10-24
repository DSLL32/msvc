"""Normal target outcome, including the original first-entry trace ownership."""
import re
from binary import PE


def decode(read,config,target):
    expected={record['start']:(record['native_symbol'],target.read(record['start'],1)[0])for record in config['driver']if record['name']in config['accepted']}
    rows=[]
    for row in read('entries.txt').decode('ascii').splitlines():
        match=re.fullmatch(r'([0-9a-f]+) ([0-9]+) (\w+)',row);assert match
        offset,byte,name=match.groups();rows.append((target.base+int(offset,16),name,int(byte)))
    assert len(rows)==len(expected)and {address:(name,byte)for address,name,byte in rows}==expected
    events=read('events.txt').decode('ascii');trace=[];seen=set()
    for address,name in re.findall(r'^entry event=\d+ address=([0-9a-f]{8}) name=(\w+)$',events,re.M):
        address=int(address,16);assert address in expected and expected[address][0]==name
        assert name not in seen,'entry observer repeated already seen function'
        seen.add(name);trace.append(name)
    assert trace and read('trace.txt')==(''.join(name+'\n'for name in trace)).encode('ascii')
    assert re.findall(r'^exit event=\d+ status=(\d+)$',events,re.M)==['0']and not re.search(r'^trap |^exception ',events,re.M)
    return {'status_DWORD':0,'stdout_hex':read('stdout.bin').hex(),'stderr_hex':read('stderr.bin').hex(),'first_entry_trace':trace,'artifacts':{'case.idb':{'bytes':len(read('case.idb')),'sha256':__import__('hashlib').sha256(read('case.idb')).hexdigest()}},'qualification':'Actual full parent normal outcome; no invented final CRT/FILE state beyond observed post-Close.'}
