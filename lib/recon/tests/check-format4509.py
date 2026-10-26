"""Registered message4509 only, native targeted formatter checks."""
import importlib.util,json,pathlib,struct,subprocess,sys,tempfile,time
start=time.monotonic();root=pathlib.Path(__file__).resolve().parents[3]
spec=importlib.util.spec_from_file_location('resources',root/'lib/recon/resources.py');resources=importlib.util.module_from_spec(spec);spec.loader.exec_module(resources)
binary=str(pathlib.Path(sys.argv[1]).resolve());admitted=len(sys.argv)>2 and sys.argv[2]=='510'
owner='62f51b0d705d10426d5b38fc6340859d0bfc829d9d93f27d12a6caa8a2ebe6c6'
text="%1 (%2!d!): warning RC4509 : Resource types '%3' and '%4' were both mapped to '%5'\r\n"
with tempfile.TemporaryDirectory(dir='/root/.ai-tmp') as temporary:
    path=pathlib.Path(temporary)/'provider'
    def write(template):resources.write_resources(path,owner,[dict(kind='message',id=4509,language=1033,text=template),dict(kind='message',id=10001,language=1033,text='Version %1 build %2!d!\r\n')])
    write(text)
    for mode,number in [('mac','44'),('mac','-2147483648'),('mac','2147483647'),('macshort','44'),('macnull','44'),('machigh','44'),('macoverlap','44'),('help','-1')]:
        p=subprocess.run([binary,str(path),mode,number],capture_output=True)
        success=mode=='help' or admitted and mode=='mac'
        assert p.returncode==(0 if success else 125),(mode,number,p)
        if success:
            expected=(('Version help build '+number+'\r\n') if mode=='help' else ("mac-resources.rc ("+number+"): warning RC4509 : Resource types '10' and '240' were both mapped to 'HEXA'\r\n")).encode()
            assert p.stdout==struct.pack('<I',len(expected))+expected+bytes(2048-len(expected)) and not p.stderr,(mode,p)
        else:assert not p.stdout and b'unsupported native operation:' in p.stderr
    for bad in [text.replace('%5','%4'),text.replace('%5','%5!ws!'),text.replace('%3','%6'),text.replace('%2!d!','%2!u!')]:
        write(bad);p=subprocess.run([binary,str(path),'mac','44'],capture_output=True)
        assert p.returncode==125 and not p.stdout and b'unsupported native operation:' in p.stderr
print('registered4509 typed five inserts/actualwarning/signed edges/count/NUL/fullbuffer/errors/live108FP; eight denials and oldhelp unchanged PASS %.6fs'%(time.monotonic()-start))
