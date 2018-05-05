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
;     along with Orange C.  If not, see <http://www.gnu.org/licenses/>.
; 
;     contact information:
;         email: TouchStone222@runbox.com <David Lindauer>
; 

%ifdef __BUILDING_LSCRTL_DLL
[export _wmemcmp]
%endif
[global _wmemcmp]
SECTION code CLASS=CODE USE32
_wmemcmp:
    push	ebp
    mov	ebp,esp
    push	esi
    push	edi
    mov	edi,[ebp+12]
    mov	esi,[ebp+8]
    mov	ecx,[ebp+16]
    cld
    repe	cmpsw
    je	zer
    jc	nega
    mov	eax,1
    jmp	exit
zer:
    sub	eax,eax
    jmp	exit
nega:
    mov	eax,-1
exit:
    pop	edi
    pop	esi
    pop	ebp
    ret
