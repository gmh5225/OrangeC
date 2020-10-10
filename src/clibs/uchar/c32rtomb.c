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
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <wchar.h>
#include <locale.h>
#include <uchar.h>
#include "libp.h"

size_t c32rtomb(char* restrict s, char32_t c32, mbstate_t* restrict ps)
{
    char xx[10];

    if (s == NULL)
    {
        return c32rtomb(xx, U'\0', ps);
    }
    if (ps == NULL)
        ps = &__getRtlData()->wcrtomb_st;

    if (c32 < 0x80)
    {
        if (s != NULL)
            *s = (char)c32;
        return 1;
    }

    if ((unsigned)c32 <= 0x7ff)
    {
        if (s != NULL)
        {
            *s++ = 0xc0 + (c32 >> 6);
            *s++ = (c32 & 0x3f) | 0x80;
        }
        return 2;
    }
    if ((unsigned)c32 <= 0xffff)
    {
        if (s != NULL)
        {
            *s++ = 0xe0 + (c32 >> 12);
            *s++ = ((c32 >> 6) & 0x3f) | 0x80;
            *s++ = (c32 & 0x3f) | 0x80;
        }
        return 3;
    }
    if (s != NULL)
    {
        *s++ = 0xf0 + ((c32 >> 18) & 7);
        *s++ = ((c32 >> 12) & 0x3f) | 0x80;
        *s++ = ((c32 >> 6) & 0x3f) | 0x80;
        *s++ = (c32 & 0x3f) | 0x80;
    }
    return 4;
}
