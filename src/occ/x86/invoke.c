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
#include <process.h>
#include <string.h>
#include <stdlib.h>
#include "be.h"
#include "winmode.h"
 
#define TEMPFILE "$$$OCC.TMP"

extern COMPILER_PARAMS cparams;
extern int prm_targettype;
extern int prm_crtdll;
extern int prm_lscrtdll;
extern int prm_msvcrt;
extern int showBanner;
extern int verbosity;

char *winflags[] = 
{
    "/T:CON32 ", "/T:GUI32 ", "/T:DLL32 ", 
    "/T:PM ", "/T:DOS32 ", "/T:BIN ", "/T:CON32;sdpmist32.bin ",
    "/T:CON32;shdld32.bin ",
};
char *winc0[] = 
{
    "c0xpe.o", "c0pe.o", "c0dpe.o", "c0pm.o", "c0wat.o", "", "c0xpe.o", "c0hx.o",
    "c0xls.o", "c0ls.o", "c0dls.o", "c0om.o", "c0wat.o", "", "c0xpe.o", "c0hx.o"
};


LIST *objlist,  *asmlist,  *liblist,  *reslist,  *rclist;
static char outputFileName[256];

#ifdef MICROSOFT
#define system(x) winsystem(x)
extern int winsystem(const char *);
#endif
static void InsertFile(LIST **r, char *name, char *ext)
{

    char buf[256],  *newbuffer;
    LIST *lst;
    strcpy(buf, name);
    if (!outputFileName[0])
    {
        strcpy(outputFileName, name);
        StripExt(outputFileName);
        strcat(outputFileName, ".exe");
    }
    if (ext)
    {
        StripExt(buf);
        strcat(buf, ext);
    }
    lst = *r;
    while (lst)
    {
        if (!strcasecmp(lst->data, buf))
            return;
        lst = lst->next;
    }
    newbuffer = (char*)malloc(strlen(buf) + 1);
    if (!newbuffer)
        return ;
    strcpy(newbuffer, buf);

    /* Insert file */
    while (*r)
        r = &(*r)->next;
    *r = malloc(sizeof(LIST));
    if (!r)
        return ;
    (*r)->next = 0;
    (*r)->data = newbuffer;
}

/*-------------------------------------------------------------------------*/

int InsertExternalFile(char *name)
{
    char buf[260], *p;
    if (HasExt(name, ".asm") || HasExt(name,".nas") || HasExt(name, ".s"))
    {
        InsertFile(&objlist, name, ".o");
        InsertFile(&asmlist, name, 0);
        return 1; /* compiler shouldn't process it*/
    }
    else if (HasExt(name, ".l"))
    {
        InsertFile(&liblist, name, 0);
        return 1;
    }
    else if (HasExt(name, ".rc"))
    {
        InsertFile(&reslist, name, ".res");
        InsertFile(&rclist, name, 0);
        return 1;
    }
    else if (HasExt(name, ".res"))
    {
        InsertFile(&reslist, name, 0);
        return 1;
    }
    else if (HasExt(name, ".o"))
    {
        InsertFile(&objlist, name, 0);
        return 1;
    }
    p = strrchr(name, '\\');
    if (!p)
         p = strrchr(name, '/');
    if (!p)
        p = name;
    else
        p++;
    strcpy(buf, p);
    InsertFile(&objlist, buf, ".o");
    
    // compiling via assembly
    if (cparams.prm_asmfile && !cparams.prm_compileonly)
    {
        InsertFile(&asmlist, buf, ".asm");
    }
    return 0; /* compiler should process it*/
}

/*-------------------------------------------------------------------------*/

void InsertOutputFileName(char *name)
{
    strcpy(outputFileName, name);
}

/*-------------------------------------------------------------------------*/

int RunExternalFiles(char *rootPath)
{
    char root[260];
    char args[1024], *c0;
    char spname[2048];
    char outName[260] ,*p;
    int rv;
    char temp[260];
    int i;
    char verbosityString[20];

    memset(verbosityString, 0, sizeof(verbosityString));
    if (verbosity > 1)
    {
        verbosityString[0] = '-';
        memset(verbosityString + 1, 'y', verbosity > sizeof(verbosityString) - 2 ? sizeof(verbosityString) - 2 : verbosity);
    }
    strcpy(root, rootPath);
    p = strrchr(root, '\\');
    if (!p)
         p = strrchr(root, '/');
    if (!p)
        p = root;
    else
        p++;
    *p = 0;
    temp[0] = 0;
    strcpy(outName, outputFileName);
    if (objlist && outName[0] && outName[strlen(outName)-1] == '\\')
    {
        strcat(outName, objlist->data);
        StripExt(outName);
        strcat(outName, ".exe");
        strcpy(temp, outputFileName);
    }
//    p = strrchr(outName, '.');
//    if (p && p[1] != '\\')
//        *p = 0;
    while (asmlist)
    {
        sprintf(spname, "\"%soasm.exe\" %s \"%s\"", root, !showBanner ? "-!" : "", asmlist->data);
        if (verbosity)
            printf("%s\n", spname);
        rv = system(spname);
        if (rv)
            return rv;
        asmlist = asmlist->next;
    }
    if (beGetIncludePath)
        sprintf(args, "\"-i%s\"", beGetIncludePath);
    else
        args[0] = 0;
    while (rclist)
    {
        sprintf(spname, "\"%sorc.exe\" -r %s %s \"%s\"", root, !showBanner ? "-!" : "", args, rclist->data);
        if (verbosity)
            printf("%s\n", spname);
        rv = system(spname);
        if (rv)
            return rv;
        rclist = rclist->next;
    }
    if (objlist)
    {
        FILE *fil = fopen(TEMPFILE, "w");
        if (!fil)
            return 1;
        
        strcpy(args, winflags[prm_targettype]);
            
        c0 = winc0[prm_targettype + prm_lscrtdll * 8];
        if (cparams.prm_debug)
        {
//            strcat(args, " /DEB");
            if (prm_targettype == DOS)
                c0 = "c0pmd.o";
            else if (prm_targettype == DOS32A)
                c0 = "c0watd.o";
            strcat(args, " /v");
        }
        fprintf(fil, "  %s", c0);
        while (objlist)
        {
            fprintf(fil, " \"%s%s\"", temp, objlist->data);
            objlist = objlist->next;
        }
        fprintf(fil, "  \"/o%s\" ", outName);
        while (liblist)
        {
            fprintf(fil, " \"%s\"", liblist->data);
            liblist = liblist->next;
        }
        if (prm_msvcrt)
            fprintf(fil," climp.l msvcrt.l ");
        else if (prm_lscrtdll)
            fprintf(fil," climp.l lscrtl.l ");
        else if (prm_crtdll)
            fprintf(fil, " climp.l crtdll.l ");
        else if (prm_targettype == DOS || prm_targettype == DOS32A || prm_targettype == RAW || prm_targettype == WHXDOS)
        {
            if (cparams.prm_farkeyword)
                fprintf(fil, " farmem");
            fprintf(fil, " cldos.l ");
        }
        else
            fprintf(fil, " climp.l clwin.l ");
        while (reslist)
        {
            fprintf(fil, " \"%s\"", reslist->data);
            reslist = reslist->next;
        }
        fclose(fil);
        sprintf(spname, "\"%solink.exe\" %s /mx /c+ %s %s @"TEMPFILE, root, !showBanner ? "-!" : "", args, verbosityString);
        if (verbosity) {
            FILE *fil = fopen(TEMPFILE, "r");
            printf("%s\n", spname);
            if (fil)
            {
                char buffer[8192];
                int len;
                printf("with " TEMPFILE "=\n");
                while ((len = fread(buffer, 1, 8192, fil)) > 0)
                    fwrite(buffer, 1, len, stdout);
                fclose(fil);
            }
        }
        rv = system(spname);
        unlink(TEMPFILE);

        if (rv)
            return rv;
       if (prm_targettype == WHXDOS)
       {
               sprintf(spname, "\"%spatchpe\" %s", root, outputFileName);
               if (verbosity)
                   printf("%s\n", spname);
               rv = system(spname);
            if (rv)
            {
                printf("Could not spawn patchpe.exe\n");
            }
       }
        if (rv)
            return rv;

    }
    return 0;
}
