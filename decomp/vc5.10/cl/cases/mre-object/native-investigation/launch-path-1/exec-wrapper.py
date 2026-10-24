#!/usr/bin/python3
import os,sys
fd=os.open('/root/.ai-tmp/msvc2/vc5.10/cl/mre-provider-native-1/trace.txt',os.O_CREAT|os.O_WRONLY|os.O_TRUNC,0o600)
os.set_inheritable(fd,True)
os.environ["RECON_TRACE_FD"]=str(fd)
os.execv(sys.argv[1],['S:\\vc5.10\\cl\\mre-object-prefix-5\\CL.EXE', '/nologo', '/Gm', '/Zi', '/c', '/DVALUE=7', '/Fdcase.pdb', 'test.obj'])
