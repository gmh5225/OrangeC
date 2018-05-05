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
 *     along with Orange C.  If not, see <http://www.gnu.org/licenses/>.
 * 
 *     contact information:
 *         email: TouchStone222@runbox.com <David Lindauer>
 * 
 */

#define GUI 1
#define DLL 2

extern void _import _exit(int);
#include <windows.h>
#include <setjmp.h>
#include <string.h>

extern char INITSTART[], INITEND[], EXITSTART[], EXITEND[], BSSSTART[], BSSEND[];
extern __import int _argc;
extern __import char **_argv;
extern __import char **_environ;
extern __import char *_oscmd;
extern __import char *_osenv;
static unsigned	dllexists = 0;
static void _dorundown(void);
// in the follow, the args are ONLY valid for DLLs
int __stdcall DllEntryPoint(HINSTANCE hInst, DWORD fdwReason, LPVOID lpvReserved);
void __stdcall __import ___lsdllinit(DWORD flags, void (*rundown)(void), unsigned *exceptBlock);
void __srproc(char *, char *);
int __stdcall ___lscrtl_startup(HINSTANCE hInst, DWORD fdwReason, LPVOID lpvReserved)
{
    int flags;
    int rv;
    unsigned exceptBlock[2];
    char quote;
    char *cmd;
    asm mov [flags],edx;
    quote = 0;
    ___lsdllinit(flags, _dorundown, exceptBlock);
    if (!(flags & DLL) || fdwReason == DLL_PROCESS_ATTACH)
    {   
        if (flags & DLL)
        {
            if (!dllexists)
                dllexists++;
            else
            {
                MessageBox(0,"NonShared data segment required","Error",0);
                return 1;
            }
        }
    }
    __srproc(INITSTART, INITEND);
    if (flags & DLL)
    {
        rv = DllEntryPoint(hInst, fdwReason, lpvReserved) + 1;
    }
    else if (flags & GUI)
    {
        cmd = _oscmd;
        while ((!quote || *cmd != ' ') && *cmd)
        {
            if (*cmd == '"')
                quote ^= *cmd;
            cmd++;
        }
        while (*cmd && *cmd == ' ') cmd++;
        rv = WinMain(hInst, 0, cmd, 10) + 1;
    }
    else
    {
        rv = main(_argc, _argv, _environ) + 1;
    }
    if ((flags & DLL) && fdwReason == DLL_PROCESS_DETACH)
    {
        __srproc(EXITSTART, EXITEND);
    }
    rv--;
    if (!(flags & DLL))
    {
        _exit(rv);
        ExitProcess(rv); // needed to satisfy the debugger
    }
    return rv;
}
static void _dorundown(void)
{
    __srproc(EXITSTART, EXITEND);
}
