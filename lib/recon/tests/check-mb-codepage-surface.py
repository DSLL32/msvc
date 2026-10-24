"""Typed linkability only; every codepage selection remains denied."""
import subprocess
import sys

binary = sys.argv[1]
subprocess.run([binary], check=True)
for value in ('-4', '-3', '-2', '0', '1252', '932', '2147483647'):
    result = subprocess.run([binary, value], capture_output=True)
    assert result.returncode == 125, (value, result.returncode, result.stderr)
    assert b'unreviewed _setmbcp selection' in result.stderr
print('Shared257-byte table alias/lead-bit surface; seven selections deny125')
