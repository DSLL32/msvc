from pathlib import Path
import sys,json,hashlib,re,subprocess,struct,copy
root=Path('/root/msvc2/decomp/vc5.10/link');sys.path.insert(0,str(root/'scripts'));from binary import PE,COFF
scratch=Path('/root/.ai-tmp/msvc2/vc5.10/link/module98-prepare');native=Path('/root/.ai-tmp/msvc2/vc5.10/link/native-4c8uw0gn');elf=native/'compiled/native-LINK.EXE';sha=lambda b:hashlib.sha256(b).hexdigest()
pe=PE(root.parents[2]/'toolchains/5.10/bin/LINK.EXE');mapdata=(root/'build/link.map').read_bytes();symbols={a:int(b,16)for a,b in re.findall(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',mapdata.decode('latin1'),re.M)}
proof=dict(target_sha256=sha(pe.data),old_options_source=dict(path='cases/hello-win32/frontiers/97-coff-reader-review/native/source/project/options.cpp',sha256=sha((root/'cases/hello-win32/frontiers/97-coff-reader-review/native/source/project/options.cpp').read_bytes()),qualification='Earlier separately defined cross_module_types/mac_target storage was a limited native model; retained unchanged, not relabeled.'),current_sources={str(p.relative_to(root)):sha(p.read_bytes())for p in[root/'src/options.cpp',root/'src/full-link.cpp',root/'src/pass-one.cpp',root/'src/input-module.cpp']},original_mac_flag=0x494578,original_distinct_ppc_flag=0x47f17c,original_anchors=[])
for start,size in [(0x446ce7,15),(0x44316f,16),(0x40ed20,22),(0x45ae60,17)]:proof['original_anchors'].append(dict(start=start,bytes=pe.read(start,size).hex()))
assert pe.read(0x446ced,7).hex()=='c6057845490001'
assert pe.read(0x44316f,5).hex()=='a078454900'
assert pe.read(0x40ed20,5).hex()=='a078454900'and pe.read(0x40ed29,5).hex()=='a07cf14700'
assert pe.read(0x45ae6b,6).hex()=='881d7cf14700'
proof['rebuilt_symbols']={k:symbols[k]for k in['_mac_target','_native_ppc_symbol_maps']};assert '_cross_module_types'not in symbols and symbols['_mac_target']!=symbols['_native_ppc_symbol_maps']
objects=[]
for path in sorted((root/'build').glob('*.obj')):
 obj=COFF(path.read_bytes());refs=[]
 for sec in obj.sections:
  for off,sym,kind in sec['relocs']:
   assert sym['name']!='_cross_module_types','retired separate symbol retained'
   if sym['name']not in['_mac_target','_native_ppc_symbol_maps']:continue
   if not sec['flags']&0x20:continue
   refs.append(dict(section=sec['name'],offset=off,symbol=sym['name'],kind=kind,raw_prefix=sec['data'][max(0,off-4):off].hex(),raw_suffix=sec['data'][off+4:off+8].hex()))
 if refs:objects.append(dict(path=str(path.relative_to(root)),sha256=sha(path.read_bytes()),refs=refs))
assert {x['path']for x in objects}=={'build/options.obj','build/full-link.obj','build/pass-one.obj','build/input-module.obj'}
proof['all_current_modeled_code_references']=objects
# Source-symbol correction has one actual original-compiler store to the
# shared DWORDaddress, rather than an unconnected alias declaration.
option=next(x for x in objects if x['path']=='build/options.obj');refs=[x for x in option['refs']if x['symbol']=='_mac_target'];assert len(refs)==1 and refs[0]['raw_suffix'].startswith('01')
# Native GDB executes the actual compiled options store with distinct sentinels.
data=elf.read_bytes();phoff=struct.unpack_from('<I',data,28)[0];phsize,phcount=struct.unpack_from('<HH',data,42)
def readva(address,size):
 for i in range(phcount):
  kind,off,va,_,filesz,memsz,_,_=struct.unpack_from('<8I',data,phoff+i*phsize)
  if kind==1 and va<=address and address+size<=va+filesz:return data[off+address-va:off+address-va+size]
 raise AssertionError('unbacked nativeinstruction')
ns={r[3]:int(r[0],16)for r in[x.split()for x in subprocess.check_output(['nm','-S','--defined-only',str(elf)],text=True).splitlines()]if len(r)==4}
needle=b'\xc6\x05'+struct.pack('<I',ns['mac_target'])+b'\x01';positions=[]
for off in range(len(data)-6):
 if data[off:off+7]==needle:
  for i in range(phcount):
   kind,fo,va,_,filesz,_,_,_=struct.unpack_from('<8I',data,phoff+i*phsize)
   if kind==1 and fo<=off<fo+filesz:positions.append(va+off-fo)
assert len(positions)==1;store=positions[0]
script=scratch/'alias-sentinel.gdb';script.write_text('set pagination off\nset confirm off\nstarti\nset variable mac_target=0x31\nset variable native_ppc_symbol_maps=0x72\nset $pc='+str(store)+'\nstepi\npython\nimport gdb,json\na=int(gdb.parse_and_eval("mac_target"));b=int(gdb.parse_and_eval("native_ppc_symbol_maps"))\nassert (a,b)==(1,0x72),(a,b)\ngdb.execute("set variable native_ppc_symbol_maps=0x47")\nassert int(gdb.parse_and_eval("mac_target"))==1\nprint("ALIAS_SENTINEL_PASS",json.dumps(dict(mac_target=a,ppc_after_mac_store=b,ppc_after_separate_store=int(gdb.parse_and_eval("native_ppc_symbol_maps")))))\nend\nquit\n')
executable=root/'build/native/LINK.EXE';assert executable.read_bytes()==data
run=subprocess.run(['gdb','-q','-nx','-batch','-x',str(script),str(executable)],capture_output=True,text=True);(scratch/'alias-sentinel.stdout').write_text(run.stdout);(scratch/'alias-sentinel.stderr').write_text(run.stderr);assert run.returncode==0 and 'ALIAS_SENTINEL_PASS'in run.stdout
assert executable.read_bytes()==data
proof['native_actual_store']=dict(actual_executed_path=str(executable),elf=str(elf),elf_sha256=sha(data),mac_target=ns['mac_target'],native_ppc_symbol_maps=ns['native_ppc_symbol_maps'],store=store,bytes=needle.hex(),sentinel_stdout_sha256=sha(run.stdout.encode()),script_sha256=sha(script.read_bytes()))
# Explicit binding controls: old duplicate symbol, aliasing distinct backend
# storage, or unrelated literal-store opcode cannot satisfy the exact contract.
def gate(p):
 assert set(p['rebuilt_symbols'])=={'_mac_target','_native_ppc_symbol_maps'}
 assert p['rebuilt_symbols']['_mac_target']!=p['rebuilt_symbols']['_native_ppc_symbol_maps']
 assert p['original_mac_flag']==0x494578 and p['original_distinct_ppc_flag']==0x47f17c
 for row in p['all_current_modeled_code_references']:
  assert row['sha256']==sha((root/row['path']).read_bytes())
  for ref in row['refs']:assert ref['symbol']in p['rebuilt_symbols']
 assert p['native_actual_store']['bytes']==needle.hex()
gate(proof);controls=[]
for label,mutate in [('retired duplicate symbol',lambda p:p['rebuilt_symbols'].__setitem__('_cross_module_types',1)),('backend aliases Macbyte',lambda p:p['rebuilt_symbols'].__setitem__('_native_ppc_symbol_maps',p['rebuilt_symbols']['_mac_target'])),('wrong original alias',lambda p:p.__setitem__('original_mac_flag',0x47f17c)),('wrong actual COFF',lambda p:p['all_current_modeled_code_references'][0].__setitem__('sha256','00'*32)),('old separate relocation',lambda p:p['all_current_modeled_code_references'][0]['refs'][0].__setitem__('symbol','_cross_module_types')),('wrong actual store',lambda p:p['native_actual_store'].__setitem__('bytes','90'))]:
 changed=copy.deepcopy(proof);mutate(changed)
 try:gate(changed)
 except AssertionError:controls.append(label)
 else:raise AssertionError(label+' accepted')
proof['focused_static_rejection_controls']=controls
(scratch/'mac-flag-alias-proof.json').write_text(json.dumps(proof,indent=2)+'\n');print('actual options store/distinct PPC sentinel and6 static binding controls PASS')
