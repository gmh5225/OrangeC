/* Software License Agreement
 * 
 *     Copyright(C) 1994-2020 David Lindauer, (LADSoft)
 * 
 *     This file is part of the Orange C Compiler package.
 * 
 *     The Orange C Compiler package is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 * 
 *     The Orange C Compiler package is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 * 
 *     You should have received a copy of the GNU General Public License
 *     along with Orange C.  If not, see <http://www.gnu.org/licenses/>.
 * 
 *     As a special exception, if other files instantiate templates or
 *     use macros or inline functions from this file, or you compile
 *     this file and link it with other works to produce a work based
 *     on this file, this file does not by itself cause the resulting
 *     work to be covered by the GNU General Public License. However
 *     the source code for this file must still be made available in
 *     accordance with section (3) of the GNU General Public License.
 *     
 *     This exception does not invalidate any other reasons why a work
 *     based on this file might be covered by the GNU General Public
 *     License.
 * 
 *     contact information:
 *         email: TouchStone222@runbox.com <David Lindauer>
 * 
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <dpmi.h>
 #include "llp.h"

extern int __dtabuflen ;

UWORD __dtaseg;
static SELECTOR __dtasel;

#pragma startup initdta 252
#pragma rundown freedta 3

static void initdta(void)
{
   DPMI_REGS r ;
   DESCRIPTOR desc ;
   int val ;
   dpmi_alloc_real_memory(&__dtasel,&__dtaseg,(__dtabuflen + 15)/16); /* 2048 bytes */
}
static void freedta(void)
{
   dpmi_dealloc_real_memory(__dtasel);
}

int __nametodta(char *name, int index)
{
    int len = strlen(name) + 1;
   dpmi_copy_from_ds(__dtasel,index,name,len);
    return index;
}
int __buftodta(unsigned char *buf, int len)
{
   dpmi_copy_from_ds(__dtasel,0,buf,len);
    return 0;
}
int __dtatobuf(unsigned char *buf, int len)
{
   dpmi_copy_to_ds(buf,__dtasel,0,len);
    return 0;
}
