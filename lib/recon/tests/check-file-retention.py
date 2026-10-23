import subprocess,sys,tempfile
from pathlib import Path
binary=str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory(prefix='file-retention-',dir=Path.home()/'.ai-tmp/msvc2') as work:
    result=subprocess.run([binary],cwd=work,capture_output=True)
    assert result.returncode==0 and not result.stdout and not result.stderr,(result.returncode,result.stderr)
print('cold FILE/reused4096,16,512 capacities+charbuf/ownership/remainingresets/fullFP/failed-open unchanged passed')
