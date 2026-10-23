import subprocess,sys
for index,binary in enumerate(sys.argv[1:]):
    p=subprocess.run([binary],capture_output=True)
    assert p.returncode==0 and not p.stderr and p.stdout==(b'037f 0000\r\n' if index==0 else b'027f 0000\r\n'),(binary,p.returncode,p.stdout,p.stderr)
print('actual direct-main x87 hardware: default037f unchanged, opt-in027f, status0 both')
