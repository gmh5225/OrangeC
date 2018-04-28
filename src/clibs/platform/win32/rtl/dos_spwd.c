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

#include <errno.h>
#include <windows.h>
#include <dos.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <wchar.h>
#include <locale.h>
#include "libp.h"

int _RTL_FUNC _dos_setpwd(char *buf)
{
    char dir[256],name[6];
   int drv ;
   _dos_getdrive(&drv) ;
   dir[0] = drv + '@';
    dir[1] = ':' ;
   dir[2] = '\\' ;
   if (buf[0] != '\\') {
      _dos_getpwd(dir+3, dir[0] - '@');
      strcat(dir,"\\") ;
   } else {
      dir[2] = 0;
    }
    strcat(dir,buf);
    name[0] = '=' ;      
    name[1] = dir[0] ;
    name[2] = ':' ;
    name[3] = '\\' ;
    name[4] = 0;
    SetEnvironmentVariable(name,dir) ;
    return !SetCurrentDirectory(dir) ;
}
