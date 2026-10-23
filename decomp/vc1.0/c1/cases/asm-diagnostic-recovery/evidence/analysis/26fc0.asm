
../../../toolchains/1.0-nt/bin/C13232.EXE:     file format pei-i386


Disassembly of section .text:

00026fc0 <.text+0xffc0>:
   26fc0:	56                   	push   esi
   26fc1:	8b 74 24 08          	mov    esi,DWORD PTR [esp+0x8]
   26fc5:	c6 06 03             	mov    BYTE PTR [esi],0x3
   26fc8:	c6 46 16 12          	mov    BYTE PTR [esi+0x16],0x12
   26fcc:	66 c7 46 14 00 00    	mov    WORD PTR [esi+0x14],0x0
   26fd2:	83 c6 08             	add    esi,0x8
   26fd5:	83 3e 00             	cmp    DWORD PTR [esi],0x0
   26fd8:	75 3e                	jne    0x27018
   26fda:	81 3d f4 b7 06 00 ff 	cmp    DWORD PTR ds:0x6b7f4,0x7fffffff
   26fe1:	ff ff 7f 
   26fe4:	72 0a                	jb     0x26ff0
   26fe6:	6a 37                	push   0x37
   26fe8:	e8 e3 3a 02 00       	call   0x4aad0
   26fed:	83 c4 04             	add    esp,0x4
   26ff0:	a1 f8 b7 06 00       	mov    eax,ds:0x6b7f8
   26ff5:	39 05 f4 b7 06 00    	cmp    DWORD PTR ds:0x6b7f4,eax
   26ffb:	72 0e                	jb     0x2700b
   26ffd:	83 3d 88 b5 05 00 00 	cmp    DWORD PTR ds:0x5b588,0x0
   27004:	74 05                	je     0x2700b
   27006:	e8 35 05 00 00       	call   0x27540
   2700b:	a1 f4 b7 06 00       	mov    eax,ds:0x6b7f4
   27010:	ff 05 f4 b7 06 00    	inc    DWORD PTR ds:0x6b7f4
   27016:	89 06                	mov    DWORD PTR [esi],eax
   27018:	5e                   	pop    esi
   27019:	c2 04 00             	ret    0x4
