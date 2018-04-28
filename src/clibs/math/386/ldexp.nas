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

%include "matherr.inc"

%ifdef __BUILDING_LSCRTL_DLL
[export _ldexp]
[export _ldexpf]
[export _ldexpl]
%endif
[global _ldexp]
[global _ldexpf]
[global _ldexpl]
SECTION data CLASS=DATA USE32

nm	db	"ldexp",0

SECTION code CLASS=CODE USE32
_ldexpf:
    lea	ecx,[esp+4]
    fild	dword[esp+8]
    fld	dword[ecx]
    sub dl,dl
    jmp short ldexp
_ldexpl:
    lea	ecx,[esp+4]
    fild	dword[esp+16]
    fld	tword[ecx]
    mov dl,2
    jmp short ldexp
_ldexp:
    lea	ecx,[esp+4]
    fild	dword[esp+12]
    fld	qword[ecx]
    mov dl,1
ldexp:
    lea eax,[nm]
    call clearmath
    call checkedscale
    jc  xit
    fxch	st1
    popone
xit:
    jmp wrapmath