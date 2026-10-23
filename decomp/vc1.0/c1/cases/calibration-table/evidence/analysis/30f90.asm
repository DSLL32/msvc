
../../../toolchains/1.0-nt/bin/C13232.EXE:     file format pei-i386


Disassembly of section .text:

00030f90 <.text+0x19f90>:
   30f90:	8b 44 24 08          	mov    eax,DWORD PTR [esp+0x8]
   30f94:	0b c0                	or     eax,eax
   30f96:	75 08                	jne    0x30fa0
   30f98:	2b c0                	sub    eax,eax
   30f9a:	c2 08 00             	ret    0x8
   30f9d:	8d 49 00             	lea    ecx,[ecx+0x0]
   30fa0:	8b 0d 80 b6 06 00    	mov    ecx,DWORD PTR ds:0x6b680
   30fa6:	39 48 08             	cmp    DWORD PTR [eax+0x8],ecx
   30fa9:	75 05                	jne    0x30fb0
   30fab:	2b c0                	sub    eax,eax
   30fad:	c2 08 00             	ret    0x8
   30fb0:	0b c0                	or     eax,eax
   30fb2:	74 09                	je     0x30fbd
   30fb4:	8b 4c 24 04          	mov    ecx,DWORD PTR [esp+0x4]
   30fb8:	39 48 20             	cmp    DWORD PTR [eax+0x20],ecx
   30fbb:	74 02                	je     0x30fbf
   30fbd:	2b c0                	sub    eax,eax
   30fbf:	c2 08 00             	ret    0x8
