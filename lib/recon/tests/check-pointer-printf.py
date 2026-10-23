import hashlib,json,os,struct,subprocess,sys,tempfile
from pathlib import Path
v=json.loads((Path(__file__).parent/'pointer-printf-original.json').read_text())
assert hashlib.sha256((Path(__file__).parent/'pointer-printf-reference.c').read_bytes()).hexdigest()==v['source_sha256']
binary=str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory() as tmp:
    result=subprocess.run([binary],cwd=tmp,capture_output=True)
    assert result.returncode==0 and not result.stdout and not result.stderr,(result.returncode,result.stderr)
    data=(Path(tmp)/'states.bin').read_bytes();expected=[r for r in v['rows'] if r['inputs'][2]<3]
    assert len(data)==len(expected)*8520
    for n,want in enumerate(expected):
        row=data[n*8520:(n+1)*8520];header=list(struct.unpack_from('<12I',row));count=header[5]
        def file(offset):
            words=list(struct.unpack_from('<8I',row,offset));assert words[2]==header[4]
            words[0]-=header[4];words[2]=1;return words
        actual={'inputs':header[:4],'return':count,'errors':header[6:],'before_file':file(48),'after_file':file(80),'text':row[4208:4208+count].decode('ascii')}
        assert actual==want,(n,actual,want)
        assert row[112:4208]==b'Z'*4096 and row[4208+count:8304]==b'Z'*(4096-count)
        assert row[8304:8412]==row[8412:8520],('x87',n)
    for mode in ('bad-prefix','bad-space','bad-chain','bad-escape-chain','width','zero','left','precision','length','mixed','multiple','star','trailing','nonascii','locale','codepage','max'):
        result=subprocess.run([binary],cwd=tmp,env=dict(os.environ,RECON_POINTER_PRINTF_NEGATIVE=mode),capture_output=True)
        assert result.returncode==125,(mode,result.returncode,result.stderr)
    result=subprocess.run([binary],cwd=tmp,env=dict(os.environ,RECON_POINTER_PRINTF_NEGATIVE='escaped'),capture_output=True)
    assert result.returncode==0 and result.stdout==b'%p' and not result.stderr
print('72 canonical plain-pointer rows: actualDWORD/null/highbits, counts/fullFILE/all4096tailbytes/errors/live108x87;17denials+escaped-literal control')
