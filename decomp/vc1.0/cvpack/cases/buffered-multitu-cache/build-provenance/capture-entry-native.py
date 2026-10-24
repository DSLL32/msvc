from pathlib import Path
import sys,os,json,subprocess,time
root=Path('/root/msvc2/decomp/vc1.0/cvpack');sys.path.insert(0,str(root/'scripts'));from state import case,prepare,sha
out=Path('/root/.ai-tmp/vc1.0-cvpack/0186-entry-owners-native');out.mkdir();record=case('buffered-multitu-cache');prepare(record,out/'work')
canonical=r'Z:\root\msvc2\toolchains\1.0-nt\bin\CVPACK.EXE'
env=os.environ.copy()
for key in list(env):
 if key.startswith(('WINE','RECON_','MALLOC_'))or key=='GLIBC_TUNABLES'or key in('CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','_MSC_IDE_FLAGS','LINK32_DB'):env.pop(key)
env.update(LC_ALL='C',TZ='PST8PDT',RECON_PROGRAM_PATH=canonical,PATH='',LIB=r'Z:\root\msvc2\toolchains\1.0-nt\lib')
script='''set pagination off
set confirm off
set startup-with-shell off
set disable-randomization off
break main
run /nologo /cvnew /verbose cache.dll
set argv[0] = recon_pgmptr
break remove_lru_page
continue
python
import gdb,json
assert gdb.newest_frame().name()=="remove_lru_page"
roots={name:int(gdb.parse_and_eval(name))for name in("free_pages","used_pages","oldest_page","newest_page","page_count")}
nodes=[];p=roots["used_pages"]
while p:
 assert len(nodes)<16 and p not in [x["address"]for x in nodes]
 raw=bytes(gdb.selected_inferior().read_memory(p,4136));nodes.append(dict(address=p,bytes=raw.hex()));p=int.from_bytes(raw[4124:4128],"little")
assert len(nodes)==roots["page_count"]==16 and roots["free_pages"]==0
json.dump(dict(roots=roots,nodes=nodes,current_module=int(gdb.parse_and_eval("current_module->module"))),open(OUTPUT,"w"),indent=2)
end
quit
'''.replace('OUTPUT',repr(str(out/'entry.json')))
(out/'entry.gdb').write_text(script);start=time.monotonic();p=subprocess.run(['/usr/bin/gdb','--batch','-x',str(out/'entry.gdb'),str(root/'build/native/CVPACK.EXE')],cwd=out/'work',env=env,capture_output=True,timeout=30);(out/'gdb.stdout').write_bytes(p.stdout);(out/'gdb.stderr').write_bytes(p.stderr);assert p.returncode==0,p.stderr
r=dict(scope='Explicit fresh same actual first remove_lru_page call entry; supplementary defined-list input proof, no new matchingINT3/frontier or complete-output claim',elapsed_seconds=time.monotonic()-start,executable_sha256=sha(root/'build/native/CVPACK.EXE'),configuration_sha256=sha(root/'functions.json'),input_sha256=sha(out/'work/cache.dll'),command=p.args,script_sha256=sha(out/'entry.gdb'),environment={k:env[k]for k in('LC_ALL','TZ','RECON_PROGRAM_PATH','PATH','LIB')});(out/'receipt.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r,indent=2))
