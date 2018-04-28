; Software License Agreement
; 
;     Copyright(C) 1994-2018 David Lindauer, (LADSoft)
; 
;     This file is part of the Orange C Compiler package.
; 
;     The Orange C Compiler package is free software: you can redistribute it and/or modify
;     it under the terms of the GNU General Public License as published by
;     the Free Software Foundation, either version 3 of the License, or
;     (at your option) any later version, with the addition of the 
;     Orange C "Target Code" exception.
; 
;     The Orange C Compiler package is distributed in the hope that it will be useful,
;     but WITHOUT ANY WARRANTY; without even the implied warranty of
;     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;     GNU General Public License for more details.
; 
;     You should have received a copy of the GNU General Public License
;     along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
; 
;     contact information:
;         email: TouchStone222@runbox.com <David Lindauer>

%ifdef __BUILDING_LSCRTL_DLL
[export _memcpy]
%endif
[global _memcpy]
[global memcpy_x]
SECTION code CLASS=CODE USE32

_memcpy:
    push ebx
    mov ecx,[esp+ 16]
    jecxz	x2
    mov	edx,[esp+8]
    mov	ebx,[esp+12]
memcpy_x:	; from MEMMOVE
    dec	edx
lp1:
    inc	edx
    test	dl,3
    jz	lp
    mov	al,[ebx]
    inc	ebx
    mov	[edx],al
    loop lp1
    jecxz	x2
lp:
    cmp	ecx,BYTE 4
    jb	c1
    mov	eax,[ebx]
    add	ebx,BYTE 4
    mov [edx],eax
    sub	ecx,BYTE 4
    add	edx,BYTE 4
    jmp short lp
c1:
    jecxz	x2
    dec	edx
lp2:
    inc	edx
    mov	al,[ebx]
    inc	ebx
    mov [edx],al
    loop	lp2
x2:
    mov eax,[esp+8]
    pop	ebx
    ret
    