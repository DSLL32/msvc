#!/usr/bin/env python3
"""Focused conversion, entry-profile, allocation and reached Reset controls."""
import argparse,copy,importlib.util,json,re,shutil,tempfile,time
from pathlib import Path
import scaler_state

def elf_controls(native):
    import struct
    from scaler_native_probe import describe
    from provenance import captured_native_binary,sha
    binary=captured_native_binary(native);raw=binary.read_bytes();started=time.perf_counter()
    controls=[('wrong-magic',0,0,1),('wrong-class',4,2,1),('wrong-endian',5,2,1),('wrong-type',16,3,2),('wrong-machine',18,62,2),
        ('wrong-header-size',40,64,2),('section-table-overrun',32,0xfffffff0,4),('section-entry-size',46,0,2),('section-count',48,0,2)]
    table=struct.unpack_from('<I',raw,32)[0];count=struct.unpack_from('<H',raw,48)[0]
    text=next(table+i*40 for i in range(count)if struct.unpack_from('<I',raw,table+i*40+4)[0]==1 and struct.unpack_from('<I',raw,table+i*40+8)[0]&4)
    controls +=[('section-file-overrun',text+16,len(raw)+1,4),('section-address-overflow',text+12,0xffffffff,4)]
    with tempfile.TemporaryDirectory(dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1',prefix='scaler-elf-')as temp:
        path=Path(temp)/'malformed'
        for name,offset,value,width in controls:
            changed=bytearray(raw);changed[offset:offset+width]=value.to_bytes(width,'little');path.write_bytes(changed)
            try:describe(path)
            except AssertionError:continue
            raise AssertionError('accepted '+name)
    return dict(status='PASS',fresh_malformed_elf_controls=len(controls),rejected=[row[0]for row in controls],seconds=time.perf_counter()-started,actual_elf_sha256=sha(binary))

def main():
    parser=argparse.ArgumentParser();parser.add_argument('original',type=Path);parser.add_argument('native',type=Path);args=parser.parse_args()
    started=time.perf_counter()
    spec=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'));trial=importlib.util.module_from_spec(spec);spec.loader.exec_module(trial)
    trial.check(args.original,args.native)
    candidate=json.loads((args.original/'candidate.json').read_text());state=json.loads((args.native/'state.json').read_text())
    events=(args.original/'events.txt').read_text();module=int(re.findall(r'^module name=c1n32_l.dll actual=([0-9a-f]+)',events,re.M)[0],16);sequence=int(re.findall(r'^trap seq=(\d+)',events,re.M)[-1])
    entry,at=scaler_state.boundary(args.original,args.native,state,sequence,module)
    controls=[]
    def json_change(filename,path,value):
        def change(o,n,s):
            f=n/filename;data=json.loads(f.read_text());cursor=data
            for key in path[:-1]:cursor=cursor[key]
            cursor[path[-1]]=value;f.write_text(json.dumps(data))
        return change
    def native(name,filename,path,value):controls.append((name,json_change(filename,path,value)))
    native('qword-width-or-value','scaler-fpu.json',[9,'qword_bytes'],'0000000000000000')
    native('qword-memory-owner','scaler-fpu.json',[9,'qword_address'],0)
    native('actual-metadata-elf','scaler-probe-metadata.json',['elf_sha256'],'0'*64)
    native('actual-metadata-code','scaler-probe-metadata.json',['instructions',7,'bytes'],'90')
    native('actual-metadata-qword-slot','scaler-probe-metadata.json',['result_ebp_displacement'],0)
    native('actual-metadata-converted-pc','scaler-probe-metadata.json',['converted_pc'],0)
    native('conversion-controlword','scaler-fpu.json',[7,'fp','cw'],0x027f)
    native('product-status','scaler-fpu.json',[3,'fp','sw'],0)
    native('temporary-depth','scaler-fpu.json',[2,'fp','live'],[])
    native('temporary-status','scaler-fpu.json',[2,'fp','sw'],0x3021)
    native('temporary-tag','scaler-fpu.json',[4,'fp','tag'],0xffff)
    native('temporary-value','scaler-fpu.json',[2,'fp','registers',0],'00000000000000000000')
    native('final-tag','scaler-fpu.json',[9,'fp','tag'],0)
    native('return-value','scaler-returns.json',[0,'eax'],1)
    native('return-location','scaler-returns.json',[0,'pc'],1)
    native('conversion-mutates-global','scaler-fpu.json',[4,'compiler_state','bss','arenas',1,'base'],1)
    native('source-percentage','scaler-entries.json',[0,'compiler_state','initialized','memory_percent'],101)
    native('scaler-descriptor','scaler-entries.json',[0,'descriptor'],0)
    native('scaler-formal-inventory','scaler-entries.json',[0,'formal_inventory'],[])
    native('initial-status','initial-x87.json',[0,'fp','sw'],0)
    native('initial-controlword','initial-x87.json',[0,'fp','cw'],0x037f)
    native('initial-tag','initial-x87.json',[0,'fp','tag'],0)
    native('profile-once','initial-x87.json',[0,'profile','applications'],2)
    native('profile-reference','initial-x87.json',[0,'profile','reference'],'0'*64)
    native('profile-raw-binding','x87-entry-binding.json',['original_float_save_sha256'],'0'*64)
    native('profile-environment','x87-entry-binding.json',['original_environment_sha256'],'0'*64)
    native('allocation-args','arena-api-entries.json',[0,'args'],[0,0x3200000,0x2000,4])
    native('allocation-formal-omitted','arena-api-entries.json',[0,'formal_inventory'],[])
    native('allocation-error','arena-api-returns.json',[0,'last_error'],50)
    native('allocation-null','arena-api-returns.json',[0,'eax'],0)
    native('allocation-unaligned','arena-api-returns.json',[0,'eax'],0x10001)
    native('allocation-stack-cleanup','arena-api-returns.json',[0,'esp'],0)
    native('allocation-preserved-register','arena-api-returns.json',[0,'preserved','edi'],0)
    native('allocation-fpu-status','arena-api-returns.json',[0,'fp','sw'],0)
    native('allocation-region-unowned','arena-api-returns.json',[0,'maps'],'00010000-00011000 ---p 00000000 00:00 0\n')
    native('reset-this','arena-reset-entries.json',[0,'ecx'],0)
    native('reset-formal-omitted','arena-reset-entries.json',[0,'formal_inventory'],[])
    native('reset-return','arena-reset-entries.json',[0,'return'],1)
    for name,path in [('wrong-arena-base',['compiler_state','bss','arenas',1,'base']),('reserved-size',['compiler_state','bss','arenas',0,'reserve_bytes']),('reset-neighbor',['compiler_state','bss','arenas',0,'reset_value']),('initialized-neighbor',['compiler_state','initialized','compiler_flags'])]:
        def change(o,n,s,path=path):
            cursor=s
            for key in path[:-1]:cursor=cursor[key]
            cursor[path[-1]]^=1
        controls.append((name,change))
    def mutate_original(filename,offset=None,old=None,new=None):
        def change(o,n,s):
            file=o/filename
            if offset is not None:data=bytearray(file.read_bytes());data[offset]^=1;file.write_bytes(data)
            else:
                text=file.read_text();assert old in text;file.write_text(text.replace(old,new,1))
        return change
    controls+= [('provider-code',mutate_original('ftol-provider-code-000.bin',0)),
        ('provider-address',mutate_original('events.txt',old='resolved=78004dc1',new='resolved=78004dc2')),
        ('original-allocation-flags',mutate_original('events.txt',old='kind=00102000',new='kind=00002000')),
        ('original-region-type',mutate_original('events.txt',old=next(line for line in events.splitlines()if line.startswith('arena-region ')),new=next(line for line in events.splitlines()if line.startswith('arena-region ')).replace('type=00020000','type=00040000'))),
        ('original-arena-base',mutate_original('compiler-bss-%03d.bin'%sequence,0x8fb8))]
    with tempfile.TemporaryDirectory(prefix='scaler-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1')as temp:
        root=Path(temp);o=root/'original';n=root/'native';shutil.copytree(args.original,o);shutil.copytree(args.native,n)
        # The direct component checks intentionally bypass immutable transport
        # hashes so mutations reach the actual semantic guards. The full raw
        # positive above checks the real receipt and all those hashes.
        binding=json.loads((n/'x87-entry-binding.json').read_text());binding['original_directory']=str(o.resolve());(n/'x87-entry-binding.json').write_text(json.dumps(binding))
        files={f:f.read_bytes()for directory in (o,n)for f in directory.iterdir()if f.is_file()}
        scaler_state.validate(o,n,entry,state,at,sequence,module,candidate)
        for name,change in controls:
            for file,data in files.items():
                if file.read_bytes()!=data:file.write_bytes(data)
            value=copy.deepcopy(state);change(o,n,value)
            try:scaler_state.validate(o,n,entry,value,at,sequence,module,candidate)
            except(AssertionError,KeyError,ValueError):continue
            raise AssertionError('accepted '+name)
    print(json.dumps(dict(status='PASS',fresh_target_state_controls=len(controls),rejected=[name for name,change in controls],elf_controls=elf_controls(args.native),seconds=time.perf_counter()-started,
        scope='Fresh full raw normal positive plus direct reached scaler/profile/allocation/Reset guards; complete restore positive is separate. Unchanged transport/core controls reused, not rerun.'),indent=2))
if __name__=='__main__':main()
