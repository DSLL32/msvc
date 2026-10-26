import pathlib,subprocess,sys,tempfile,time,os
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve());admitted=len(sys.argv)>2 and sys.argv[2]=='510';env={k:v for k,v in os.environ.items() if not k.startswith('RECON_MKTEMP_') and k!='RECON_TEMPNAM_PROFILE'}
with tempfile.TemporaryDirectory() as cwd:
 p=pathlib.Path(cwd);(p/'sentinel').write_bytes(b'kept');(p/'legacy').mkdir()
 for mode in (('normal','unbound','missing','foreign-tmp','dir','prefix','null-prefix','low-prefix','wrap-prefix','locale','mb','gate') if admitted else ('legacy',)):
  result=subprocess.run([binary,mode],cwd=cwd,env=env,capture_output=True);expected=0 if mode in ('normal','legacy') else 125
  assert result.returncode==expected and not result.stdout,(mode,result)
  assert bool(result.stderr)==bool(expected),(mode,result)
  assert sorted(x.name for x in p.iterdir())==['legacy','sentinel'] and (p/'sentinel').read_bytes()==b'kept' and not list((p/'legacy').iterdir())
print('boundtargetTMP/hostcontrolindependence/4ownedprefixednames/no-create/real4stream+collision/reopen/close/unlink/error+fullFP/11denials/legacyretention PASS %.6fs'%(time.monotonic()-start))
