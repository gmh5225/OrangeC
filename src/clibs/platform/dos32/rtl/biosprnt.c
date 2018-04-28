/* Software License Agreement
 * 
 *     Copyright(C) 1994-2018 David Lindauer, (LADSoft)
 * 
 *     This file is part of the Orange C Compiler package.
 * 
 *     The Orange C Compiler package is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version, with the addition of the 
 *     Orange C "Target Code" exception.
 * 
 *     The Orange C Compiler package is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 * 
 *     You should have received a copy of the GNU General Public License
 *     along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 * 
 *     contact information:
 *         email: TouchStone222@runbox.com <David Lindauer>
 */

#include <bios.h>
#include <dpmi.h>

int _RTL_FUNC      biosprint(int __cmd, int __abyte, int __port)
{
   union REGS regs ;
   regs.h.ah = __cmd ;
   regs.h.al = __abyte ;
   regs.w.dx = __port ;
   _int386(0x17,&regs,&regs) ;
   return regs.h.ah ;
}
unsigned _RTL_FUNC _bios_printer(unsigned __cmd, unsigned __port, unsigned __abyte)
{
   return biosprint(__cmd, __abyte, __port) ;
}
