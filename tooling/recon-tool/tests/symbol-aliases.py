#!/usr/bin/env python3
"""Native-only alias builder controls; never invokes Wine or original tools."""
import copy
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('symbol_aliases',ROOT/'symbol-aliases.py')
helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
ROWS=[
 {'win32_alias':'?Reset@CompilerPlainText@@UAEXXZ','win32_target':'?Reset@CompilerLexer@@UAEXXZ',
  'native_alias':'_ZN17CompilerPlainText5ResetEv','native_target':'_ZN13CompilerLexer5ResetEv'},
 {'win32_alias':'?Trace@CompilerPlainText@@UAEHH@Z','win32_target':'?Trace@CompilerLexer@@UAEHH@Z',
  'native_alias':'_ZN17CompilerPlainText5TraceEi','native_target':'_ZN13CompilerLexer5TraceEi'},
 {'win32_alias':'?Reset@CompilerOtherText@@UAEXXZ','win32_target':'?Reset@CompilerLexer@@UAEXXZ',
  'native_alias':'_ZN17CompilerOtherText5ResetEv','native_target':'_ZN13CompilerLexer5ResetEv'}]

class AliasControls(unittest.TestCase):
 def test_metadata_has_no_sections_and_targets_are_actual_externals(self):
  raw=helper.coff(ROWS);machine,sections,timestamp,table,count,optional,flags=struct.unpack_from('<HHIIIHH',raw)
  self.assertEqual((machine,sections,timestamp,table,count,optional,flags),(0x14c,0,0,20,8,0,0))
  strings=20+18*count;self.assertEqual(struct.unpack_from('<I',raw,strings)[0],len(raw)-strings)
  def symbol(index):
   zero,offset,value,section,kind,storage,aux=struct.unpack_from('<IIIhHBB',raw,20+18*index)
   end=raw.index(b'\0',strings+offset)
   self.assertEqual((zero,value,section,kind),(0,0,0,0x20))
   return raw[strings+offset:end].decode(),storage,aux
  self.assertEqual(symbol(0),(ROWS[0]['win32_target'],2,0));self.assertEqual(symbol(1),(ROWS[1]['win32_target'],2,0))
  for row,index,target in zip(ROWS,(2,4,6),(0,1,0)):
   self.assertEqual(symbol(index),(row['win32_alias'],105,1))
   tag,search,padding=struct.unpack_from('<II10s',raw,20+18*(index+1));self.assertEqual((tag,search,padding),(target,3,b'\0'*10))
 def test_bad_maps_fail_before_outputs(self):
  with tempfile.TemporaryDirectory() as d:
   path=Path(d)/'map.json'
   def denied(value):
    path.write_text(json.dumps(value))
    with self.assertRaises(ValueError):helper.load(path)
   path.write_text('{"schema":1,"schema":1,"aliases":[]}')
   with self.assertRaises(ValueError):helper.load(path)
   base={'schema':1,'aliases':ROWS}
   for value in ({}, {'schema':True,'aliases':ROWS},{'schema':1,'aliases':[]},{**base,'body':'c3'}):denied(value)
   for key,name in [('native_alias','x=0'),('native_target','x+4'),('win32_alias','bad\nname'),('win32_target','é')]:
    v=copy.deepcopy(base);v['aliases'][0][key]=name;denied(v)
   for prefix in ('win32','native'):
    v=copy.deepcopy(base);v['aliases'][1][prefix+'_alias']=v['aliases'][0][prefix+'_alias'];denied(v)
    v=copy.deepcopy(base);v['aliases'][0][prefix+'_alias']=v['aliases'][0][prefix+'_target'];denied(v)
    v=copy.deepcopy(base);v['aliases'][0][prefix+'_target']=v['aliases'][1][prefix+'_alias'];denied(v)
   v=copy.deepcopy(base);v['aliases'][2]['native_target']='another_canonical';denied(v)
 def test_actual_native_vtables_and_missing_canonical(self):
  scratch=Path.home()/'.ai-tmp/msvc2';scratch.mkdir(parents=True,exist_ok=True)
  with tempfile.TemporaryDirectory(prefix='symbol-aliases-',dir=scratch) as d:
   work=Path(d);path=work/'map.json';path.write_text(json.dumps({'schema':1,'aliases':ROWS}))
   obj=work/'members.o';rsp=work/'aliases.rsp';coff=work/'aliases.obj';binary=work/'members'
   subprocess.run(['g++','-m32','-O0','-g','-fno-inline','-fno-exceptions','-fno-rtti','-c',str(ROOT/'tests/alias-members.cpp'),'-o',str(obj)],check=True,capture_output=True)
   subprocess.run([sys.executable,str(ROOT/'symbol-aliases.py'),str(path),'--coff',str(coff),'--native-rsp',str(rsp)],check=True,capture_output=True)
   subprocess.run(['gcc','-m32',str(obj),'-Wl,@'+str(rsp),'-o',str(binary)],check=True,capture_output=True)
   subprocess.run([str(binary)],check=True,capture_output=True)
   names={line.split()[-1]:line.split()[0] for line in subprocess.check_output(['nm','-n',str(binary)],text=True).splitlines() if len(line.split())>=3}
   for row in ROWS:self.assertEqual(names[row['native_alias']],names[row['native_target']])
   bad=copy.deepcopy(ROWS);bad[0]['native_target']='missing_canonical';rsp.write_bytes(helper.native(bad))
   result=subprocess.run(['gcc','-m32',str(obj),'-Wl,@'+str(rsp),'-o',str(work/'missing')],capture_output=True)
   self.assertNotEqual(result.returncode,0);self.assertIn(b'missing_canonical',result.stderr)

if __name__=='__main__':unittest.main()
