"""Saved public open packet and native-only domain guards; never runs originals."""
import json
import os
from pathlib import Path
import pty
import struct
import subprocess
import sys
import tempfile
import time

start = time.monotonic()
binary, reference = map(Path, sys.argv[1:3])
explicit = len(sys.argv) > 3 and sys.argv[3] == '510'
capture = json.loads(reference.read_text())
record = next(item for item in capture['input_files'] if item['path'] == 'console-rows.bin')
raw = bytes.fromhex(record['data'][4:])
assert len(raw) == 100 and struct.unpack('<8I', raw[:32]) == (111,222,333,25,111,222,6,1)
expected = raw[32:]
assert struct.unpack('<4I', expected[:16]) == (0xffffffff,6,111,222)

with tempfile.TemporaryDirectory() as directory:
    work = Path(directory)
    (work/'plain').write_bytes(b'ordinary unchanged')
    def run(mode='', *, stdin_file=False, stdout_pipe=False, stderr_pipe=False, terminal=False):
        master = slave = None
        if terminal:
            master, slave = pty.openpty()
        with open(work/'out', 'wb') as out, open(work/'err', 'wb') as err, open(work/'plain', 'rb') as incoming:
            def setup():
                if terminal:
                    import fcntl, termios
                    os.setsid()
                    fcntl.ioctl(slave, termios.TIOCSCTTY, 0)
            result = subprocess.run([str(binary.resolve())] + ([mode] if mode else []),
                cwd=work, stdin=incoming if stdin_file else subprocess.DEVNULL,
                stdout=subprocess.PIPE if stdout_pipe else out,
                stderr=subprocess.PIPE if stderr_pipe else err,
                start_new_session=not terminal,
                preexec_fn=setup if terminal else None,
                pass_fds=(slave,) if terminal else ())
        if terminal:
            os.close(slave);os.close(master)
        data = result.stdout if stdout_pipe else (work/'out').read_bytes()
        errors = result.stderr if stderr_pipe else (work/'err').read_bytes()
        return result.returncode, data, errors
    for mode in ('', 'live'):
        code, data, errors = run(mode)
        assert code == 0 and not errors, (mode, code, data, errors)
        if explicit:
            assert data == expected, (mode, data.hex(), expected.hex())
        else:
            assert struct.unpack('<4I', data[:16]) == (0xffffffff,2,111,222), data
    for mode in ('ordinary','missing'):
        assert run(mode) == (0,b'',b''), mode
    if explicit:
        # The DOS device wins over an unrelated literal host filename.
        (work/'CONOUT$').write_bytes(b'host file untouched')
        assert run() == (0,expected,b'')
        assert (work/'CONOUT$').read_bytes() == b'host file untouched'
        (work/'CONOUT$').unlink()
        for mode in ('access','share','security','disposition','attributes','template','std-handle'):
            code, data, errors = run(mode)
            assert code == 125 and not data and b'CONOUT$' in errors, (mode,code,data,errors)
        for options in ({'stdin_file':True},{'stdout_pipe':True},{'stderr_pipe':True},{'terminal':True}):
            code, data, errors = run(**options)
            assert code == 125 and not data and b'CONOUT$' in errors, (options,code,data,errors)
    assert (work/'plain').read_bytes() == b'ordinary unchanged'
    assert sorted(p.name for p in work.iterdir()) == ['err','out','plain']
print(('explicit510 saved public packet +11 domain denials' if explicit else 'generic legacy path retained') +
      ', errors/full108FP/FILE/flags/namespace PASS %.6fs' % (time.monotonic()-start))
