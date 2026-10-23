"""Ordinary C test sources, compiled only by the untouched original toolchain."""
from pathlib import Path
HERE=Path(__file__).resolve().parent
for unit in range(2):
    lines=['/*2048 distinct ordinary external definitions; fixture source only. */']
    for index in range(unit*2048,(unit+1)*2048):
        lines.append('int symbol_%05d = %d;'%(index,index))
    if unit==0:
        lines+=['extern int symbol_02048;',
                'int entry(void) { return symbol_00000 + symbol_02048 ==2048 ?0 :1; }']
    (HERE/('unit%d.c'%unit)).write_text('\n'.join(lines)+'\n')
