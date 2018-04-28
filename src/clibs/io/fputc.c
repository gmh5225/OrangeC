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

#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <locale.h>
#include <wchar.h>
#include <io.h>
#include "libp.h"

int _RTL_FUNC fputc(int c, FILE *stream)
{
    int rv;
    if (stream->token != FILTOK) {
        errno= _dos_errno = ENOENT;
        return EOF;
    }
    if (stream->extended->orient == __or_wide) {
        errno = EINVAL;
        return EOF;
    }
    stream->extended->orient = __or_narrow;
    stream->flags &= ~_F_VBUF;
    if (!(stream->flags & _F_WRIT)) {
        stream->flags |= _F_ERR;
        errno = EFAULT;
        return EOF;
    }
    if ((stream->flags & _F_IN) || 
            stream->buffer && ( stream->flags & _F_OUT) 
                && stream->level >= 0) {
        if (fflush(stream))
            return EOF;
        goto join;
    }
    else {
        if (!(stream->flags & _F_OUT)) {
join:
            stream->flags &= ~_F_IN;
            stream->flags |= _F_OUT;
            stream->level = -stream->bsize;
            stream->curp = stream->buffer;
        }
    }
    if (stream->buffer) {
        if (stream->level < 0) // if a buffered string don't write beyond the end
        {
            ++stream->level;
            *stream->curp++ = (unsigned char)c;
        }
        if (!(stream->flags & _F_BUFFEREDSTRING))
        {
            if (stream->level >= 0 || c == '\n' && (stream->flags & _F_LBUF)) {
                if (fflush(stream))
                    return EOF ;
                stream->flags &= ~_F_IN;
                stream->flags |= _F_OUT;
                stream->level = -stream->bsize;
                stream->curp = stream->buffer;
            }
        }
    }		
    else {
        if (write(fileno(stream),&c,1) < 0) {
            stream->flags |= _F_ERR;
            errno = EIO;
            return EOF;
        }
        if (eof(fileno(stream)))
            stream->flags |= _F_EOF;
    }
    return c;
}
int _RTL_FUNC _fputc(int c, FILE *stream)
{
    return fputc(c,stream);
}
#undef putc
#undef putchar
int _RTL_FUNC putc(const int c, FILE *stream)
{
    return fputc(c,stream);
}
int _RTL_FUNC putchar(const int c)
{
    return fputc(c,stdout);
}
int _RTL_FUNC fputchar(int c)
{
    return fputc(c,stdout);
}
int _RTL_FUNC _fputchar(int c)
{
    return fputc(c,stdout);
}
