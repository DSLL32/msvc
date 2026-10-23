
../../../toolchains/1.0-nt/bin/C13232.EXE:     file format pei-i386


Disassembly of section .text:

0004e3f2 <.text+0x373f2>:
   4e3f2:	53                   	push   ebx
   4e3f3:	8b 44 24 08          	mov    eax,DWORD PTR [esp+0x8]
   4e3f7:	8b 54 24 0c          	mov    edx,DWORD PTR [esp+0xc]
   4e3fb:	8b c8                	mov    ecx,eax
   4e3fd:	40                   	inc    eax
   4e3fe:	8a 1a                	mov    bl,BYTE PTR [edx]
   4e400:	88 19                	mov    BYTE PTR [ecx],bl
   4e402:	42                   	inc    edx
   4e403:	0a db                	or     bl,bl
   4e405:	75 f4                	jne    0x4e3fb
   4e407:	48                   	dec    eax
   4e408:	5b                   	pop    ebx
   4e409:	c2 08 00             	ret    0x8
