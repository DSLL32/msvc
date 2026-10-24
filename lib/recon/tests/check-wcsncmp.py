import hashlib,json,pathlib,re,subprocess,sys
binary=pathlib.Path(sys.argv[1]).resolve();raw=pathlib.Path(sys.argv[2]);data=(raw/'stdout.bin').read_bytes()
assert hashlib.sha256(data).hexdigest()=='8c6c751f709bd4377d02de38fe54350c1e55f279b0b937435f3a7dbeacefa7bf'
assert hashlib.sha256((raw/'receipt.json').read_bytes()).hexdigest()=='11b25ec3fc09fd211c6b1e76e58daacccfeb18c0e280eade7ef216712e9bb8a4'
pattern=rb'wcsncmp case=(\d+) count=(\d+) result=(-?\d+) before-errno=(-?\d+) after-errno=(-?\d+) before-error=([0-9a-f]+) after-error=([0-9a-f]+) left=([0-9a-f]+) right=([0-9a-f]+) fp-before=([0-9a-f]+) fp-after=([0-9a-f]+)'
rows=re.findall(pattern,data);assert len(rows)==64
lines=[]
for case,count,result,error,after,last,lastafter,left,right,fp,fpafter in rows:
 assert error==after and last==lastafter and fp==fpafter and len(bytes.fromhex(fp.decode()))==108
 a=bytes.fromhex(left.decode());b=bytes.fromhex(right.decode());assert len(a)==len(b)==32
 words=lambda v:[int.from_bytes(v[n:n+2],'little')for n in range(0,32,2)]
 aw,bw=words(a),words(b);expected=0
 for x,y in zip(aw[:int(count)],bw[:int(count)]):
  if x!=y:expected=x-y;break
  if x==0:break
 assert expected==int(result)
 lines.append(' '.join([count.decode(),result.decode(),error.decode(),'222',str(int(last,16))]+[f'{v:x}'for v in aw+bw]))
fixture=('\n'.join(lines)+'\n').encode()
for mode in ('empty','live'):
 p=subprocess.run([str(binary)]+(['live']if mode=='live'else[]),input=fixture,capture_output=True)
 assert p.returncode==0 and b'full108FP PASS'in p.stdout and not p.stderr,(mode,p.returncode,p.stdout,p.stderr)
for mode in ('null-left','null-right'):
 p=subprocess.run([str(binary),mode],capture_output=True)
 assert p.returncode==125 and b'wcsncmp null positive-count span'in p.stderr,(mode,p.returncode,p.stderr)
print('WCHAR16/count32:64 authenticated original rows, unsigned65536 returns x empty/live108FP, guardpage/zero-count/input/error preservation,2NULL denials PASS')
