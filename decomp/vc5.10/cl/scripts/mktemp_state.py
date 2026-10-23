"""Decode the observed temporary-name boundary from original memory records."""
import hashlib
import json
from pathlib import Path
import re
import struct

CRT_SHA='e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210'
sha=lambda data:hashlib.sha256(data).hexdigest()


def original(directory):
    directory=Path(directory)
    capture=json.loads((directory/'capture.json').read_text())
    assert capture['loaded_msvcrt_sha256']==CRT_SHA
    provider=(directory/'loaded-msvcrt.bin').read_bytes()
    assert sha(provider)==CRT_SHA
    # The canonical provider's _errno and __doserrno both call getptd,
    # then add8 and12 respectively. The debugger also checks _errno at runtime.
    from binary import PE
    pe=PE(directory/'loaded-msvcrt.bin')
    assert pe.read(pe.base+0x6436,9)==bytes.fromhex('e888b1ffff83c008c3')
    assert pe.read(pe.base+0x642d,9)==bytes.fromhex('e891b1ffff83c00cc3')
    lines=(directory/'mktemp-events.txt').read_text().splitlines()
    assert len(lines)==5,'missing/extra temporary-name observations'
    states={};identities=[]
    for phase,line in [('entry',lines[0]),('before',lines[2]),('after',lines[4])]:
        match=re.fullmatch(r'state phase='+phase+r' teb=([0-9a-f]{8}) tid=(\d+) tls_index=(\d+) ptd=([0-9a-f]{8})',line)
        assert match,'malformed raw error-state observation'
        teb,tid,index,ptd=int(match[1],16),int(match[2]),int(match[3]),int(match[4],16)
        raw=(directory/('mktemp-'+phase+'-teb.bin')).read_bytes()
        thread=(directory/('mktemp-'+phase+'-ptd.bin')).read_bytes()
        assert len(raw)==4096 and len(thread)==116 and index<64
        assert struct.unpack('<I',(directory/('mktemp-'+phase+'-tls-index.bin')).read_bytes())[0]==index
        assert struct.unpack_from('<I',raw,0x18)[0]==teb
        assert struct.unpack_from('<I',raw,0x24)[0]==tid
        assert struct.unpack_from('<I',raw,0xe10+4*index)[0]==ptd
        identities.append((teb,tid,index,ptd))
        states[phase]={'errno':struct.unpack_from('<i',thread,8)[0],
            'doserrno':struct.unpack_from('<I',thread,12)[0],
            'last_error':struct.unpack_from('<I',raw,0x34)[0]}
    assert identities[0]==identities[1]==identities[2]
    call=re.fullmatch(r'call sequence=1 argument=([0-9a-f]{8})',lines[1])
    result=re.fullmatch(r'return sequence=1 result=([0-9a-f]{8}) argument=([0-9a-f]{8})',lines[3])
    assert call and result and call[1]==result[1]==result[2],'result does not alias the input'
    before=(directory/'mktemp-before.bin').read_bytes();after=(directory/'mktemp-after.bin').read_bytes()
    assert before==b'.\\XXXXXX\0' and re.fullmatch(rb'\.\\[a-zA-Z0-9]{6}\x00',after)
    initial=json.loads((directory/'input-directory.json').read_text())['entries']
    names=set()
    for item in initial:
        assert item['name'] not in names and '/' not in item['name'] and '\\' not in item['name']
        names.add(item['name']);raw=(directory/item['name']).read_bytes()
        assert len(raw)==item['bytes'] and sha(raw)==item['sha256'],'original input-directory bytes changed'
    assert names=={'CL.EXE','MSPDB50.DLL','MSVCRT.DLL','config.json','entries.txt','reference-debugger.c','reference-debugger.exe'}
    assert after[2:-1].decode('ascii') not in names|set(capture['files']),'temporary-name collision'
    entry_context=(directory/'main-entry-context.bin').read_bytes()
    entry={**states['entry'],'x87_control_word':struct.unpack_from('<I',entry_context,28)[0]&0xffff}
    return {'entry':entry,'calls':[{'template':before[:-1].decode('ascii'),
        'result':after[:-1].decode('ascii'),'result_alias':True,
        'before':states['before'],'after':states['after']}]}


def native(directory):
    directory=Path(directory)
    expected=json.loads((directory/'mktemp-expected.json').read_text())['calls'][0]
    entry=json.loads((directory/'native-entry-errors.json').read_text())
    process=json.loads((directory/'native-process.json').read_text())['pid']
    assert re.findall(r'^NATIVE_PROCESS_PID=(\d+)$',(directory/'debugger.txt').read_text(),re.M)==[str(process)]
    build=json.loads((directory/'native-build.json').read_text())
    import importlib.util,subprocess
    root=Path(__file__).resolve().parents[4]
    reader=directory/'mktemp_reader.py'
    assert reader.read_bytes()==subprocess.check_output(['git','show',build['shared_revision']+':lib/recon/mktemp.py'],cwd=root),'shared receipt decoder differs from compiled release'
    spec=importlib.util.spec_from_file_location('cl_mktemp_reader',reader)
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    prestate=[expected['before'][key]for key in ('errno','doserrno','last_error')]
    with module.MktempObservation(expected['template'],expected['result'][-6:],prestate) as observation:
        receipt=(directory/'mktemp-receipt.bin').read_bytes()
        assert len(receipt)==observation.size,'truncated/extra native temporary-name receipt'
        observation.file.seek(0);observation.file.write(receipt);observation.file.flush()
        raw=observation.read(process)
    records=json.loads((directory/'native-mktemp-gdb.json').read_text())['calls']
    assert len(records)==1,'missing/extra GDB temporary-name observation'
    gdb=records[0]
    assert gdb['argument_address']==raw['argument'] and gdb['result_address']==raw['result']
    call={'template':raw['before'],'result':raw['after'],'result_alias':raw['argument']==raw['result'],
        'before':{key:raw['before_'+key]for key in ('errno','doserrno','last_error')},
        'after':{key:raw['after_'+key]for key in ('errno','doserrno','last_error')}}
    assert call=={key:gdb[key]for key in call},'shared receipt differs from read-only GDB entry/return state'
    return {'entry':entry,'calls':[call]}


if __name__=='__main__':
    import sys
    print(json.dumps(original(Path(sys.argv[1])),indent=2))
