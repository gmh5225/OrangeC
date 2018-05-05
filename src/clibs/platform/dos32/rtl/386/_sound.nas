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

[global __sound]
[global _sound]
[global __nosound]
[global _nosound]
SECTION code CLASS=CODE USE32
__sound:
_sound:
        mov eax,1234ddh
        sub edx,edx
        mov ecx,[esp+4]
        cmp ecx,eax
        jnb ns
        or  ecx,ecx
        jz  ns
        div ecx
        push eax
        mov al,0b6h
        out 43h,al
        pop eax
        out 42h,al
        xchg al,ah
        out 42h,al
        in al,61h
        or al,3
        out 61h,al
ns:
        ret

__nosound:
_nosound:
        in al,61h
        and al,0fch
        out 61h,al
        ret
