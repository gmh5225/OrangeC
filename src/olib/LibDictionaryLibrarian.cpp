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

#include "LibDictionary.h"
#include "LibFiles.h"
#include "ObjFile.h"
#include "ObjSymbol.h"
#include "ObjSection.h"
#include "Utils.h"
#include <ctype.h>
#include <iostream>
#include <string.h>

void LibDictionary::CreateDictionary(LibFiles &files)
{
    int total = 0;
    int symbols = 0;
    Clear();
    int i = 0;
    for (LibFiles::FileIterator it = files.FileBegin(); it != files.FileEnd(); ++it)
    {
        const LibFiles::FileDescriptor *fd = (*it);
        if (fd->data)
        {	
            for (ObjFile::SymbolIterator pi = fd->data->PublicBegin(); pi != fd->data->PublicEnd(); ++pi)
            {
                InsertInDictionary((*pi)->GetName().c_str(), i);
            }
            for (ObjFile::SymbolIterator pi = fd->data->ImportBegin(); pi != fd->data->ImportEnd(); ++pi)
            {
                InsertInDictionary((*pi)->GetName().c_str(), i);
            }
            // support for virtual sections
            for (ObjFile::SectionIterator si = fd->data->SectionBegin(); si != fd->data->SectionEnd(); ++si)
            {
                if ((*si)->GetQuals() & ObjSection::virt)
                {
                    int j;
                    std::string name = (*si)->GetName();
                    for (j=0; j < name.size(); j++)
                        if (name[j] == '@')
                            break;
                    if (j < name.size())
                    {
                        name = name.substr(j);
                        InsertInDictionary(name.c_str(), i);
                    }
                }
            }
            i++;
        }
    }
}
void LibDictionary::InsertInDictionary(const char *name, int index)
{
    char buf[2048];
    int l = strlen(name);
    int n = l + 1;
    strncpy(buf, name, 2048);
    buf[2047] = 0;
    if (!caseSensitive)
        for (int i=0; i <= l; i++)
            buf[i] = toupper(buf[i]);
    dictionary[buf] = index;
}
void LibDictionary::Write(FILE *stream)
{
    char sig[4] = { '1','0',0,0 };
    fwrite(&sig[0], 4, 1, stream);
    for (auto d : dictionary)
    {
        short len = d.first.size();
        fwrite(&len, sizeof(len), 1, stream);
        fwrite(d.first.c_str(), len , 1, stream);
        ObjInt fileNum = d.second;
        fwrite(&fileNum, sizeof(fileNum), 1, stream);
    }
    short eof = 0;
    fwrite(&eof, sizeof(eof), 1, stream);
}
