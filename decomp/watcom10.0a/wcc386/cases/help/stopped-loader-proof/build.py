from pathlib import Path
import hashlib,json,os,subprocess,time
ROOT=Path('/root/msvc2');WORK=Path(__file__).resolve().parent
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def win(p):
 p=Path(p).resolve()
 for root,drive in ((ROOT,'R:'),(Path('/root/.ai-tmp/msvc2'),'S:')):
  try:return drive+'\\'+str(p.relative_to(root)).replace('/','\\')
  except ValueError:pass
 raise ValueError(p)
env=os.environ.copy()
for key in ('CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','_MSC_IDE_FLAGS','INCLUDE','LIB','WINEPATH','WINEDLLOVERRIDES'):env.pop(key,None)
env.update(WINEPREFIX='/root/.ai-tmp/vc1.0-capture/wine',WINEDEBUG='-all',WINEARCH='win32',INCLUDE=win(ROOT/'toolchains/5.10/include'),LIB=win(ROOT/'toolchains/5.10/lib'))
p=WORK/'host-environment.json';fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
with os.fdopen(fd,'w') as f:json.dump(dict(sorted(env.items())),f,indent=2)
source=ROOT/'tooling/watcom-capture/win32-broker.c'
commands=[['/usr/bin/wine',win(ROOT/'toolchains/5.10/bin/CL.EXE'),'/nologo','/c','/Od','/MT','/W3','/Fo'+win(WORK/'broker.obj'),win(source)],['/usr/bin/wine',win(ROOT/'toolchains/5.10/bin/LINK.EXE'),'/nologo','/subsystem:console','/out:'+win(WORK/'broker.exe'),'/map:'+win(WORK/'broker.map'),win(WORK/'broker.obj'),'kernel32.lib']]
receipt={'scope':'Build only authored debug broker using MSVC5.10; no original Watcom loader or generated broker execution','source_sha256':sha(source),'environment_sha256':sha(p),'commands':[]}
for name,command in zip(('compile','link'),commands):
 start=time.monotonic();completed=subprocess.run(command,cwd=WORK,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=45)
 (WORK/(name+'.stdout')).write_bytes(completed.stdout);(WORK/(name+'.stderr')).write_bytes(completed.stderr)
 receipt['commands'].append({'command':command,'status':completed.returncode,'seconds':time.monotonic()-start,'stdout_sha256':sha(WORK/(name+'.stdout')),'stderr_sha256':sha(WORK/(name+'.stderr'))})
 (WORK/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
 print(name,completed.returncode,completed.stdout.decode('latin1'),completed.stderr.decode('latin1'))
 if completed.returncode:raise SystemExit(completed.returncode)
receipt['artifacts']={p.name:sha(p) for p in (WORK/'broker.obj',WORK/'broker.exe',WORK/'broker.map')}
(WORK/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(receipt['artifacts'])
