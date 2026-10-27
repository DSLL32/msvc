from pathlib import Path
import subprocess,sys
for index,binary in enumerate(sys.argv[1:]):
    binary=str(Path(binary).resolve())
    result=subprocess.run([binary],capture_output=True)
    expected=125 if index in (1,2) else 0
    assert result.returncode==expected and not result.stdout,(binary,result)
    if expected: assert b'ctime explicit MSVCRT510 profile' in result.stderr
    else: assert not result.stderr,(binary,result.stderr)
    if index==0:
        for mode in ('null','wrap'):
            result=subprocess.run([binary,mode],capture_output=True)
            assert result.returncode==125 and b'ctime input span' in result.stderr,(mode,result)
print('MSVCRT510 ctime exact26B/day widths/static lifetime/negative/input/errors/FP and profile isolation pass')
