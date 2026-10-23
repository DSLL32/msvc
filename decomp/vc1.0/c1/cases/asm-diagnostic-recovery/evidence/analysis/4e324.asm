
../../../toolchains/1.0-nt/bin/C13232.EXE:     file format pei-i386


Disassembly of section .text:

0004e324 <.text+0x37324>:
   4e324:	53                   	push   ebx
   4e325:	56                   	push   esi
   4e326:	57                   	push   edi
   4e327:	83 3d 2c b1 05 00 00 	cmp    DWORD PTR ds:0x5b12c,0x0
   4e32e:	74 67                	je     0x4e397
   4e330:	68 42 01 00 00       	push   0x142
   4e335:	e8 16 e0 ff ff       	call   0x4c350
   4e33a:	83 c4 04             	add    esp,0x4
   4e33d:	8b d8                	mov    ebx,eax
   4e33f:	8b fb                	mov    edi,ebx
   4e341:	b9 ff ff ff ff       	mov    ecx,0xffffffff
   4e346:	2b c0                	sub    eax,eax
   4e348:	f2 ae                	repnz scas al,BYTE PTR es:[edi]
   4e34a:	f7 d1                	not    ecx
   4e34c:	8d 71 05             	lea    esi,[ecx+0x5]
   4e34f:	83 e6 fc             	and    esi,0xfffffffc
   4e352:	39 35 04 e2 06 00    	cmp    DWORD PTR ds:0x6e204,esi
   4e358:	7d 0f                	jge    0x4e369
   4e35a:	68 f8 e1 06 00       	push   0x6e1f8
   4e35f:	68 c8 e1 06 00       	push   0x6e1c8
   4e364:	e8 97 c2 ff ff       	call   0x4a600
   4e369:	8b 3d 00 e2 06 00    	mov    edi,DWORD PTR ds:0x6e200
   4e36f:	29 35 04 e2 06 00    	sub    DWORD PTR ds:0x6e204,esi
   4e375:	01 35 00 e2 06 00    	add    DWORD PTR ds:0x6e200,esi
   4e37b:	53                   	push   ebx
   4e37c:	57                   	push   edi
   4e37d:	e8 70 00 00 00       	call   0x4e3f2
   4e382:	c6 00 20             	mov    BYTE PTR [eax],0x20
   4e385:	40                   	inc    eax
   4e386:	8b 0d 24 b0 05 00    	mov    ecx,DWORD PTR ds:0x5b024
   4e38c:	80 c1 31             	add    cl,0x31
   4e38f:	88 08                	mov    BYTE PTR [eax],cl
   4e391:	c6 40 01 00          	mov    BYTE PTR [eax+0x1],0x0
   4e395:	eb 55                	jmp    0x4e3ec
   4e397:	68 43 01 00 00       	push   0x143
   4e39c:	e8 af df ff ff       	call   0x4c350
   4e3a1:	83 c4 04             	add    esp,0x4
   4e3a4:	8b d8                	mov    ebx,eax
   4e3a6:	8b fb                	mov    edi,ebx
   4e3a8:	b9 ff ff ff ff       	mov    ecx,0xffffffff
   4e3ad:	2b c0                	sub    eax,eax
   4e3af:	f2 ae                	repnz scas al,BYTE PTR es:[edi]
   4e3b1:	f7 d1                	not    ecx
   4e3b3:	8d 71 03             	lea    esi,[ecx+0x3]
   4e3b6:	83 e6 fc             	and    esi,0xfffffffc
   4e3b9:	3b 35 04 e2 06 00    	cmp    esi,DWORD PTR ds:0x6e204
   4e3bf:	7e 0f                	jle    0x4e3d0
   4e3c1:	68 f8 e1 06 00       	push   0x6e1f8
   4e3c6:	68 c8 e1 06 00       	push   0x6e1c8
   4e3cb:	e8 30 c2 ff ff       	call   0x4a600
   4e3d0:	8b 3d 00 e2 06 00    	mov    edi,DWORD PTR ds:0x6e200
   4e3d6:	29 35 04 e2 06 00    	sub    DWORD PTR ds:0x6e204,esi
   4e3dc:	01 35 00 e2 06 00    	add    DWORD PTR ds:0x6e200,esi
   4e3e2:	53                   	push   ebx
   4e3e3:	57                   	push   edi
   4e3e4:	e8 09 00 00 00       	call   0x4e3f2
   4e3e9:	c6 00 00             	mov    BYTE PTR [eax],0x0
   4e3ec:	8b c7                	mov    eax,edi
   4e3ee:	5f                   	pop    edi
   4e3ef:	5e                   	pop    esi
   4e3f0:	5b                   	pop    ebx
   4e3f1:	c3                   	ret
