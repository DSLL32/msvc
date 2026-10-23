
../../../toolchains/1.0-nt/bin/C13232.EXE:     file format pei-i386


Disassembly of section .text:

0004be10 <.text+0x34e10>:
   4be10:	83 ec 24             	sub    esp,0x24
   4be13:	8d 44 24 00          	lea    eax,[esp+0x0]
   4be17:	50                   	push   eax
   4be18:	8b 44 24 2c          	mov    eax,DWORD PTR [esp+0x2c]
   4be1c:	50                   	push   eax
   4be1d:	e8 2e ea 00 00       	call   0x5a850
   4be22:	83 c4 08             	add    esp,0x8
   4be25:	0b c0                	or     eax,eax
   4be27:	75 17                	jne    0x4be40
   4be29:	8b 44 24 2c          	mov    eax,DWORD PTR [esp+0x2c]
   4be2d:	8b 4c 24 1c          	mov    ecx,DWORD PTR [esp+0x1c]
   4be31:	89 08                	mov    DWORD PTR [eax],ecx
   4be33:	b8 01 00 00 00       	mov    eax,0x1
   4be38:	83 c4 24             	add    esp,0x24
   4be3b:	c2 08 00             	ret    0x8
   4be3e:	8b ff                	mov    edi,edi
   4be40:	2b c0                	sub    eax,eax
   4be42:	83 c4 24             	add    esp,0x24
   4be45:	c2 08 00             	ret    0x8
