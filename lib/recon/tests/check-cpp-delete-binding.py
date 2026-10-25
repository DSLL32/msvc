"""Direct scalar-delete import binding and NULL state preservation, native only."""
import subprocess
import sys
import time

start = time.monotonic()
obj, exe = sys.argv[1:]
symbols = subprocess.check_output(['nm', obj], text=True)
assert ' U recon_cpp_new' in symbols and ' U recon_cpp_delete' in symbols, symbols
assert '_ZdlPv' not in symbols and '_Znwj' not in symbols, symbols
assert ' U __cyg_profile_func_enter' in symbols, symbols
assert 'libstdc++' not in subprocess.check_output(['readelf', '-d', exe], text=True)
for args in ([], ['live']):
    result = subprocess.run([exe, *args], capture_output=True)
    assert result.returncode == 0 and not result.stdout and not result.stderr, result
print('direct scalar new/delete bindings; NULL preserves live/dead ownership, '
      'payload, errors and empty/live108FP PASS %.6fs' % (time.monotonic()-start))
